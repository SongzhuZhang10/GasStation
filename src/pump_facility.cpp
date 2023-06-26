#include "pump_facility.h"

/**
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
shared_ptr<CMutex> windowMutex = sharedResources.getPumpWindowMutex();

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
shared_ptr<CRendezvous> rndv = sharedResources.getRndv();

UINT __stdcall printCustomers(void* args)
{
	windowMutex->Wait();
	MOVE_CURSOR(0, 0);
	std::cout << "--------------------------------------------------------------------------------" << std::endl;
	std::cout << "                           Customer Information                                 " << std::endl;
	std::cout << "--------------------------------------------------------------------------------" << std::endl;
	fflush(stdout);
	windowMutex->Signal();

	while (true) {
		for (size_t i = 0; i < customers.size(); ++i) {
			printCustomerRecord(i);
		}
	}
}


void
printCustomerRecord(int idx)
{
	const int block_height = 13;
	static vector<CustomerRecord> prev_records(NUM_CUSTOMERS); // declare a vector with a size of `NUM_CUSTOMERS`
	static vector<string> prev_statuses(NUM_CUSTOMERS, "Null"); // declare a vector with a size of `NUM_CUSTOMERS`, initialized with value "Null"
	static vector<CustomerRecord> records(NUM_CUSTOMERS);

	records[idx] = customers[idx]->getData();
#
	if (prev_records[idx] == records[idx] && prev_statuses[idx] == customers[idx]->getStatusString()) {
		return;
	}
	else {
		windowMutex->Wait();
		MOVE_CURSOR(0, idx * block_height + CUSTOMER_STATUS_POSITION);
		std::cout << "---------------------------------------------\n";
		std::cout << "Name:                      " << records[idx].name << "                        " << "\n";
		std::cout << "Credit Card Number:        " << records[idx].creditCardNumber << "                        " << "\n";
		std::cout << "Fuel Grade:                " << fuelGradeToString(records[idx].grade) << "                        " << "\n";
		std::cout << "Unit Cost ($/L):           " << records[idx].unitCost << "                        " << "\n";
		std::cout << "Requested Volume (L):      " << records[idx].requestedVolume << "                        " << "\n";
		std::cout << "Received Volume (L):       " << records[idx].receivedVolume << "                        " << "\n";
		std::cout << "Total Cost ($):            " << records[idx].cost << "                        " << "\n";
		if (customers[idx]->getStatusString() == "Wait for auth") {
			TEXT_COLOUR(11);
		}
		std::cout << "Status:                    " << customers[idx]->getStatusString() << "                        " << "\n";
		TEXT_COLOUR();
		std::cout << "Pump ID:                   " << records[idx].pumpId << "                        " << "\n";
		std::cout << "---------------------------------------------\n";
		std::cout << "\n";
		windowMutex->Signal();
		prev_records[idx] = records[idx];
		prev_statuses[idx] = customers[idx]->getStatusString();
	}
}

void
runPumpFacility()
{
	CThread printCustomersThread(printCustomers, ACTIVE, NULL);

	/**
	 * It is absolutely necessary to ensure that the pumpDataOps are created, initialized and ready to
	 * read the pump before generating any customer. This is to avoid race condition where custmers
	 * write pipes while pumpDataOps are not ready to read the pipe, which can lead to inexplicable outputs
	 * (e.g., eight customers are waiting for auth at the same time at the beginning).
	 */
	rndv->Wait();

	for (int i = 0; i < NUM_CUSTOMERS; i++) {
		customers.emplace_back(make_unique<Customer>());
		customers[i]->Resume();
	}

	
	printCustomersThread.WaitForThread();
	cout << "Press Enter to terminate the Customer process." << endl;
	waitForKeyPress();
}