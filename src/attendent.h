#ifndef __ATTENDENT_H__
#define __ATTENDENT_H__

#include "rt.h"
#include "common.h"

class Attendent
{
private:
	vector<shared_ptr<CMutex>> pumpMutex;
	vector<shared_ptr<CustomerRecord>> pumpDpData;

	vector<shared_ptr<CEvent>> txnApprovedEvent;

	shared_ptr<CTypedPipe<Cmd>> pipe;

	CustomerRecord pumpData[NUM_PUMPS];

	vector<shared_ptr<CMutex>> tankMutex;
	vector<shared_ptr<TankData>> tankDpData;

public:
	Attendent();
	bool approveTxn(int idx);
	void printTxns();
	
	bool addFuelToTank(int idx);
	void refillTank(int idx);
};
#endif // !__ATTENDENT_H__
