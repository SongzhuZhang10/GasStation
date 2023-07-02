#ifndef __CUSTOMER_H__
#define __CUSTOMER_H__

#include "rt.h"
#include "common.h"
#include "fuel_price.h"
#include "pump.h"

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

	std::unique_ptr<CMutex> pumpEnquiryMutex;

	CustomerRecord data;

	FuelPrice& fuelPrice_;

	std::vector<std::shared_ptr<CEvent>> txnApprovedEvent;

	std::vector<std::unique_ptr<Pump>>& pumps_;

	std::vector<std::shared_ptr<CTypedPipe<CustomerRecord>>> pipe;

	// to protect DOS window from being shared by multiple threads at the same time
	std::shared_ptr<CMutex> windowMutex;

	// to protect data pointer pointing to the data in the pump data pool
	std::shared_ptr<CReadersWritersMutex> pumpDpMutex;

	std::string getRandomName();
	std::string getRandomCreditCardNumber();
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
	std::string customerStatusToString(const CustomerStatus& status) const;
	// To trigger the this function, the declaration must be exactly
	// in this form, including the `void` keyword.
	int main(void); 

public:
	Customer(std::vector<std::unique_ptr<Pump>>& pumps, FuelPrice& fuelPrice);
	CustomerRecord& getData();
	std::string getStatusString();

};


#endif // __CUSTOMER_H__