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
		assert(customer.txnStatus == TxnStatus::Pending);
		*data = customer;
	} while (data->name != customer.name || data->creditCardNumber != customer.creditCardNumber);
	assert(customer.txnStatus == TxnStatus::Pending);

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
	dpMutex->Wait();
	*data = customer;
	dpMutex->Signal();
	assert(*data == customer);
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
	// Inform the completion of the transaction to the customer
	pumpStatus->isTransactionCompleted = true;
	pumpStatusMutex->Signal();
}

void
Pump::resetPump()
{
	assert(pumpStatus->busy == true);
	customer.resetToDefault();

	dpMutex->Wait();
	do {
		*data = customer;
	} while ( *data != customer );
	dpMutex->Signal();

	pumpStatusMutex->Wait();
	pumpStatus->busy = false; // notify the customer the transaction is done.
	pumpStatusMutex->Signal();
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

	// This pump has arrived at Rendezvous and is about to read the pipe ...
	rendezvousOnce();

	pipe->Read(&customer);

	if (customer.txnStatus != TxnStatus::Pending)
		cout << "DEBUG after reading pipe: customer.txnStatus = " << txnStatusToString(customer.txnStatus) << endl;


	pumpStatusMutex->Wait();
	pumpStatus->isTransactionCompleted = false;
	pumpStatusMutex->Signal();

	assert(customer.txnStatus == TxnStatus::Pending);
}

void
Pump::waitForAuth()
{
	assert(customer.txnStatus == TxnStatus::Pending);

	txnApproved->Wait();

	dpMutex->Wait();
	assert(data->txnStatus == TxnStatus::Approved);
	customer.txnStatus = data->txnStatus;
	dpMutex->Signal();

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