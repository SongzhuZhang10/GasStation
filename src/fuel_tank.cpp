#include "fuel_tank.h"
#include "common.h"

FuelTank::FuelTank(int id)
	:	_id(id)
{
	windowMutex = make_unique<CMutex>("PumpScreenMutex");

	mutex = make_unique<CMutex>(getName("FuelTankDataPoolMutex", _id, ""));
	dataPool = make_unique<CDataPool>(getName("FuelTankDataPool", _id, ""), sizeof(TankData));
	/*
	 * The reset() function releases the currently managed pointer (if any) and takes
	 * ownership of the new pointer. If you call reset() with a null pointer, the unique_ptr
	 * becomes empty and releases its ownership of any previously managed pointer.
	 */
	data.reset(static_cast<TankData*>(dataPool->LinkDataPool()));

	data->remainingVolume = TANK_CAPACITY; // All tanks are initially full.
	data->fuelGrade = intToFuelGrade(_id);
	fuelGrade = intToFuelGrade(_id);
}

float
FuelTank::readVolume() const
{
	mutex->Wait();
	float remaining_fuel = data->remainingVolume;
	mutex->Signal();
	return remaining_fuel;
}

bool
FuelTank::checkRemainingVolume()
{
	bool enough_fuel = false;
	if (this->readVolume() >= FLOW_RATE)
		enough_fuel = true;
	else
		cout << "No enough fuel in the tank." << endl;
	return enough_fuel;
}

void
FuelTank::refillTank()
{
	while (increment()) {
		cout << "Refilling ..." << endl;
	};
	cout << "Tank " << _id << " has been refilled." << endl;
}

bool
FuelTank::increment()
{
	bool keep_filling = false;
	mutex->Wait();
	if (data->remainingVolume + FLOW_RATE <= TANK_CAPACITY) {
		keep_filling = true;
		data->remainingVolume += FLOW_RATE;
	}
	mutex->Signal();
	SLEEP(1000);
	return keep_filling;
}

bool
FuelTank::decrement()
{
	bool keep_dispensing = false;
	mutex->Wait();
	if (data->remainingVolume - FLOW_RATE >= 0) {
		keep_dispensing = true;
		data->remainingVolume = data->remainingVolume - FLOW_RATE;
	}
	mutex->Signal();
	SLEEP(1000);
	return keep_dispensing;
}

FuelGrade
FuelTank::getFuelGrade()
{
	return fuelGrade;
}