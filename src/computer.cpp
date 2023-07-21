#include "fuel_price.h"
#include "computer.h"
#include "pump_controller.h"

using namespace std;
/**
 * For the variables that are used in the function definitions, they should be
 * declared inside the implementation file.
 */

list<CustomerRecord> txnList;

unique_ptr<CMutex> txnListMutex = make_unique<CMutex>("TransactionListMutex");
TxnListPrinter txnPrinter(txnList);

vector<shared_ptr<TankData>> tankDpData;
vector<shared_ptr<CMutex>> tankDpMutex;

shared_ptr<CMutex> windowMutex = sharedResources.getComputerWindowMutex();

vector<float> tankReadingsPercent(NUM_TANKS, 0.0f);
vector<float> tankReadings(NUM_TANKS, 0.0f);

shared_ptr<CTypedPipe<Cmd>> attendentPipe;

vector<unique_ptr<CThread>> readPumpThreads;

vector<unique_ptr<PumpController>> pumpController;
vector<unique_ptr<CThread>> readTankThreads;

shared_ptr<CRendezvous> rndv = sharedResources.getRndv();

/***********************************************
 *                                             *
 *                Transactions                 *
 *                                             *
 ***********************************************/

vector<unique_ptr<CThread>> transactionThreads;

TxnListPrinter::TxnListPrinter(list<CustomerRecord>& lst) : lst(&lst) {}

void
TxnListPrinter::printNew()
{
	static int count = 0;
	static size_t last_size = 0;
	const int offset = 12;
	
	txnListMutex->Wait();
	if (lst->size() == 0)
		cout << "Cannot print txn because list size is 0." << endl;

	if (last_size < lst->size()) {
		auto it = lst->begin();
		advance(it, last_size);

		while (it != lst->end()) {
			printTxn(*it, count * offset + TXN_LIST_POSITION, count);
			++it;
			++count;
		}

		last_size = lst->size();
	}
	txnListMutex->Signal();
}

void
setupComputer()
{
	vector<int>& tankThreadIds = sharedResources.getTankThreadIds();
	vector<int>& pumpThreadIds = sharedResources.getPumpThreadIds();

	tankDpMutex = sharedResources.getTankDpDataMutexVec();
	tankDpData = sharedResources.getTankDpDataVec();

	for (int i = 0; i < NUM_TANKS; ++i) {
		// Make read tank threads active at creation time can avoid UI being garbled.
		// The vector `tankThreadIds` must be a global variable so that it does not get
		// destroyed when it goes out of scopt.
		readTankThreads.emplace_back(make_unique<CThread>(readTank, ACTIVE, &tankThreadIds[i]));
	}

	
	for (int i = 0; i < NUM_PUMPS; ++i) {
		pumpThreadIds.push_back(i);
	}

	for (int i = 0; i < NUM_PUMPS; ++i) {
		pumpController.emplace_back(make_unique<PumpController>(i));
		readPumpThreads.emplace_back(make_unique<CThread>(runPump, ACTIVE, &pumpThreadIds[i]));
	}

	attendentPipe = sharedResources.getAttendentPipe();

	windowMutex->Wait();
	MOVE_CURSOR(0, 0);
	cout << "--------------------------------------------------------------------------------" << endl;
	cout << "                           Gas Station Computer (GSC)                           " << endl;
	cout << "--------------------------------------------------------------------------------" << endl;
	windowMutex->Signal();
}

void exitComputer()
{
	for (const auto& t : transactionThreads) {
		t->WaitForThread();
	}
	for (const auto& t : readTankThreads) {
		t->WaitForThread();
	}
	for (const auto& t : readPumpThreads) {
		t->WaitForThread();
	}
}

void
printTxn(const CustomerRecord& record, int position, int txn_id)
{
	windowMutex->Wait();
	MOVE_CURSOR(0, position);
	cout << "--------------- Pump " << record.pumpId << " Transaction " << txn_id  << " --------------- \n";
	/*
	* For some reason, there are some residual characters on the DOS window that were printed from previous calls
	* of this function, leanding to some puzzling characters printed in the furture calls of this function
	* (e.g., waitoved, N/A 85, etc.).
	* To resolve this problem, we can print use empty string " " to overwrite those residual characters.
	*/
	if (record.name == "___Unknown___") {
		cout << "Name:                      " << "N/A             " << "\n";
		cout << "Credit Card Number:        " << "N/A             " << "\n";
		cout << "Fuel Grade:                " << "N/A             " << "\n";
		cout << "Unit Cost ($/L):           " << "N/A             " << "\n";
		cout << "Requested Volume (L):      " << "N/A             " << "\n";
		cout << "Received Volume (L):       " << "N/A             " << "\n";
		cout << "Total Cost ($):            " << "N/A             " << "\n";
		cout << "Transaction Status:        " << "N/A             " << "\n";
	}
	else {
		cout << "Name:                      " << record.name                         << "          " << "\n";
		cout << "Credit Card Number:        " << record.creditCardNumber             << "          " << "\n";
		cout << "Fuel Grade:                " << fuelGradeToString(record.grade)     << "          " << "\n";
		cout << "Unit Cost ($/L):           " << record.unitCost                     << "          " << "\n";
		cout << "Requested Volume (L):      " << record.requestedVolume              << "          " << "\n";
		cout << "Received Volume (L):       " << record.receivedVolume               << "          " << "\n";
		cout << "Total Cost ($):            " << record.cost                         << "          " << "\n";
		cout << "Transaction Status:        " << txnStatusToString(record.txnStatus) << "          " << "\n";
		cout << "Pump ID:                   " << record.pumpId                       << "          " << "\n";
		if (record.nowTime.tm_year == 0) {
			cout << "Time:                                              " << "\n";
		}
		else {
			cout << "Time:                      ";
			printTimestamp(record.nowTime);
		}
	}
	cout << "----------------------------------------------------\n";
	cout << "\n";
	windowMutex->Signal();
}

void
writeTxnToPipe(const unique_ptr<PumpController>& pump_ctrl)
{
	pump_ctrl->addTimestamp();

	CustomerRecord txn = pump_ctrl->getData();

	if (txn.txnStatus == TxnStatus::Done) {
		txn.txnStatus = TxnStatus::Archived;
		pump_ctrl->archiveData();

		txnListMutex->Wait();
		txnList.push_back(txn);
		txnListMutex->Signal();
	}
}

UINT __stdcall
runPump(void* args)
{
	int id = *(int*)(args);
	assert(id >= 0 && id <= NUM_PUMPS - 1);

	pumpController[id]->printPumpStatus(pumpController[id]->getData());

	while (true) {

		pumpController[id]->readData();

		writeTxnToPipe(pumpController[id]);

		pumpController[id]->printPumpData();

	}
	return 0;
}

UINT __stdcall
printTxnHistory(void* args)
{
	Cmd cmd = Cmd::Invalid;
	bool executedOnce = false;

	while (true) {
		attendentPipe->Read(&cmd);

		if (cmd == Cmd::PrintTxn) {
			if (!executedOnce) {
				windowMutex->Wait();
				MOVE_CURSOR(0, TXN_LIST_POSITION - 3);
				cout << "--------------------------------------------------------------------------------" << endl;
				cout << "                           Transaction History                                  " << endl;
				cout << "--------------------------------------------------------------------------------" << endl;
				windowMutex->Signal();
				executedOnce = true;
			}
			txnPrinter.printNew();
		}
	}
	return 0;
}

/*******************************************************************************************
 * In multi-tasking/mult-threaded programming, it is generally not reliable
 * to use if-else enclosing print statements as a method to print out debug info when
 * bug-triggering conditions are met by the `if` statement.
 * This is because the assembly code of the entire solution may be executed
 * out-of-order concurrently. Thus, when the bug-triggering is met, the thread
 * may be in a transient state; the debug info on a `false bug` may be printed
 * out in this situation.
 * For example, when `if (fuelGradeToInt(tank_data_temp.fuelGrade) != id)` is met,
 * info on false bugs is printed. By using only the `if` statement, the steady
 * state info on tanks are printed, which means the `if` statement was triggered
 * on a transient state and thus not helpful and is misleading.
 * In conclusion, do not use if-else enclosing print statements as a method to
 * print out debug info. We can just use the print statement without branch
 * conditionsfor debugging.
 *******************************************************************************************/
UINT __stdcall
readTank(void* args)
{
	int tank_id = *(int*)(args);
	assert(tank_id >= 0 && tank_id <= 3);
	TankData tank_data;
	
	static const int maxBarLength = 14;
	static const char barChar = '#';
	static bool flash_red = false;
	static bool toggle = true;
	rndv->Wait();

	while (true) {
		tankDpMutex[tank_id]->Wait();
		tank_data = *tankDpData[tank_id];
		tankDpMutex[tank_id]->Signal();

		if (tankReadings[tank_id] != tank_data.remainingVolume || tankReadings[tank_id] < LOW_FUEL_VOLUME) {
			tankReadings[tank_id] = tank_data.remainingVolume;

			
			tankReadingsPercent[tank_id] = tankReadings[tank_id] / TANK_CAPACITY * 100;
			// Calculate the length of the bar based on the fuel level
			int bar_length = (int)(tankReadings[tank_id] / TANK_CAPACITY * maxBarLength);
		
			if (tankReadings[tank_id] < LOW_FUEL_VOLUME) {
				flash_red = true;
			}
			else {
				flash_red = false;
			}

			windowMutex->Wait();
			MOVE_CURSOR(0, TANK_UI_POSITION + tank_id); // Move the cursor to the appropriate location on the screen
			if (tankReadingsPercent[tank_id] > 75) {
				TEXT_COLOUR(GREEN);
			}
			if (tankReadingsPercent[tank_id] <= 75 && tankReadings[tank_id] >= LOW_FUEL_VOLUME) {
				TEXT_COLOUR(YELLOW);
			}
			if (flash_red) {
				if (toggle) {
					TEXT_COLOUR(RED);
				}
				else {
					TEXT_COLOUR();
				}
				toggle = !toggle;
			}

			// Draw the bar
			cout << "Tank " << tank_id << " (" << fuelGradeToString(static_cast<FuelGrade>(tank_data.fuelGrade)) << "): [";
			for (int i = 0; i < bar_length; ++i) {
				cout << barChar;
			}
			for (int i = bar_length; i < maxBarLength; ++i) {
				cout << " ";
			}

			cout << "] " << tankReadingsPercent[tank_id] << "% " << "(" << tankReadings[tank_id] << " Liters)          " << endl;
			fflush(stdout);
			TEXT_COLOUR();
			windowMutex->Signal();
			SLEEP(500);
		}
	}
	return 0;
}