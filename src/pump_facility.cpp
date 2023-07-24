#include "pump_facility.h"
#include "command_processor.h"
#include <iomanip> // Required for std::setw()

using namespace std;

static FuelPrice fuelPrice;

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
 *                Command Processor            *
 *                                             *
 ***********************************************/
CommandProcessor cmdProcessor(fuelPrice, pumps);

UINT __stdcall
runCommandProcessor(void* args)
{
	cmdProcessor.run();
	return 0;
}

/***********************************************
 *                                             *
 *                Customers                    *
 *                                             *
 ***********************************************/

//vector<unique_ptr<Customer>> customers;
shared_ptr<CRendezvous> rndv = sharedResources.getRndv();

UINT __stdcall printCustomers(void* args)
{
	windowMutex->Wait();
	MOVE_CURSOR(0, 0);
	std::cout << "--------------------------------------------------------------------------------" << std::endl;
	std::cout << "                   Welcome to Gas Station Control Panel                         " << std::endl;
	std::cout << "--------------------------------------------------------------------------------" << std::endl;
	std::cout << "Supported commands:" << std::endl;

	// Set the width for the command and description columns
	const int commandWidth = 10;
	const int descriptionWidth = 25;

	// Set the left alignment for the columns
	std::cout << std::left << std::setw(commandWidth) << "- gc#:";
	std::cout << std::setw(descriptionWidth) << "Generate # number of customers (must less than "<< MAX_NUM_CUSTOMERS << ")" << std::endl;

	std::cout << std::left << std::setw(commandWidth) << "- op#";
	std::cout << std::setw(descriptionWidth) << "Authorize the transaction at pump # by opening that pump" << std::endl;

	std::cout << std::left << std::setw(commandWidth) << "- cpX Y:";
	std::cout << std::setw(descriptionWidth) << "Change the unit price of fuel grade X to the price Y" << std::endl;

	std::cout << std::left << std::setw(commandWidth) << "- pt:";
	std::cout << std::setw(descriptionWidth) << "Print transaction history" << std::endl;

	std::cout << std::left << std::setw(commandWidth) << "- rf#:";
	std::cout << std::setw(descriptionWidth) << "Refill tank # to full capacity" << std::endl;


	std::cout << "\n";
	

	std::cout << "--------------------------------------------------------------------------------" << std::endl;
	std::cout << "                           Customer Information                                 " << std::endl;
	std::cout << "--------------------------------------------------------------------------------" << std::endl;
	windowMutex->Signal();

	static size_t num_customers = 0;

	while (true) {
		num_customers = cmdProcessor.getCustomers().size();
		for (size_t i = 0; i < num_customers; ++i) {
			printCustomerRecord(i, cmdProcessor.getCustomers());
		}
	}
}


void
printCustomerRecord(int idx, vector<unique_ptr<Customer>>& customers)
{
	const int block_height = 13;
	static vector<CustomerRecord> prev_records(MAX_NUM_CUSTOMERS); // declare a vector with a size of `MAX_NUM_CUSTOMERS`
	static vector<string> prev_statuses(MAX_NUM_CUSTOMERS, "Null"); // declare a vector with a size of `MAX_NUM_CUSTOMERS`, initialized with value "Null"
	static vector<CustomerRecord> records(MAX_NUM_CUSTOMERS);

	records[idx] = customers[idx]->getData();

	if (customers[idx]->getStatusString() == "Null")
		return;

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
			TEXT_COLOUR(CYAN);
		}
		std::cout << "Status:                    " << customers[idx]->getStatusString() << "                        " << "\n";
		TEXT_COLOUR();
		if (records[idx].pumpId == -1) {
			std::cout << "Pump ID:                   Pending" << "                        " << "\n";
		}
		else {
			std::cout << "Pump ID:                   " << records[idx].pumpId << "                        " << "\n";
		}
		if (records[idx].nowTime.tm_year == 0) {
			std::cout << "Time:                                              " << "\n";
		}
		else {
			std::cout << "Time:                      ";
			printTimestamp(records[idx].nowTime);
		}

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
	 * It is absolutely necessary to ensure that the pumpController are created, initialized and ready to
	 * read the pump before generating any customer. This is to avoid race condition where custmers
	 * write pipes while pumpController are not ready to read the pipe, which can lead to inexplicable outputs
	 * (e.g., eight customers are waiting for auth at the same time at the beginning).
	 */
	rndv->Wait();

	CThread runCommandProcessorThread(runCommandProcessor, ACTIVE, NULL);

	printCustomersThread.WaitForThread();
	runCommandProcessorThread.WaitForThread();
	cout << "Press Enter to terminate the Customer process." << endl;
	waitForKeyPress();
}