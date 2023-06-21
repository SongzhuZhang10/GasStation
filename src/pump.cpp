#include "pump.h"
#include <iomanip>

Pump::Pump(int id, vector<unique_ptr<FuelTank>>& tanks)
	:	_id(id),
		tanks_(tanks)
{
	windowMutex = make_unique<CMutex>("PumpScreenMutex");

	// for Customer objects
	// pipe size is set to 1 so that one customer is serviced at a time.
	pipe = make_unique<CTypedPipe<CustomerRecord>>(getName("Pipe", _id, ""), 1);
	pipeMutex = make_unique<CMutex>(getName("PipeMutex", _id, ""));
	
	
	dataPool = make_unique<CDataPool>(getName("PumpDataPool", _id, ""), sizeof(CustomerRecord));
	data.reset(static_cast<CustomerRecord*>(dataPool->LinkDataPool()));
	dpMutex = make_unique<CMutex>(getName("PumpDataPoolMutex", _id, ""));

	txnApproved = make_unique<CEvent>(getName("TxnApprovedByPump", _id, ""));

	// Create a rendezvous object on the heap
	rendezvous = make_unique<CRendezvous>("PumpRendezvous", NUM_PUMPS + 1);

#if 0
	cout << "Before initializing data pointer!" << endl; // should be printed only once.
	cout << "data->name = " << data->name << endl;
	cout << "data->creditCardNumber = " << data->creditCardNumber << endl;
	cout << "data->pumpId = " << data->pumpId << endl;


	/* TODO: Check if the value of the pointer is the same as the one in the computer class */
	dpMutex->Wait();
	resizeToFit(data->creditCardNumber, "0000 0000 0000");
	resizeToFit(data->name, "Unknown");
	dpMutex->Signal();
#endif
	//assert(sizeof(*data) == sizeof(customer));

	/*
	 * 1. Do not forget to initialize the value pointed by the pointer `data`. Or, the data pointed
	 * by the `data` pointer cannot be accessed properly later on.
	 * 2. You need to use a mutex to protect the datapool (specifically, protect the pointer pointing
	 * to the data in the data pool). Or you may get an `access violation writing location` exception
	 * in the memcpy.asm file or a weird bug that it needs 2 times of assignment operation to modify the
	 * `*data` variable properply.
	 * 3. Only the owner of the data pool (i.e., pump class) needs to initialize the data pool.
	 * If both the pump class and the computer class initialize the data pool in their own constructor,
	  an exception in the memcpy.asm file will be induced.
	 */
	dpMutex->Wait();
	do {
		cout << "Initializing data pointer!" << endl; // should be printed only once.	
		//data->txnStatus = customer.txnStatus;
		//data->pumpId = customer.pumpId;
		//data->requestedVolume = customer.requestedVolume;
		//resizeToFit(data->name, customer.name);
		//data->creditCardNumber = customer.creditCardNumber;
		assert(customer.txnStatus == TxnStatus::Pending);
		*data = customer;	

		cout << "data->name = " << data->name << endl;
		cout << "data->creditCardNumber = " << data->creditCardNumber << endl;
		cout << "data->pumpId = " << data->pumpId << endl;
	} while (data->name != customer.name || data->creditCardNumber != customer.creditCardNumber);
	assert(customer.txnStatus == TxnStatus::Pending);

	if (data->name != customer.name)
		cout << "BUG: Name: " << data->name << endl;
	if (data->creditCardNumber != customer.creditCardNumber)
		cout << "BUG: Credit card number: " << data->creditCardNumber << endl;
	if (data->grade != customer.grade)
		cout << "BUG: Fuel Grade: " << fuelGradeToString(data->grade) << endl;
	if (data->requestedVolume != customer.requestedVolume)
		cout << "BUG: Requested volume: " << data->requestedVolume << endl;
	if (data->pumpId != customer.pumpId)
		cout << "BUG: Pump ID: " << data->pumpId << endl;
	if (sizeof(*data) != sizeof(customer)) {
		cout << "BUG: Size of *data = " << sizeof(*data) << endl;
		cout << "BUG: Size of customer = " << sizeof(customer) << endl;
	}
	dpMutex->Signal();


	// Used to identify the idle pump
	flagDataPool = make_unique<CDataPool>(getName("PumpBusyFlagDataPool", _id, ""), sizeof(PumpStatus));
	pumpStatus.reset(static_cast<PumpStatus*>(flagDataPool->LinkDataPool()));
	pumpStatusMutex = make_unique<CMutex>(getName("PumpStatusMutex", _id, ""));

	pumpStatusMutex->Wait();
	// Must initialize the values pointed by the pointer in the constructor.
	pumpStatus->busy = false;
	pumpStatus->isTransactionCompleted = true;
	pumpStatusMutex->Signal();

	windowMutex->Wait();
	cout << "Pump " << _id << " has been created." << endl;
	fflush(stdout);
	windowMutex->Signal();
}

int
Pump::main()
{
#if 0
	ClassThread<Pump> readPipeThread(this, &Pump::serviceCustomer, ACTIVE, NULL);
	readPipeThread.WaitForThread();
	return 0;
#endif
	while (true) {
		windowMutex->Wait();
		cout << "Pump " << _id << " is ready to service a customer." << endl;
		fflush(stdout);
		windowMutex->Signal();

		if (customer.txnStatus != TxnStatus::Pending)
			cout << "DEBUG 1: customer.txnStatus = " << txnStatusToString(customer.txnStatus) << endl;

		readPipe();

		if (customer.txnStatus != TxnStatus::Pending)
			cout << "DEBUG 2: customer.txnStatus = " << txnStatusToString(customer.txnStatus) << endl;

		sendTransactionInfo();

		if (customer.txnStatus != TxnStatus::Pending)
			cout << "DEBUG 3: customer.txnStatus = " << txnStatusToString(customer.txnStatus) << endl;

		waitForAuth();

		if (customer.txnStatus != TxnStatus::Approved)
			cout << "DEBUG 4: customer.txnStatus = " << txnStatusToString(customer.txnStatus) << endl;

		assert(customer.txnStatus == TxnStatus::Approved);

		sendTransactionInfo();

		getFuel();

		sendTransactionInfo();

		SLEEP(4000);
		resetPump();
	}
	return 0;
}


void
Pump::sendTransactionInfo()
{
	cout << "Entered sendTransactionInfo" << endl;

	dpMutex->Wait();

	*data = customer;
	
	if (data->name != customer.name)
		cout << "BUG: Name: " << data->name << endl;
	if (data->creditCardNumber != customer.creditCardNumber)
		cout << "BUG: Credit card number: " << data->creditCardNumber << endl;
	if (data->grade != customer.grade)
		cout << "BUG: Fuel Grade: " << fuelGradeToString(data->grade) << endl;
	if (data->requestedVolume != customer.requestedVolume)
		cout << "BUG: Requested volume: " << data->requestedVolume << endl;
	if (data->pumpId != customer.pumpId)
		cout << "BUG: Pump ID: " << data->pumpId << endl;
	if (data->receivedVolume != customer.receivedVolume)
		cout << "BUG: receivedVolume = " << data->receivedVolume << endl;
	if (sizeof(*data) != sizeof(customer))
		cout << "BUG: Size of data = " << sizeof(*data) << endl;
	if (data->txnStatus != customer.txnStatus)
		cout << "BUG: data->txnStatus = " << txnStatusToString(data->txnStatus) << endl;
	dpMutex->Signal();

	assert(*data == customer);
	cout << "Exicted sendTransactionInfo" << endl;
}

FuelTank&
Pump::getTank(int id)
{
	/* If you don't want to transfer the object's ownership, you should return a raw pointer
	 * or a reference, not a smart pointer.
	 */
	return *(tanks_[id]);
}

void
Pump::getFuel()
{
	assert(pumpStatus->isTransactionCompleted == false);
	int tank_id = fuelGradeToInt(customer.grade);
	// It's best to use a local temp variable here rather than a private variable of the class.
	FuelTank& chosen_tank = getTank(tank_id);
	assert(chosen_tank.getFuelGrade() == customer.grade);

	if (customer.txnStatus == TxnStatus::Approved) {
		if (chosen_tank.readVolume() >= customer.requestedVolume) {
			txnApproved->Signal();
			do {
				if (chosen_tank.decrement()) {
					windowMutex->Wait();
					cout << chosen_tank.readVolume() << " liters of fuel left in the tank." << endl;
					fflush(stdout);
					windowMutex->Signal();
					// TODO: Make this a running cost as apposed to a final cost available only
					// after the requested fuel has all been dispensed.
					customer.receivedVolume += FLOW_RATE;
					customer.cost = price.getCost(customer.receivedVolume, customer.grade);
					sendTransactionInfo();
					SLEEP(1200);
				}
			} while (customer.receivedVolume < customer.requestedVolume && chosen_tank.checkRemainingVolume());
			customer.txnStatus = TxnStatus::Done;
		}
		else {
			cout << "Warning: The fuel tank does not have enough fuel left. Customer transaction cannot be completed." << endl;
			// No charge to the customer in this branch.
		}
	}
	else {
		assert(customer.txnStatus != TxnStatus::Pending);
		cout << "Customer transaction was denied by the gas station attendant. Fuel cannot be dispensed." << endl;
		// No charge to the customer in this branch.
	}
	pumpStatusMutex->Wait();
	cout << "getFuel informs the completion of the transaction to the customer" << endl;
	pumpStatus->isTransactionCompleted = true;
	pumpStatusMutex->Signal();
}

void
Pump::resetPump()
{
	cout << "Entered resetPump" << endl;
	assert(pumpStatus->busy == true);
	dpMutex->Wait();
	customer.resetToDefault();

	

	do {
		cout << "Initializing data pointer!" << endl; // should be printed only once.	
		//data->txnStatus = customer.txnStatus;
		//data->pumpId = customer.pumpId;
		//data->requestedVolume = customer.requestedVolume;
		//resizeToFit(data->name, customer.name);
		//data->creditCardNumber = customer.creditCardNumber;
		*data = customer;

		cout << "data->name = " << data->name << endl;
		cout << "data->creditCardNumber = " << data->creditCardNumber << endl;
		cout << "data->pumpId = " << data->pumpId << endl;
		cout << "data->cost = " << data->cost << endl;
		cout << "data->requestedVolume = " << data->requestedVolume << endl;
		cout << "data->receivedVolume = " << data->receivedVolume << endl;
		cout << "data->txnStatus = " << txnStatusToString(data->txnStatus) << endl;
	} while ( *data != customer );
	dpMutex->Signal();

	pumpStatusMutex->Wait();
	pumpStatus->busy = false; // notify the customer the transaction is done.
	pumpStatusMutex->Signal();
	cout << "Exited resetPump" << endl;
}
void
Pump::readPipe()
{
	/**
	 * One should not use `pipeMutex->Wait();` and `pipeMutex->Signal();` to guard
	 * `pipe->Read(&customer);`. For inter-process comunication with pipelines, either
	 * the writer or the reader needs the mutex protection, not both. Or, a deadlock
	 * will be induced. Since the pump is the only reader to its pipe, it does not
	 * need mutex protection.
	 */
	/* BUG: The creditCardNumber value of customer is empty when it's printed out.
	 * This is why `data->creditCardNumber = customer.creditCardNumber;` later on can cause
	 * the memcpy exception.
	*/
	//assert(customer.txnStatus == TxnStatus::Pending);
	if (customer.txnStatus != TxnStatus::Pending)
		cout << "DEBUG before reading pipe: customer.txnStatus = " << txnStatusToString(customer.txnStatus) << endl;

	windowMutex->Wait();
	cout << "Pump " << _id << " has arrived at Rendezvous and is about to read the pipe ..." << endl;
	fflush(stdout);
	windowMutex->Signal();

	rendezvousOnce();

	pipe->Read(&customer);

	if (customer.txnStatus != TxnStatus::Pending)
		cout << "DEBUG after reading pipe: customer.txnStatus = " << txnStatusToString(customer.txnStatus) << endl;


	pumpStatusMutex->Wait();
	pumpStatus->isTransactionCompleted = false;
	pumpStatusMutex->Signal();

	cout << "Customer data read from the pipe is as follows:" << endl;
	cout << "customer.name = " << customer.name << endl;
	cout << "customer.pumpId = " << customer.pumpId << endl;
	cout << "customer.creditCardNumber = " << customer.creditCardNumber << endl;
	cout << "customer.requestedVolume = " << customer.requestedVolume << endl;
	cout << "customer.receivedVolume = " << customer.receivedVolume << endl;
	cout << "customer.txnStatus = " << txnStatusToString(customer.txnStatus) << endl;
	assert(customer.txnStatus == TxnStatus::Pending);
}

void
Pump::waitForAuth()
{
	cout << "Entered waitForAuth" << endl;
	assert(customer.txnStatus == TxnStatus::Pending);

	txnApproved->Wait();

	dpMutex->Wait();

	cout << "Customer Name (before reading dp): " << customer.name << endl;
	cout << "Customer Auth (before reading dp): " << txnStatusToString(customer.txnStatus) << endl;

	customer.txnStatus = data->txnStatus;

	cout << "Customer Name (after reading dp): " << customer.name << endl;
	cout << "Customer Auth (after reading dp): " << txnStatusToString(customer.txnStatus) << endl;

	dpMutex->Signal();


	cout << "Exited waitForAuth" << endl;
}

int
Pump::getId()
{
	return _id;
}

void
Pump::rendezvousOnce()
{
	static bool has_run = false;

	if (!has_run) {
		rendezvous->Wait();
		has_run = true;
	}
}