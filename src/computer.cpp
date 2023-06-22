#include "computer.h"
#include "pump_data.h"

/**
 * For the variables that are used in the function definitions, they should be
 * declared inside the implementation file.
 */

list<CustomerRecord> txnList;
unique_ptr<CMutex> txnListMutex;
TxnListPrinter txnPrinter(txnList);

vector<unique_ptr<CDataPool>> tankDp;
vector<unique_ptr<TankData>> tankDpData;
vector<unique_ptr<CMutex>> tankDpMutex;

unique_ptr<CMutex> windowMutex = make_unique<CMutex>("ComputerWindowMutex");

vector<float> tankReadingsPercent(NUM_TANKS, 0.0f);
vector<float> tankReadings(NUM_TANKS, 0.0f);

unique_ptr<CTypedPipe<Cmd>> attendentPipe;

vector<unique_ptr<CThread>> readPumpThreads;
vector<int> threadIds = { 0, 1, 2, 3 };
vector<unique_ptr<PumpData>> pumpDataOps;
vector<unique_ptr<CThread>> readTankThreads;


/***********************************************
 *                                             *
 *                Transactions                 *
 *                                             *
 ***********************************************/

vector<unique_ptr<CThread>> transactionThreads;
unique_ptr<CTypedPipe<CustomerRecord>> customerPipe = make_unique<CTypedPipe<CustomerRecord>>("CustomerPipe", 1);
unique_ptr<CMutex> customerPipeMutex = make_unique<CMutex>("CustomerPipeMutex");

TxnListPrinter::TxnListPrinter(list<CustomerRecord>& lst) : lst(&lst)
{
	mutex = make_unique<CMutex>("TransactionListMutex");
}

void
TxnListPrinter::printNew()
{
	static int count = 0;
	static size_t last_size = 0;
	const int offset = 11;
	
	//mutex->Wait();
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
	//mutex->Signal();
	txnListMutex->Signal();
}

void
setupComputer()
{
	for (int i = 0; i < NUM_TANKS; i++) {
		tankDpMutex.emplace_back(make_unique<CMutex>(getName("FuelTankMutex", i, "")));
		tankDp.emplace_back(make_unique<CDataPool>(getName("FuelTankDataPool", i, ""), sizeof(TankData)));
		tankDpData.emplace_back(static_cast<TankData*>(tankDp[i]->LinkDataPool()));
		// Make read tank threads active at creation time can avoid UI being garbled.
		readTankThreads.emplace_back(make_unique<CThread>(readTank, ACTIVE, &threadIds[i]));
	}

	for (int i = 0; i < NUM_PUMPS; i++) {
		pumpDataOps.emplace_back(make_unique<PumpData>(i));
		readPumpThreads.emplace_back(make_unique<CThread>(readPump, ACTIVE, &threadIds[i]));
	}

	txnListMutex = make_unique<CMutex>("TransactionListMutex");
	windowMutex = make_unique<CMutex>("ComputerWindowMutex");
	attendentPipe = make_unique<CTypedPipe<Cmd>>("AttendentPipe", 1);
	customerPipe = make_unique<CTypedPipe<CustomerRecord>>("CustomerPipe", 1);

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

UINT __stdcall
recordTxn(void* args)
{
	CustomerRecord txn;
	while (true) {
		customerPipe->Read(&txn);

		txnListMutex->Wait();
		txnList.push_back(txn);
		txnListMutex->Signal();
		
	}
	return 0;
}

void
printTxn(const CustomerRecord& record, int position, int txn_id)
{
	windowMutex->Wait();
	MOVE_CURSOR(0, position);
	//cout << "update count: " << ++j << endl;
	cout << "--------------- Pump " << record.pumpId << " Transaction " << txn_id  << " --------------- \n";
	/*
	* For some reason, there are some residual characters on the DOS window that were printed from previous calls
	* of this function, leanding to some puzzling characters printed in the furture calls of this function
	* (e.g., waitoved, N/A 85, etc.).
	* To resolve this problem, we can print use empty string " " to overwrite those residual characters.
	*/
	if (record.name == "Unknown") {
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
	}
	cout << "----------------------------------------------------\n";
	cout << "\n";
	fflush(stdout);
	windowMutex->Signal();
}

void
writeTxnToPipe(const unique_ptr<PumpData>& pump_data_ptr)
{

	CustomerRecord txn = pump_data_ptr->getData();

	if (txn.txnStatus == TxnStatus::Done) {
		txn.txnStatus = TxnStatus::Archived;
		pump_data_ptr->archiveData();

		customerPipeMutex->Wait();
		customerPipe->Write(&txn);
		customerPipeMutex->Signal();
	}
}

UINT __stdcall
readPump(void* args)
{
	int id = *(int*)(args);
	assert(id >= 0 && id <= 3);

	pumpDataOps[id]->printPumpStatus(pumpDataOps[id]->getData());

	while (true) {

		pumpDataOps[id]->readData();

		writeTxnToPipe(pumpDataOps[id]);

		pumpDataOps[id]->printPumpData();

		//SLEEP(REFRESH_RATE);
	}
	return 0;
}

UINT __stdcall
printTxnHistory(void* args)
{
	Cmd cmd = Cmd::Invalid;

	windowMutex->Wait();
	MOVE_CURSOR(0, TXN_LIST_POSITION - 3);
	cout << "--------------------------------------------------------------------------------" << endl;
	cout << "                           Transaction History                                  " << endl;
	cout << "--------------------------------------------------------------------------------" << endl;
	windowMutex->Signal();

	while (true) {
		attendentPipe->Read(&cmd);

		if (cmd == Cmd::PrintTxn) {
			//cout << "Print txn command received!" << endl;
			txnPrinter.printNew();
			
		}

		SLEEP(5000);
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

	while (true) {
		tankDpMutex[tank_id]->Wait();
		tank_data = *tankDpData[tank_id];
		tankDpMutex[tank_id]->Signal();

		if (tankReadings[tank_id] != tank_data.remainingVolume) {
			tankReadings[tank_id] = tank_data.remainingVolume;

			windowMutex->Wait();
			// Move the cursor to the appropriate location on the screen
			MOVE_CURSOR(0, TANK_UI_POSITION + tank_id);
			tankReadingsPercent[tank_id] = tankReadings[tank_id] / TANK_CAPACITY * 100;
			// Calculate the length of the bar based on the fuel level
			int bar_length = (int)(tankReadings[tank_id] / TANK_CAPACITY * maxBarLength);

			// Draw the bar
			cout << "Tank " << tank_id << " (" << fuelGradeToString(static_cast<FuelGrade>(tank_data.fuelGrade)) << "): [";
			for (int i = 0; i < bar_length; i++) {
				cout << barChar;
			}
			for (int i = bar_length; i < maxBarLength; i++) {
				cout << " ";
			}

			cout << "] " << tankReadingsPercent[tank_id] << "% " << "(" << tankReadings[tank_id] << " Liters)" << endl;
			fflush(stdout);
			windowMutex->Signal();
		}
	}
	return 0;
}