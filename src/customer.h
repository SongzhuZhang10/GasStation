#ifndef __CUSTOMER_H__
#define __CUSTOMER_H__

#include "rt.h"
#include "common.h"
#include "fuel_price.h"
#include <random>

class Customer : public ActiveClass {
private:
	enum class CustomerStatus
	{
		WaitForPump,
		ArriveAtPump,
		SwipeCreditCard,
		RemoveGasHose,
		SelectFuelGrade,
		WaitForAuth,
		GetFuel,
		ReturnGasHose,
		DriveAway,
		Null
	};

	CustomerStatus status;

	int pumpId;

	unique_ptr<CMutex> pumpEnquiryMutex;

	CustomerRecord data;

	FuelPrice price;

	vector<unique_ptr<CMutex>> pumpStatusMutex;

	vector<unique_ptr<CEvent>> txnApproved;
	vector<unique_ptr<CDataPool>> pumpFlagDataPool;
	vector<unique_ptr<PumpStatus>> pumpStatuses;

	vector<unique_ptr<CTypedPipe<CustomerRecord>>> pipe;
	vector<unique_ptr<CMutex>> pipeMutex;

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
	void removeGasHose();
	void selectFuelGrade();
	void getFuel();
	void returnGasHose();
	void driveAway();
	string customerStatusToString(const CustomerStatus& status) const;
	// To trigger the this function, the declaration must be exactly
	// in this form, including the `void` keyword.
	int main(void); 

public:
	Customer();
	CustomerRecord& getData();
	string getStatus();

};


#endif // __CUSTOMER_H__