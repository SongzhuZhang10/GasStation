#ifndef __CUSTOMER_H__
#define __CUSTOMER_H__

#include "rt.h"
#include "common.h"
#include "fuel_price.h"
#include <random>

class Customer : public ActiveClass {
private:
	int pumpId;

	CustomerRecord data;

	FuelPrice price;

	vector<unique_ptr<CDataPool>> pumpFlagDataPool;
	vector<unique_ptr<PumpStatus>> pumpStatuses;

	vector<unique_ptr<CTypedPipe<CustomerRecord>>> pipe;
	vector<unique_ptr<CMutex>> pipeMutex;

	// protect the data pool containing the `pumpStatus` variable of the pump
	vector<unique_ptr<CMutex>> pumpMutex;

	// to protect DOS window from being shared by multiple threads at the same time
	unique_ptr<CMutex> windowMutex;

	string getRandomName();
	string getRandomCreditCardNumber();
	FuelGrade getRandomFuelGrade();
	float getRandomFloat(float min, float max);
	void writePipe(CustomerRecord* customer);
	int getAvailPumpId();
	

	void arriveAtPump();
	void swipeCreditCard();
	void removeGasHoseFromPump();
	void selectFuelGrade();
	void getFuel();
	void returnHoseToPump();
	void driveAway();
	//void unlockPump();

	// To trigger the this function, the declaration must be exactly in this form, including the `void`.
	int main(void); 

public:
	Customer();
	
};


#endif // __CUSTOMER_H__