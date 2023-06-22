#include "pump_data.h"

PumpData::PumpData(int id) : _id(id)
{
	windowMutex = make_unique<CMutex>("ComputerWindowMutex");

	dp = make_unique<CDataPool>(getName("PumpDataPool", _id, ""), sizeof(CustomerRecord));
	// Must use the reset function to avoid the error E0349 `no operator "=" matches these operands 
	dpData.reset(static_cast<CustomerRecord*>(dp->LinkDataPool()));

	mutex = make_unique<CMutex>(getName("PumpDataPoolMutex", _id, ""));
	assert(data.txnStatus == TxnStatus::Pending && prev_data.txnStatus == TxnStatus::Pending);

}

void
PumpData::readData()
{
	mutex->Wait();
	data = *dpData;
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
	mutex->Signal();
}

CustomerRecord
PumpData::getData() const
{
	mutex->Wait();
	CustomerRecord temp = data;
	mutex->Signal();

	return temp;
}

void
PumpData::printPumpData()
{
	if (prev_data == data) {
		return;
	}
	else {
		// Must use mutex to protect `data` so that it is not written while being read. 
		mutex->Wait();
		prev_data = data;
		assert(data == prev_data);
		mutex->Signal();

		printPumpStatus(prev_data);
	}
}

void
PumpData::printPumpStatus(const CustomerRecord& record) const
{
	windowMutex->Wait();
	MOVE_CURSOR(0, PUMP_STATUS_POSITION + _id * 12);
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
