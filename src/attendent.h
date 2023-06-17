#ifndef __ATTENDENT_H__
#define __ATTENDENT_H__

#include "rt.h"
#include "common.h"

class Attendent
{
private:
	vector<unique_ptr<CMutex>> mutex;
	vector<unique_ptr<CDataPool>> dp;
	vector<unique_ptr<CustomerRecord>> dpData;

	vector<unique_ptr<CSemaphore>> comPs;
	vector<unique_ptr<CSemaphore>> comCs;

	unique_ptr<CTypedPipe<Cmd>> pipe;

	CustomerRecord data[NUM_PUMPS];
public:
	Attendent();
	bool approveTxn(int id);
	void printTxns();
};
#endif // !__ATTENDENT_H__
