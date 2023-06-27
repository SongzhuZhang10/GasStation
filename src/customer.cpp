#include "customer.h"
#include <cstdlib>

constexpr int MIN_LITERS = 5;
constexpr int MAX_LITERS = 70;

/*
* Receive fuel should not happen after returning the pump hose. Need to fix this.
*/
Customer::Customer() : pumpId(-1)
{
    windowMutex = sharedResources.getPumpWindowMutex();
    pipe = sharedResources.getPumpPipeVec();
    pumpFlagDataPool = sharedResources.getPumpFlagDataPoolVec();
    txnApprovedEvent = sharedResources.getTxnApprovedEventVec();
    pumpStatuses = sharedResources.getPumpStatusVec();

    data.name = getRandomName();
    data.requestedVolume = getRandomFloat(MIN_LITERS, MAX_LITERS);
    data.txnStatus = TxnStatus::Pending;
    status = CustomerStatus::Null;
    pumpEnquiryMutex = make_unique<CMutex>("PumpEnquiryMutex");
}

int
Customer::getAvailPumpId()
{
    status = CustomerStatus::WaitForPump;

    while (true) {
        pumpEnquiryMutex->Wait();
        for (int i = 0; i < NUM_PUMPS; i++) {
            if (!pumpStatuses[i]->busy) {
                // Own the pump so that it cannot be shared by others
                pumpStatuses[i]->busy = true;
                data.pumpId = i;
                pumpEnquiryMutex->Signal();
                return i;
            }
        }
        pumpEnquiryMutex->Signal();
    }
}

void
Customer::arriveAtPump()
{
    
    pumpId = getAvailPumpId();

    pumpData = sharedResources.getPumpDpDataPtr(pumpId);
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

    data.unitCost = price.getUnitCost(data.grade);

    writePipe(&data);
}

void
Customer::getFuel()
{
    status = CustomerStatus::WaitForAuth;
    bool txn_completed = false;

    txnApprovedEvent[pumpId]->Wait();
    
    status = CustomerStatus::GetFuel;

    do {
        // Get real time received volume and total cost directly from the GSC rather than from the pump data pool
        pumpDpMutex->Wait();
        data.receivedVolume = pumpData->receivedVolume;
        data.cost = pumpData->cost;
        pumpDpMutex->Signal();

        txn_completed = pumpStatuses[pumpId]->isTransactionCompleted;
    } while (!txn_completed);
}

void
Customer::writePipe(CustomerRecord* customer)
{
    assert(pumpId != -1);
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

    return dis(gen);  // Generate and return a random float
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