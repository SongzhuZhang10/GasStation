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

    resizeToFit(data.name, getRandomName());

    data.requestedVolume = getRandomFloat(MIN_LITERS, MAX_LITERS);

    windowMutex->Wait();
    cout << "Customer " << data.name << " was created to request " << data.requestedVolume << " liters of fuel." << endl;
    windowMutex->Signal();
    data.txnStatus = TxnStatus::Pending;

}

int
Customer::getAvailPumpId()
{
    int time_out = 0;
    while (true) {
        for (int i = 0; i < NUM_PUMPS; i++) {
            pumpMutex[i]->Wait();
            bool busy_flag = pumpStatuses[i]->busy;
#if DEBUG_MODE
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
    windowMutex->Wait();
    cout << "Customer " << data.name << " is arriving at a pump" << endl;
    fflush(stdout);
    windowMutex->Signal();

    pumpId = getAvailPumpId();

    windowMutex->Wait();
    cout << "Found an idling pump with ID " << pumpId << endl;
    fflush(stdout);
    windowMutex->Signal();
    assert(data.txnStatus == TxnStatus::Pending);
}

void
Customer::swipeCreditCard()
{
    resizeToFit(data.creditCardNumber, getRandomCreditCardNumber());

    windowMutex->Wait();
    cout << "Customer " << data.name << " credit card number: " << data.creditCardNumber << endl;
    fflush(stdout);
    windowMutex->Signal();
}

void
Customer::removeGasHoseFromPump()
{
    windowMutex->Wait();
    cout << "Customer " << data.name << " removed the gas hose from the pump." << endl;
    fflush(stdout);
    windowMutex->Signal();
}

void
Customer::selectFuelGrade()
{
    data.grade = getRandomFuelGrade();
    //data.grade = FuelGrade::Oct87;
    //data.grade = FuelGrade::Oct89;
    assert(fuelGradeToInt(data.grade) >= 0 && fuelGradeToInt(data.grade) <= 3);
    windowMutex->Wait();
    cout << "Customer " << data.name << " selected fuel " << fuelGradeToString(data.grade) << endl;
    fflush(stdout);
    windowMutex->Signal();

    data.unitCost = price.getUnitCost(data.grade);

#if 0
    pumpMutex[pumpId]->Wait();
    bool busy_flag = pumpStatuses[pumpId]->busy;
    pumpMutex[pumpId]->Signal();
    assert(busy_flag == true);
    cout << "Customer is about to write data to the pipe." << endl;
    cout << "DEBUG: credit card number: " << data.creditCardNumber << endl;
#endif

    writePipe(&data);
#if DEBUG_MODE
    windowMutex->Wait();
    cout << "Customer has written data to the pipe." << endl;
    fflush(stdout);
    windowMutex->Signal();
#endif

#if DEBUG_MODE
    do {
        SLEEP(2000);
        if (pipe[pumpId]->TestForData() != 0) {
            windowMutex->Wait();
            cout << "Debug info: data written to the pipe is not consumed." << endl;
            fflush(stdout);
            windowMutex->Signal();
        }
    } while (pipe[pumpId]->TestForData() != 0);
    
#endif
}

void
Customer::getFuel()
{

    windowMutex->Wait();
    cout << "Customer " << data.name << " is ready to get their fuel." << endl;
    fflush(stdout);
    windowMutex->Signal();

    bool is_transaction_completed = false;
    do {
        pumpMutex[pumpId]->Wait();
        is_transaction_completed = pumpStatuses[pumpId]->isTransactionCompleted;
        pumpMutex[pumpId]->Signal();
        cout << "Receiving fuel..." << endl;
        SLEEP(1300);
    } while (!is_transaction_completed);

    windowMutex->Wait();
    cout << "Requested amount of fuel has been received." << endl;
    fflush(stdout);
    windowMutex->Signal();
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

    windowMutex->Wait();
    cout << "data has been written to the pipe" << endl;
    fflush(stdout);
    windowMutex->Signal();

    pipeMutex[pumpId]->Signal();
}

void
Customer::returnHoseToPump()
{
    windowMutex->Wait();
    cout << "Customer " << data.name << " returned hose to the pump." << endl;
    windowMutex->Signal();
}

void
Customer::driveAway()
{
    windowMutex->Wait();
    cout << "Customer drove away." << endl;
    fflush(stdout);
    windowMutex->Signal();
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
    //for (int i = 0; i < 15; ++i) {
#if 0
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            creditCardNumber += to_string(dist(rng));
        }
        if (i != 3) {
            creditCardNumber += " ";
        }
    }
#endif
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
    removeGasHoseFromPump();
    selectFuelGrade();
    getFuel();
    returnHoseToPump();
    driveAway();
    //cout << "Press enter to terminal Customer " << data.name << " thread " << endl;
    //waitForKeyPress();
    return 0;
}