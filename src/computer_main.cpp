#include "computer.h"


int main(void) {
	MOVE_CURSOR(0, 0);
	SLEEP(5000);

	unique_ptr<CMutex> windowMutex = make_unique<CMutex>("ComputerWindowMutex");
	
	cout << "--------------------------------------------------------------------------------" << endl;
	cout << "                           Gas Station Computer (GSC)                           " << endl;
	cout << "--------------------------------------------------------------------------------" << endl;
	setupComputer();

	vector<int> threadIds = { 0, 1, 2, 3 };
	vector<unique_ptr<CThread>> readTankThreads;
	vector<unique_ptr<CThread>> readPumpThreads;
	vector<unique_ptr<CThread>> transactionThreads;

	for (int i = 0; i < NUM_TANKS; i++) {
		// Make read tank threads active at creation time can avoid UI being garbled.
		readTankThreads.emplace_back(make_unique<CThread>(readTank, ACTIVE, &threadIds[i]));
	}

	for (int i = 0; i < NUM_PUMPS; i++) {
		readPumpThreads.emplace_back(make_unique<CThread>(readPump, ACTIVE, &threadIds[i]));
	}

	SLEEP(1500);

	/* If you want to pass no arguments to the thread function by using NULL macro,
	 * then it's better to create CThread object directly rather than creating a CThread pointer.
	 * This is because, when creating CThread object via pointers, you cannot use the NULL macro
	 * as the third argument (or build process will fail.).
	 * You can use the NULL macro as the third argument only when the CThread object is created directly.
	 * Because the print txn thread depends on the pump- and tank-related threads, it should be activated
	 * after them!
	 */
	CThread printTxnHistoryThread(printTxnHistory, ACTIVE, NULL);
	
	for (const auto& t : readPumpThreads) {
		t->WaitForThread();
	}
	for (const auto& t : transactionThreads) {
		t->WaitForThread();
	}
	for (const auto& t : readTankThreads) {
		t->WaitForThread();
	}
	printTxnHistoryThread.WaitForThread();

	cout << "Press Enter to terminate the Computer process." << endl;
	waitForKeyPress();

	return 0;
}
