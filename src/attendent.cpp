#include "attendent.h"

Attendent::Attendent()
{
	for (int i = 0; i < NUM_PUMPS; i++) {
		mutex.emplace_back(make_unique<CMutex>(getName("PumpDataPoolMutex", i, "")));
		dp.emplace_back(make_unique<CDataPool>(getName("PumpDataPool", i, ""), sizeof(CustomerRecord)));
		dpData.emplace_back(static_cast<CustomerRecord*>(dp[i]->LinkDataPool()));
		txnApproved.emplace_back(make_unique<CEvent>(getName("TxnApprovedByPump", i, "")));
	}
	pipe = make_unique<CTypedPipe<Cmd>>("AttendentPipe", 1);
}

bool
Attendent::approveTxn(int id)
{
	mutex[id]->Wait();
	data[id] = *dpData[id];
	mutex[id]->Signal();

	if (data[id].txnStatus == TxnStatus::Pending && data[id].name != "Unknown") {
		
		mutex[id]->Wait();
		dpData[id]->txnStatus = TxnStatus::Approved;
		assert(dpData[id]->txnStatus == TxnStatus::Approved);
		mutex[id]->Signal();
		
		txnApproved[id]->Signal(); // Trigger the event in `waitForAuth` in `pump.cpp`

		return true;
	}
	else {
		return false;
	}
}

void
Attendent::printTxns()
{ 
	static const Cmd command = Cmd::PrintTxn;
	pipe->Write(&command);
}