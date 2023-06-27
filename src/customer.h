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

	vector<shared_ptr<CEvent>> txnApprovedEvent;
	vector<shared_ptr<CDataPool>> pumpFlagDataPool;
	vector<shared_ptr<PumpStatus>> pumpStatuses;

	vector<shared_ptr<CTypedPipe<CustomerRecord>>> pipe;

	// to protect DOS window from being shared by multiple threads at the same time
	shared_ptr<CMutex> windowMutex;
	
	shared_ptr<CustomerRecord> pumpData;
	shared_ptr<CDataPool> pumpDataPool;
	// to protect data pointer pointing to the data in the pump data pool
	shared_ptr<CMutex> pumpDpMutex;

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
	string getStatusString();

};


#endif // __CUSTOMER_H__