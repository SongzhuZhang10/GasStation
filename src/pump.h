#ifndef __PUMP_H__
#define __PUMP_H__

#include "rt.h"
#include "fuel_tank.h"
#include "common.h"
#include "fuel_price.h"


class Pump : public ActiveClass 
{
private:
	int id_;
	bool busy;
	string name;
	FuelPrice price;

	shared_ptr<CustomerRecord> data;
	// to protect data pointer pointing to the data in the pump data pool
	shared_ptr<CReadersWritersMutex> dpMutex;

	shared_ptr<CTypedPipe<CustomerRecord>> pipe;

	// to protect DOS window from being shared by multiple threads at the same time
	shared_ptr<CMutex> windowMutex;

	CustomerRecord customer;

	vector<unique_ptr<FuelTank>>& tanks_;

	shared_ptr<CEvent> txnApprovedEvent;

	shared_ptr<CRendezvous> rndv;

	shared_ptr<CSemaphore> producer, consumer;

	// To create a class thread out of this function, the return value type must be `int`.
	void readPipe();
	void getFuel();
	void resetPump();
	void sendTransactionInfo();
	void waitForAuth();
	void rendezvousOnce();
	int main();
	
	FuelTank& getTank(int id);

public:
	Pump(int id, vector<unique_ptr<FuelTank>>& tanks);
	void setBusy();
	bool isBusy();
	int getId();
	float getReceivedVolume();
	float getCost();
};
#endif // __PUMP_H__