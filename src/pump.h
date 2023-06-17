#ifndef __PUMP_H__
#define __PUMP_H__

#include "rt.h"
#include "fuel_tank.h"
#include "common.h"
#include "fuel_price.h"


class Pump : public ActiveClass 
{
private:
	int _id;
	string name;
	FuelPrice price;
	//float dispsensedFuel;
	unique_ptr<CDataPool> flagDataPool;
	unique_ptr<PumpStatus> pumpStatus;

	unique_ptr<CustomerRecord> data;
	unique_ptr<CDataPool> dataPool;
	// to protect data pointer pointing to the data in the data pool
	unique_ptr<CMutex> dpMutex;

	unique_ptr<CSemaphore> pumpPs; // pump is the producer
	unique_ptr<CSemaphore> pumpCs; // pump is the producer

	unique_ptr<CSemaphore> comPs; // computer is the producer
	unique_ptr<CSemaphore> comCs; // computer is the producer

	unique_ptr<CTypedPipe<CustomerRecord>> pipe;
	unique_ptr<CMutex> pipeMutex;

	// to protect `pumpStatus` resource
	unique_ptr<CMutex> pumpMutex;

	// to protect DOS window from being shared by multiple threads at the same time
	unique_ptr<CMutex> windowMutex;

	CustomerRecord customer;

	vector<unique_ptr<FuelTank>>& tanks_;

	// To create a class thread out of this function, the return value type must be `int`.
	void readPipe();
	void getFuel();
	void resetPump();
	void sendTransactionInfo();
	void waitForAuth();
	//int serviceCustomer(void* args);
	int main();
	
	FuelTank& getTank(int id);
public:
	Pump(int id, vector<unique_ptr<FuelTank>>& tanks);
	int getId();
};
#endif // __PUMP_H__