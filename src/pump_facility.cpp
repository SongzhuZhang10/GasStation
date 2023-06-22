#include "pump_facility.h"

/*
 * Plan: incorperate the four tanks inside the pump facility which is the top level.
 * The pump facality also includes the four pumps.
 * So, the main function of this project is the pump facility, which means the PumpFacility class is not needed.
 */

/***********************************************
 *                                             *
 *                Tanks                        *
 *                                             *
 ***********************************************/
vector<unique_ptr<FuelTank>> tanks;
unique_ptr<CMutex> windowMutex = make_unique<CMutex>("PumpScreenMutex");

void
setupTanks()
{
	for (int i = 0; i < NUM_TANKS; i++) {
		tanks.emplace_back(make_unique<FuelTank>(i));
	}
	for (int i = 0; i < NUM_TANKS; i++) {
		assert(fuelGradeToInt(tanks[i]->getFuelGrade()) == i);
	}
}

/***********************************************
 *                                             *
 *                Pumps                        *
 *                                             *
 ***********************************************/
vector<unique_ptr<Pump>> pumps;

void
setupPumpFacility()
{
	for (int i = 0; i < NUM_PUMPS; i++) {
		pumps.emplace_back(make_unique<Pump>(i, tanks));
		pumps[i]->Resume();
	}
}

/***********************************************
 *                                             *
 *                Customers                    *
 *                                             *
 ***********************************************/

vector<unique_ptr<Customer>> customers;


vector<unique_ptr<CThread>> readPumpThreads;
vector<int> threadIds = { 0, 1, 2, 3 };
vector<unique_ptr<PumpData>> pumpDataOps;


unique_ptr<CTypedPipe<CustomerRecord>> customerPipe = make_unique<CTypedPipe<CustomerRecord>>("CustomerPipe", 1);
unique_ptr<CMutex> customerPipeMutex = make_unique<CMutex>("CustomerPipeMutex");
unique_ptr<CRendezvous> rendezvous = make_unique<CRendezvous>("PumpRendezvous", NUM_PUMPS + 1);

void
writeTxnToPipe(const unique_ptr<PumpData>& pump_data_ptr)
{

	CustomerRecord txn = pump_data_ptr->getData();

	if (txn.txnStatus == TxnStatus::Done) {
		txn.txnStatus = TxnStatus::Archived;
		pump_data_ptr->archiveData();
		assert(pump_data_ptr->getData().txnStatus == TxnStatus::Archived);

		customerPipeMutex->Wait();
		customerPipe->Write(&txn);
		customerPipeMutex->Signal();
	}
}

UINT __stdcall printCustomers(void* args)
{
	windowMutex->Wait();
	MOVE_CURSOR(0, CUSTOMER_STATUS_POSITION - 3);
	cout << "--------------------------------------------------------------------------------" << endl;
	cout << "                           Customer Information                                 " << endl;
	cout << "--------------------------------------------------------------------------------" << endl;
	fflush(stdout);
	windowMutex->Signal();

	while (true) {
		for (size_t i = 0; i < customers.size(); ++i) {
			printCustomerRecord(i);
		}
		SLEEP(2500);
	}
}

UINT __stdcall
readPump(void* args)
{
	int id = *(int*)(args);
	assert(id >= 0 && id <= 3);

	pumpDataOps[id]->printCustomer(pumpDataOps[id]->getData());

	while (true) {

		pumpDataOps[id]->readData();

		writeTxnToPipe(pumpDataOps[id]);

		pumpDataOps[id]->printPumpData();

		SLEEP(REFRESH_RATE);
	}
	return 0;
}

void
printCustomerRecord(int idx)
{
	const int block_height = 9;
	static vector<CustomerRecord> prev_records(NUM_CUSTOMERS); // declare a vector with a size of `NUM_CUSTOMERS`
	static vector<string> prev_statuses(NUM_CUSTOMERS, "Null"); // declare a vector with a size of `NUM_CUSTOMERS`, initialized with value "Null"
	static vector<CustomerRecord> records(NUM_CUSTOMERS);

	records[idx] = customers[idx]->getData();
#
	if (prev_records[idx] == records[idx] && prev_statuses[idx] == customers[idx]->getStatus()) {
		return;
	}
	else {
		if (records[idx].pumpId == -1) {
			assert(customers[idx]->getStatus() == "Wait for pump");
		}
		windowMutex->Wait();
		MOVE_CURSOR(0, idx * block_height + CUSTOMER_STATUS_POSITION);
		cout << "---------------------------------------------\n";
		cout << "Name:                      " << records[idx].name << "          " << "\n";
		cout << "Credit Card Number:        " << records[idx].creditCardNumber << "          " << "\n";
		cout << "Fuel Grade:                " << fuelGradeToString(records[idx].grade) << "          " << "\n";
		cout << "Unit Cost ($/L):           " << records[idx].unitCost << "          " << "\n";
		cout << "Requested Volume (L):      " << records[idx].requestedVolume << "          " << "\n";
		cout << "Status:                    " << customers[idx]->getStatus() << "          " << "\n";
		cout << "Pump ID:                   " << records[idx].pumpId << "          " << "\n";
		cout << "---------------------------------------------\n";
		cout << "\n";
		fflush(stdout);
		windowMutex->Signal();
		prev_records[idx] = records[idx];
		prev_statuses[idx] = customers[idx]->getStatus();
	}
}

void
runPumpFacility()
{
	windowMutex->Wait();
	MOVE_CURSOR(0, 0);
	cout << "--------------------------------------------------------------------------------" << endl;
	cout << "                          Gas Station Fuel Pump Status                          " << endl;
	cout << "--------------------------------------------------------------------------------" << endl;
	windowMutex->Signal();

	for (int i = 0; i < NUM_PUMPS; i++) {
		pumpDataOps.emplace_back(make_unique<PumpData>(i));
		readPumpThreads.emplace_back(make_unique<CThread>(readPump, ACTIVE, &threadIds[i]));
	}

	CThread printCustomersThread(printCustomers, ACTIVE, NULL);

	/**
	 * It is absolutely necessary to ensure that the pumpDataOps are created, initialized and ready to
	 * read the pump before generating any customer. This is to avoid race condition where custmers
	 * write pipes while pumpDataOps are not ready to read the pipe, which can lead to inexplicable outputs
	 * (e.g., eight customers are waiting for auth at the same time at the beginning).
	 */
	rendezvous->Wait();

	for (int i = 0; i < NUM_CUSTOMERS; i++) {
		customers.emplace_back(make_unique<Customer>());
		customers[i]->Resume();
	}

	for (const auto& t : readPumpThreads) {
		t->WaitForThread();
	}
	printCustomersThread.WaitForThread();
	cout << "Press Enter to terminate the Customer process." << endl;
	waitForKeyPress();
}