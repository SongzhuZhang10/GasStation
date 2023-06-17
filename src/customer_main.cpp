#include "customer.h"

int
main(void)
{
	int num_customers = 5;
	vector<unique_ptr<Customer>> customers;
	for (int i = 0; i < num_customers; i++) {
		customers.emplace_back(make_unique<Customer>());
	}
	for (int i = 0; i < num_customers; i++) {
		customers[i]->Resume();
	}
	cout << "Press Enter to terminate the Customer process." << endl;
	waitForKeyPress();
	return 0;
}