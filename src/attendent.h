#ifndef __ATTENDENT_H__
#define __ATTENDENT_H__

#include "rt.h"
#include "common.h"

class Attendent
{
private:
	std::vector<std::shared_ptr<CReadersWritersMutex>> pumpMutex;
	std::vector<std::shared_ptr<CustomerRecord>> pumpDpData;

	std::vector<std::shared_ptr<CEvent>> txnApprovedEvent;

	std::shared_ptr<CTypedPipe<Cmd>> pipe;

	CustomerRecord pumpData[NUM_PUMPS];

	std::vector<std::shared_ptr<CMutex>> tankMutex;
	std::vector<std::shared_ptr<TankData>> tankDpData;

public:
	Attendent();
	bool approveTxn(int idx);
	void printTxns();
	
	bool addFuelToTank(int idx);
	void refillTank(int idx);
};
#endif // !__ATTENDENT_H__
