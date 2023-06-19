#include "customer.h"
#include <string>
#include <vector>


constexpr int NUM_CUSTOMERS = 2;
unique_ptr<CMutex> windowMutex = make_unique<CMutex>("CustomerWindowMutex");
vector<unique_ptr<Customer>> customers;
UINT __stdcall printCustomers(void* args);
void printCustomerRecord(const unique_ptr<Customer>& customer, int idx);

UINT __stdcall printCustomers(void* args)
{
	//int id = *(int*)(args);

	while (true) {
		for (size_t i = 0; i < customers.size(); ++i) {
			printCustomerRecord(customers[i], i);
		}
		SLEEP(2500);
	}
}

#if 1
void printCustomerRecord(const unique_ptr<Customer>& customer, int idx)
{ 
	const CustomerRecord record = customer->getData();
	const int block_height = 10;

	static CustomerRecord prev_record;
	static string prev_status = "Null";
	if (prev_record == record && prev_status == customer->getStatus()) {
		return;
	}
	else {
		windowMutex->Wait();
		MOVE_CURSOR(0, idx * block_height);
		cout << "------------------------------------------------\n";
		cout << "Name:                      " << record.name                     << "          " << "\n";
		cout << "Credit Card Number:        " << record.creditCardNumber         << "          " << "\n";
		cout << "Fuel Grade:                " << fuelGradeToString(record.grade) << "          " << "\n";
		cout << "Unit Cost ($/L):           " << record.unitCost                 << "          " << "\n";
		cout << "Requested Volume (L):      " << record.requestedVolume          << "          " << "\n";
		cout << "Status:                    " << customer->getStatus()           << "          " << "\n";
		cout << "------------------------------------------------\n";
		cout << "\n";
		fflush(stdout);
		windowMutex->Signal();
		prev_record = record;
		prev_status = customer->getStatus();
	}
}

#endif

int
main(void)
{
	for (int i = 0; i < NUM_CUSTOMERS; i++) {
		customers.emplace_back(make_unique<Customer>());
	}
	for (int i = 0; i < NUM_CUSTOMERS; i++) {
		customers[i]->Resume();
	}

	CThread printCustomersThread(printCustomers, ACTIVE, NULL);
	printCustomersThread.WaitForThread();
	cout << "Press Enter to terminate the Customer process." << endl;
	waitForKeyPress();
	return 0;
}

