#include "customer.h"
#include <cstdlib>
#include <random>
#include <cmath>

using namespace std;

constexpr int MIN_LITERS = 5;
constexpr int MAX_LITERS = 70;

/*
* Receive fuel should not happen after returning the pump hose. Need to fix this.
*/
Customer::Customer(vector<unique_ptr<Pump>>& pumps, FuelPrice& fuelPrice)
    : pumpId(-1), pumps_(pumps), fuelPrice_(fuelPrice)
{
    windowMutex = sharedResources.getPumpWindowMutex();
    pipe = sharedResources.getPumpPipeVec();
    txnApprovedEvent = sharedResources.getTxnApprovedEventVec();
    
    data.name = getRandomName();
    data.requestedVolume = getRandomFloat(MIN_LITERS, MAX_LITERS);
    data.txnStatus = TxnStatus::Pending;
    status = CustomerStatus::Null;
    pumpEnquiryMutex = make_unique<CMutex>("PumpEnquiryMutex");
}

int
Customer::getAvailPumpId()
{
    while (true) {
        pumpEnquiryMutex->Wait();
        for (int i = 0; i < NUM_PUMPS; i++) {
            if ( !pumps_[i]->isBusy() ) {
                // Own the pump so that it cannot be shared by others.
                pumps_[i]->setBusy();
                data.pumpId = i;
                pumpEnquiryMutex->Signal();
                assert(pumps_[i]->isBusy() == true);
                return i;
            }
        }
        pumpEnquiryMutex->Signal();
    }
}

void
Customer::arriveAtPump()
{
    status = CustomerStatus::WaitForPump;

    pumpId = getAvailPumpId();

    pumpDpMutex = sharedResources.getPumpDpDataMutex(pumpId);

    status = CustomerStatus::ArriveAtPump;
}

void
Customer::swipeCreditCard()
{
    data.creditCardNumber = getRandomCreditCardNumber();
    status = CustomerStatus::SwipeCreditCard;
}

void
Customer::removeGasHose()
{
    status = CustomerStatus::RemoveGasHose;
}

void
Customer::selectFuelGrade()
{
    data.grade = getRandomFuelGrade();
    //data.grade = FuelGrade::Oct87;

    status = CustomerStatus::SelectFuelGrade;
    
    assert(fuelGradeToInt(data.grade) >= 0 && fuelGradeToInt(data.grade) <= 3);

    data.unitCost = fuelPrice_.getUnitCost(data.grade);

    writePipe(&data);
}

void
Customer::getFuel()
{
    status = CustomerStatus::WaitForAuth;

    txnApprovedEvent[pumpId]->Wait();
    
    status = CustomerStatus::GetFuel;

    do {
        pumpDpMutex->WaitToRead();
        data.receivedVolume = pumps_[pumpId]->getReceivedVolume();
        data.cost = pumps_[pumpId]->getTotalCost();
        pumpDpMutex->DoneReading();
    } while (data.receivedVolume < data.requestedVolume);
}

void
Customer::writePipe(CustomerRecord* customer)
{
    assert(pumpId != -1);
    assert(pumps_[pumpId]->isBusy() == true);
    assert(pipe[pumpId]->TestForData() == 0);
    
    /**
     * There might be multiple customers trying to write to the same pipe.
     * So, use mutex here. If the lock-unlock pump mechanism works perfectly,
     * then it may not be necessary to mutex here. Verify the mutex is truly necessary later on.
     */

    pipe[pumpId]->Write(customer);
}

void
Customer::returnGasHose()
{
    status = CustomerStatus::ReturnGasHose;
}

void
Customer::driveAway()
{
    status = CustomerStatus::DriveAway;
}

string
Customer::getRandomName()
{
    // Define a list of possible names
    vector<string> names = {
        "Alice", "Bob", "Charlie", "David", "Eve", "Frank",
        "Grace", "John", "Ivan", "Jane", "Kevin", "Linda",
        "Songzhu", "Tippy", "Mike", "William", "Emma", "Emily",
        "Sophia", "Mia", "Ava", "Andrew", "Songzhu", "Ruby", "John",
        "Olivia", "Tucker", "James", "Isabella", "Carolina", "Dale",
        "Chad", "Allison", "Chuck"
    };

    // Generate a random index
    // Creates a source of non-deterministic random numbers
    random_device rd;

    /*
     * Create a random number engine (mt19937) object named rng.
     * Initialize it with the random device rd as its seed. 
     * mt19937 is a widely-used random number generator algorithm.
     */
    mt19937 rng(rd());

    // Produce integers within the range of 0 to `names.size() - 1`.
    uniform_int_distribution<int> dist(0, static_cast<int>(names.size() - 1));

    /*
     * Assign the generated random integer to the variable index,
     * which represents the randomly selected index of the name to
     * be returned.
     */
    int index = dist(rng);

    // Return the randomly selected name
    return names[index];
}

/*
 * The pipeline read and write cannot work properly if 
 * the string variable representing credit card number
 * is too long (more than exactly 15 digits). Four digits can ensure that bank card string
 * can be passed to other threads through CPipe properply and
 * can be assigned to the dereferenced data pool pointer without causing the memcpy exception.
 * However, the CDataPool cannot handle string variables in a deterministic way.
 */
string
Customer::getRandomCreditCardNumber()
{
    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution<int> dist(0, 9);

    string creditCardNumber;

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j) {
            creditCardNumber += to_string(dist(rng));
        }
        if (i != 3) {
            creditCardNumber += " ";
        }
    }
    return creditCardNumber;
}
   

FuelGrade
Customer::getRandomFuelGrade()
{
    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution<int> dist(0, NUM_TANKS - 1);

    int randomValue = dist(rng);
    assert(randomValue >= 0 && randomValue <= 3);
    return intToFuelGrade(randomValue);
}

float
Customer::getRandomFloat(float min, float max) {
    random_device rd;  // Obtain a random seed from the hardware
    mt19937 gen(rd());  // Seed the random number generator
    uniform_real_distribution<float> dis(min, max);  // Define the range
    // Round a floating point value to three decimal places
    return round(dis(gen) * 1000) / 1000;  // Generate and return a random float
}

int
Customer::main(void)
{
    arriveAtPump();
    swipeCreditCard();
    removeGasHose();
    selectFuelGrade();
    getFuel();
    returnGasHose();
    driveAway();
    return 0;
}

string
Customer::customerStatusToString(const CustomerStatus& status) const
{
    // `static const` means it is shared among all instances of Customer and
    // won't change once initialized. 
    static const unordered_map<CustomerStatus, string> status_to_string = {
        {CustomerStatus::WaitForPump, "Wait for pump"},
        {CustomerStatus::ArriveAtPump, "Arrive at pump"},
        {CustomerStatus::SwipeCreditCard, "Swipe credit card"},
        {CustomerStatus::RemoveGasHose, "Remove gas hose"},
        {CustomerStatus::SelectFuelGrade, "Select fuel grade"},
        {CustomerStatus::WaitForAuth, "Wait for auth"},
        {CustomerStatus::GetFuel, "Getting fuel"},
        {CustomerStatus::ReturnGasHose, "Return gas hose"},
        {CustomerStatus::DriveAway, "Drive away"},
        {CustomerStatus::Null, "Null"}
    };

    // The.at() function is used to retrieve the value associated with the given key.
    // It will throw an exception if the key doesn't exist, which can help us catch potential errors.
    return status_to_string.at(status);
}

CustomerRecord &
Customer::getData()
{
    return data;
}

string
Customer::getStatusString()
{
    return customerStatusToString(status);
}