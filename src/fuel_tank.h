#ifndef __FUEL_TANK_H__
#define __FUEL_TANK_H__

#include "rt.h"
#include "common.h"

/*
 * You should create multi-threaded related objects by using the
 * `new` operator. This is because creating those objects without
 * using the `new` operator is not supported by the `rt` library
 * (i.e., `rt.h` and `rt.cpp`).
 * My guess as to why it is not supported by the `rt` library is
 * that we want those objects to be created on the heap rather than
 * on the stack; otherwise, they will be destroyed when being used
 * across thread boundary or process boundary as they are out of scope.
 */

class FuelTank
{
private:
	/** The object that owns this object. */
	unique_ptr<CDataPool> dataPool;
	unique_ptr<TankData> data;
	unique_ptr<CMutex> mutex;
	unique_ptr<CMutex> windowMutex;
	int _id;
	FuelGrade fuelGrade;


public:
	FuelTank(int id);
	float readVolume() const;

	void refillTank();
	bool checkRemainingVolume();
	bool increment();
	bool decrement();
	FuelGrade getFuelGrade();
};
#endif // !__FUEL_TANK_H__