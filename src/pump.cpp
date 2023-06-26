#include "pump.h"
#include <iomanip>

Pump::Pump(int id, vector<unique_ptr<FuelTank>>& tanks) : id_(id), tanks_(tanks)
{
	windowMutex = sharedResources.getPumpWindowMutex();

	// for Customer objects
	// pipe size is set to 1 so that one customer is serviced at a time.
	pipe = sharedResources.getPumpPipe(id_);
	pipeMutex = sharedResources.getPumpPipeMutex(id_);
	
	data = sharedResources.getPumpDpDataPtr(id_);

	dpMutex = sharedResources.getPumpDpDataMutex(id_);

	txnApprovedEvent = sharedResources.getTxnApprovedEvent(id_);

	rndv = sharedResources.getRndv();

	// semaphore with initial value 0 and max value 1
	producer = sharedResources.getProducer(id_);
	// semaphore with initial value 1 and max value 1
	consumer = sharedResources.getConsumer(id_);

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
	do {
		assert(customer.txnStatus == TxnStatus::Pending);
		dpMutex->Wait();
		*data = customer;
		dpMutex->Signal();
	} while (*data != customer);
	assert(customer.txnStatus == TxnStatus::Pending);
	


	// Used to identify the idle pump
	pumpStatus = sharedResources.getPumpStatus(id_);
	pumpStatusMutex = sharedResources.getPumpStatusMutex(id_);

	pumpStatusMutex->Wait();
	// Must initialize the values pointed by the pointer in the constructor.
	pumpStatus->busy = false;
	pumpStatus->isTransactionCompleted = true;
	pumpStatusMutex->Signal();

}

int
Pump::main()
{
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

		resetPump();
	}
	return 0;
}


void
Pump::sendTransactionInfo()
{
	consumer->Wait();

	dpMutex->Wait();
	*data = customer;
	assert(*data == customer);
	dpMutex->Signal();
	
	producer->Signal();
}

FuelTank&
Pump::getTank(int id)
{
	/**
	 * If you don't want to transfer the object's ownership, you should return a raw pointer
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
			txnApprovedEvent->Signal();
			do {
				if (chosen_tank.decrement()) {
					customer.receivedVolume += FLOW_RATE;
					customer.cost = price.getCost(customer.receivedVolume, customer.grade);
					sendTransactionInfo();
					SLEEP(1200);
				}
			} while (customer.receivedVolume < customer.requestedVolume && chosen_tank.checkRemainingVolume());
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

	customer.txnStatus = TxnStatus::Done;
	sendTransactionInfo(); // This is to ensure TxnStatus::Done is sent successfully to the computer.

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

	sendTransactionInfo();

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

	txnApprovedEvent->Wait();

	dpMutex->Wait();
	assert(data->txnStatus == TxnStatus::Approved);
	customer.txnStatus = data->txnStatus;
	dpMutex->Signal();

}

int
Pump::getId()
{
	return id_;
}

void
Pump::rendezvousOnce()
{
	static bool has_run = false;

	if (!has_run) {
		rndv->Wait();
		has_run = true;
	}
}