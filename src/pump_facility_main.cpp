#include "rt.h"
#include "common.h"
#include "pump_facility.h"



int main(void) {

	setupTanks();

	setupPumpFacility();

	runPumpFacility();

	return 0;
}