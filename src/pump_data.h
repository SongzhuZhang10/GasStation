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

	unique_ptr<CDataPool> dp;
	unique_ptr<CustomerRecord> dpData;	

public:
	PumpData(int id);
	void printPumpData();
	void printPumpStatus(const CustomerRecord& record) const;
	void readData();
	void writeData();
	void archiveData();
	CustomerRecord getData() const;
};


#endif // __PUMP_DATA_H__

