#ifndef __ATTENDENT_H__
#define __ATTENDENT_H__

#include "rt.h"
#include "common.h"

class Attendent
{
private:
	vector<unique_ptr<CMutex>> pumpMutex;
	vector<unique_ptr<CDataPool>> pumpDp;
	vector<unique_ptr<CustomerRecord>> pumpDpData;

	vector<unique_ptr<CEvent>> txnApproved;

	unique_ptr<CTypedPipe<Cmd>> pipe;

	CustomerRecord pumpData[NUM_PUMPS];

	vector<unique_ptr<CMutex>> tankMutex;
	vector<unique_ptr<CDataPool>> tankDp;
	vector<unique_ptr<TankData>> tankDpData;

public:
	Attendent();
	bool approveTxn(int idx);
	void printTxns();
	
	bool addFuelToTank(int idx);
	void refillTank(int idx);
};
#endif // !__ATTENDENT_H__
