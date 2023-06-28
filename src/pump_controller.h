#ifndef __PUMP_DATA_H__
#define __PUMP_DATA_H__

#include "rt.h"
#include "common.h"


class PumpController
{
private:
	int id_;

	// No need to initialize this variable because it is
	// initialized by its contructor when it is declared.
	CustomerRecord data;
	CustomerRecord prev_data;

	shared_ptr<CMutex> mutex;
	shared_ptr<CMutex> windowMutex;

	shared_ptr<CustomerRecord> dpData;

	shared_ptr<CSemaphore> producer, consumer;

public:
	PumpController(int id);
	void printPumpData();
	void printPumpStatus(const CustomerRecord& record) const;
	void readData();
	void archiveData();
	CustomerRecord getData() const;
};


#endif // __PUMP_DATA_H__

