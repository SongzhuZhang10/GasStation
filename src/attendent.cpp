#include "attendent.h"

using namespace std;

Attendent::Attendent()
{
	pumpMutex = sharedResources.getPumpDataPooMutexlVec();
	pumpDpData = sharedResources.getPumpDpDataPtrVec();
	txnApprovedEvent = sharedResources.getTxnApprovedEventVec();

	tankMutex = sharedResources.getTankDpDataMutexVec();
	tankDpData = sharedResources.getTankDpDataVec();

	pipe = sharedResources.getAttendentPipe();
}

bool
Attendent::approveTxn(int idx)
{
	pumpMutex[idx]->WaitToRead();
	pumpData[idx] = *pumpDpData[idx];
	pumpMutex[idx]->DoneReading();

	if (pumpData[idx].txnStatus == TxnStatus::Pending && pumpData[idx].name != "___Unknown___") {
		
		pumpMutex[idx]->WaitToWrite();
		pumpDpData[idx]->txnStatus = TxnStatus::Approved;
		assert(pumpDpData[idx]->txnStatus == TxnStatus::Approved);
		pumpMutex[idx]->DoneWriting();
		
		txnApprovedEvent[idx]->Signal(); // Trigger the event in `waitForAuth` in `pump.cpp`

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

bool
Attendent::addFuelToTank(int idx)
{
	bool keep_filling = false;
	tankMutex[idx]->Wait();
	if (tankDpData[idx]->remainingVolume + FLOW_RATE <= TANK_CAPACITY) {
		keep_filling = true;
		tankDpData[idx]->remainingVolume += FLOW_RATE;
	}
	tankMutex[idx]->Signal();
	return keep_filling;
}

void
Attendent::refillTank(int idx)
{
	while (addFuelToTank(idx)) {
		SLEEP(1000);
	};
}