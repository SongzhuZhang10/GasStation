#include "attendent.h"

Attendent::Attendent()
{
	for (int i = 0; i < NUM_PUMPS; i++) {
		pumpMutex.emplace_back(make_unique<CMutex>(getName("PumpDataPoolMutex", i, "")));
		pumpDp.emplace_back(make_unique<CDataPool>(getName("PumpDataPool", i, ""), sizeof(CustomerRecord)));
		pumpDpData.emplace_back(static_cast<CustomerRecord*>(pumpDp[i]->LinkDataPool()));
		txnApproved.emplace_back(make_unique<CEvent>(getName("TxnApprovedByPump", i, "")));
	}

	for (int i = 0; i < NUM_TANKS; i++) {
		tankMutex.emplace_back(make_unique<CMutex>(getName("FuelTankDataPoolMutex", i, "")));
		tankDp.emplace_back(make_unique<CDataPool>(getName("FuelTankDataPool", i, ""), sizeof(TankData)));
		tankDpData.emplace_back(static_cast<TankData*>(tankDp[i]->LinkDataPool()));
	}
	pipe = make_unique<CTypedPipe<Cmd>>("AttendentPipe", 1);
}

bool
Attendent::approveTxn(int idx)
{
	pumpMutex[idx]->Wait();
	pumpData[idx] = *pumpDpData[idx];
	pumpMutex[idx]->Signal();

	if (pumpData[idx].txnStatus == TxnStatus::Pending && pumpData[idx].name != "Unknown") {
		
		pumpMutex[idx]->Wait();
		pumpDpData[idx]->txnStatus = TxnStatus::Approved;
		assert(pumpDpData[idx]->txnStatus == TxnStatus::Approved);
		pumpMutex[idx]->Signal();
		
		txnApproved[idx]->Signal(); // Trigger the event in `waitForAuth` in `pump.cpp`

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