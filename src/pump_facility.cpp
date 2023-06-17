#include "pump_facility.h"

/*
 * Plan: incorperate the four tanks inside the pump facility which is the top level.
 * The pump facality also includes the four pumps.
 * So, the main function of this project is the pump facility, which means the PumpFacility class is not needed.
 */

/***********************************************
 *                                             *
 *                Tanks                        *
 *                                             *
 ***********************************************/
vector<unique_ptr<FuelTank>> tanks;
unique_ptr<CMutex> windowMutex;

void
setupTanks()
{
	windowMutex = make_unique<CMutex>("PumpScreenMutex");
	for (int i = 0; i < NUM_TANKS; i++) {
		tanks.emplace_back(make_unique<FuelTank>(i));

		windowMutex->Wait();
		cout << "Tank " << i << " has been created." << endl;
		fflush(stdout);
		windowMutex->Signal();
	}
	for (int i = 0; i < NUM_TANKS; i++) {
		assert(fuelGradeToInt(tanks[i]->getFuelGrade()) == i);
	}
	windowMutex->Wait();
	cout << "All tanks have been activated." << endl;
	fflush(stdout);
	windowMutex->Signal();
}

/***********************************************
 *                                             *
 *                Pumps                        *
 *                                             *
 ***********************************************/
vector<unique_ptr<Pump>> pumps;

void
setupPumpFacility()
{
	for (int i = 0; i < NUM_PUMPS; i++) {
		pumps.emplace_back(make_unique<Pump>(i, tanks));
	}
	for (int i = 0; i < NUM_PUMPS; i++) {
		pumps[i]->Resume();
	}
	windowMutex->Wait();
	cout << "All pumps have been activated." << endl;
	fflush(stdout);
	windowMutex->Signal();
}