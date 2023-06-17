#include "rt.h"
#include "common.h"
#include "pump_facility.h"

int main(void) {
	setupTanks();
	setupPumpFacility();

	cout << "Computer should be ready to start at this point." << endl;
	while (true) {}
	waitForKeyPress();
	return 0;
}