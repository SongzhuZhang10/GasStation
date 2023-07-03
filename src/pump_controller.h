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

	std::shared_ptr<CReadersWritersMutex> mutex;
	std::shared_ptr<CMutex> windowMutex;

	std::shared_ptr<CustomerRecord> dpData;

	std::shared_ptr<CSemaphore> producer, consumer;

public:
	PumpController(int id);
	void printPumpData();
	void printPumpStatus(const CustomerRecord& record) const;
	void readData();
	void archiveData();
	void addTimestamp();

	CustomerRecord getData() const;
};


#endif // __PUMP_DATA_H__

