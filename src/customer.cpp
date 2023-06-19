#include "customer.h"
#include <cstdlib>

constexpr int MIN_LITERS = 5;
constexpr int MAX_LITERS = 70;

/*
* Receive fuel should not happen after returning the pump hose. Need to fix this.
*/
Customer::Customer() : pumpId(-1)
{
    windowMutex = make_unique<CMutex>("CustomerWindowMutex");

    for (int i = 0; i < NUM_PUMPS; i++) {
        pipe.emplace_back(make_unique<CTypedPipe<CustomerRecord>>(getName("Pipe", i, ""), 1));
        pipeMutex.emplace_back(make_unique<CMutex>(getName("PipeMutex", i, "")));
        pumpFlagDataPool.emplace_back(make_unique<CDataPool>(getName("PumpBusyFlagDataPool", i, ""), sizeof(PumpStatus)));
        pumpMutex.emplace_back(make_unique<CMutex>(getName("PumpMutex", i, "")));
    }
    for (int i = 0; i < NUM_PUMPS; i++) {
        pumpStatuses.emplace_back(static_cast<PumpStatus*>(pumpFlagDataPool[i]->LinkDataPool()));
    }

    data.name = getRandomName();
    data.requestedVolume = getRandomFloat(MIN_LITERS, MAX_LITERS);
    data.txnStatus = TxnStatus::Pending;
    status = CustomerStatus::Null;
}

int
Customer::getAvailPumpId()
{
    //int time_out = 0;
    while (true) {
        for (int i = 0; i < NUM_PUMPS; i++) {
            // TODO: Should mutex be placed outside the for loop?
            pumpMutex[i]->Wait();
            bool busy_flag = pumpStatuses[i]->busy;
#if 0
            if (busy_flag) {
                windowMutex->Wait();
                cout << "Pump " << i << " is busy." << endl;
                fflush(stdout);
                windowMutex->Signal();
            }
            else {
                windowMutex->Wait();
                cout << "Pump " << i << " is free." << endl;
                fflush(stdout);
                windowMutex->Signal();
            }
#endif
            if ( !busy_flag ) {
                // Own the pump so that it cannot be shared by others
                pumpStatuses[i]->busy = true;
                pumpMutex[i]->Signal();
                data.pumpId = i;
                return i;
            }
            pumpMutex[i]->Signal();
        }
#if 0
        if (++time_out == 10) {
            windowMutex->Wait();
            cout << "Warning: Tried to get an available pump too many times." << endl;
            fflush(stdout);
            windowMutex->Signal();
            exit(-1);
        }
#endif
        SLEEP(1500);
    }
}

void
Customer::arriveAtPump()
{
    
    pumpId = getAvailPumpId();
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
    //data.grade = FuelGrade::Oct89;
    assert(fuelGradeToInt(data.grade) >= 0 && fuelGradeToInt(data.grade) <= 3);

    data.unitCost = price.getUnitCost(data.grade);

    writePipe(&data);


#if DEBUG_MODE
    do {
        if (pipe[pumpId]->TestForData() != 0) {
            SLEEP(2000);
        }
    } while (pipe[pumpId]->TestForData() != 0);
#endif
    status = CustomerStatus::SelectFuelGrade;
}

void
Customer::getFuel()
{
    status = CustomerStatus::WaitForAuth;
    
    TxnStatus txn_status = TxnStatus::Pending;
    do {
        pumpMutex[pumpId]->Wait();
        txn_status = pumpStatuses[pumpId]->txnStatus;
        pumpMutex[pumpId]->Signal();
    } while (txn_status == TxnStatus::Pending);
    
    status = CustomerStatus::GetFuel;

    bool txn_completed = false;
    do {
        pumpMutex[pumpId]->Wait();
        txn_completed = pumpStatuses[pumpId]->isTransactionCompleted;
        pumpMutex[pumpId]->Signal();
        SLEEP(3000);
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
    pipeMutex[pumpId]->Wait();

    assert(pipe[pumpId]->TestForData() == 0);
    pipe[pumpId]->Write(customer);

    pipeMutex[pumpId]->Signal();
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
        "Sophia", "Mia", "Ava", "Andrew", "Songzhu", "Ruby", "John"
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
 * can be assigned to the dereferenced data pool pointer without 
 * causing the memcpy exception.
 * However, the CDataPool cannot handle string variables
 * in a deterministic way.
 * 
 * 
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
Customer::getStatus()
{
    return customerStatusToString(status);
}