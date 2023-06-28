#include "computer.h"

int main(void) {

	setupComputer();

	/* If you want to pass no arguments to the thread function by using NULL macro,
	 * then it's better to create CThread object directly rather than creating a CThread pointer.
	 * This is because, when creating CThread object via pointers, you cannot use the NULL macro
	 * as the third argument (or build process will fail.).
	 * You can use the NULL macro as the third argument only when the CThread object is created directly.
	 * Because the print txn thread depends on the pump- and tank-related threads, it should be activated
	 * after them!
	 */
	CThread printTxnHistoryThread(printTxnHistory, ACTIVE, NULL);
	
	printTxnHistoryThread.WaitForThread();

	exitComputer();

	std::cout << "Press Enter to terminate the Computer process." << std::endl;
	waitForKeyPress();

	return 0;
}
