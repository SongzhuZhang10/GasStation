#ifndef __PUMP_DATA_H__
#define __PUMP_DATA_H__

#include "rt.h"
#include "common.h"


class PumpData
{
private:
	int _id;

	// No need to initialize this variable because it is
	// initialized by its contructor when it is declared.
	CustomerRecord data;
	CustomerRecord prev_data;

	unique_ptr<CMutex> mutex;
	unique_ptr<CMutex> windowMutex;

	// pumps are the producer
	unique_ptr<CSemaphore> pumpPs;
	unique_ptr<CSemaphore> pumpCs;

	// computer is the producer
	unique_ptr<CSemaphore> comPs;
	unique_ptr<CSemaphore> comCs;

	unique_ptr<CDataPool> dp;
	unique_ptr<CustomerRecord> dpData;	

public:
	PumpData(int id);
	void printPumpData();
	void printCustomer(const CustomerRecord& record) const;
	void printDpData();
	void readData();
	void writeData();
	void archiveData();
	CustomerRecord getData() const;
};


#endif // __PUMP_DATA_H__

