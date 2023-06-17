#include "pump_data.h"

PumpData::PumpData(int id) : _id(id)
{
	windowMutex = make_unique<CMutex>("ComputerWindowMutex");

	dp = make_unique<CDataPool>(getName("PumpDataPool", _id, ""), sizeof(CustomerRecord));
	// Must use the reset function to avoid the error E0349 `no operator "=" matches these operands 
	dpData.reset(static_cast<CustomerRecord*>(dp->LinkDataPool()));
#if 0
	// producer consumer arrangement where pump is the producer
	pumpPs = make_unique<CSemaphore>(getName("PumpProducerPs", _id, ""), 0, 1);
	pumpCs = make_unique<CSemaphore>(getName("PumpProducerCs", _id, ""), 1, 1);
#endif

	// producer consumer arrangement where computer is the producer
	comPs = make_unique<CSemaphore>(getName("ComProducerPs", _id, ""), 0, 1);
	comCs = make_unique<CSemaphore>(getName("ComProducerCs", _id, ""), 1, 1);

	mutex = make_unique<CMutex>(getName("PumpDataPoolMutex", _id, ""));
	assert(data.txnStatus == TxnStatus::Pending && prev_data.txnStatus == TxnStatus::Pending);

}

void
PumpData::printDpData()
{
	mutex->Wait();
	windowMutex->Wait();
	MOVE_CURSOR(0, CMD_POSITION + _id * 12);
	cout << "--------------- DP Data Contents ---------------" << endl;
	cout << "Name:                  " << dpData->name << endl;
	cout << "Credit Card Number:    " << dpData->creditCardNumber << endl;
	cout << "Grade:                 " << fuelGradeToString(dpData->grade) << endl;
	cout << "Requested Volume:      " << dpData->requestedVolume << endl;
	cout << "Received Volume:       " << dpData->receivedVolume << endl;
	cout << "Cost:                  " << dpData->cost << endl;
	cout << "Pump ID:               " << dpData->pumpId << endl;
	cout << "---------------------------------------------\n";
	cout << "\n";
	fflush(stdout);
	windowMutex->Signal();
	mutex->Signal();
}

void
PumpData::readData()
{
	mutex->Wait();
	data = *dpData;

#if 0
	windowMutex->Wait();
	//cout << "Auth status: " << txnStatusToString(dpData->txnStatus) << endl;
	if (data.name != dpData->name)
		cout << "BUG: name = " << dpData->name;
	if (data.creditCardNumber != dpData->creditCardNumber)
		cout << "BUG: creditCardNumber = " << dpData->creditCardNumber;
	if (dpData->grade != data.grade)
		cout << "BUG: fuel grade = " << fuelGradeToString(dpData->grade) << endl;
	if (dpData->pumpId != data.pumpId)
		cout << "BUG: pumpId = " << dpData->pumpId << endl;
	if (dpData->requestedVolume != data.requestedVolume)
		cout << "BUG: requestedVolume = " << dpData->requestedVolume << endl;
	if (data.txnStatus != dpData->txnStatus)
		cout << "BUG: txnStatus = " << txnStatusToString(dpData->txnStatus) << endl;
	if (data.receivedVolume != dpData->receivedVolume)
		cout << "BUG: receivedVolume = " << dpData->receivedVolume << endl;
	if (sizeof(*dpData) != sizeof(data)) {
		cout << "BUG: Size of data = " << sizeof(data) << endl;
		cout << "BUG: Size of dp data = " << sizeof(sizeof(*dpData)) << endl;
	}
	windowMutex->Signal();
#endif
	assert(data == *dpData);
	mutex->Signal();
}

void
PumpData::writeData()
{
	mutex->Wait();
	*dpData = data;
	mutex->Signal();
}

void
PumpData::archiveData()
{
	mutex->Wait();
	data.txnStatus = TxnStatus::Archived;
	dpData->txnStatus = data.txnStatus;
	//*dpData = data;
	mutex->Signal();
}

CustomerRecord
PumpData::getData() const
{
	return data;
}

void
PumpData::printPumpData()
{
	if (prev_data == data) {
#if 0
		windowMutex->Wait();
		MOVE_CURSOR(0, DEBUG_1 + _id * 10);
		cout << "DEBUG: data.name = " << data.name << endl;
		cout << "DEBUG: data.txnStatus = " << txnStatusToString(data.txnStatus) << endl;
		cout << "DEBUG: data.creditCardNumber = " << data.creditCardNumber << endl;
		cout << "DEBUG: prev_data.name = " << prev_data.name << endl;
		cout << "DEBUG: prev_data.txnStatus = " << txnStatusToString(prev_data.txnStatus) << endl;
		cout << "DEBUG: ptrv_data.creditCardNumber = " << prev_data.creditCardNumber << endl;
		cout << "return count: " << ++i << endl;
		assert(data.name == prev_data.name);
		fflush(stdout);
		windowMutex->Signal();
#endif
		return;
	}
	else {
		// Must use mutex to protect `data` so that it is not written while being read. 
		mutex->Wait();
		prev_data = data;
		assert(data == prev_data);
		mutex->Signal();

		printCustomer(prev_data);
	}
}

void
PumpData::printCustomer(const CustomerRecord& record) const
{
	windowMutex->Wait();
	MOVE_CURSOR(0, PUMP_STATUS_POSITION + _id * 12);
	//cout << "update count: " << ++j << endl;
	cout << "--------------- Pump " << _id << " Status ---------------\n";
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
		cout << "Name:                      " << record.name << "          " << "\n";
		cout << "Credit Card Number:        " << record.creditCardNumber << "          " << "\n";
		cout << "Fuel Grade:                " << fuelGradeToString(record.grade) << "          " << "\n";
		cout << "Unit Cost ($/L):           " << record.unitCost << "          " << "\n";
		cout << "Requested Volume (L):      " << record.requestedVolume << "          " << "\n";
		cout << "Received Volume (L):       " << record.receivedVolume << "          " << "\n";
		cout << "Total Cost ($):            " << record.cost << "          " << "\n";
		cout << "Transaction Status:        " << txnStatusToString(record.txnStatus) << "          " << "\n";
	}
	cout << "---------------------------------------------\n";
	cout << "\n";
	fflush(stdout);
	windowMutex->Signal();
	SLEEP(1500);
}
