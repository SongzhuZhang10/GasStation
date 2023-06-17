#include "rt.h"
#include "common.h"

int
main(void)
{
#if 1
	CProcess gas_station_facility("D:\\OneDrive\\GasStation\\GasStation\\Debug\\Computer.exe",
									NORMAL_PRIORITY_CLASS, OWN_WINDOW, ACTIVE);
	SLEEP(1000);
	CProcess gas_station_computer("D:\\OneDrive\\GasStation\\GasStation\\Debug\\PumpFacility.exe",
									NORMAL_PRIORITY_CLASS, OWN_WINDOW, ACTIVE);
	SLEEP(1000);
	CProcess customer_generator("D:\\OneDrive\\GasStation\\GasStation\\Debug\\CustomerGenerator.exe",
									NORMAL_PRIORITY_CLASS, OWN_WINDOW, ACTIVE);
	SLEEP(1000);


	cout << "Waiting for GSC process to terminate ..." << endl;
	gas_station_computer.WaitForProcess();
	
	cout << "Waiting for gas station facility process to terminate ..." << endl;
	gas_station_facility.WaitForProcess();

	cout << "Waiting for customer generator process to terminate ..." << endl;
	customer_generator.WaitForProcess();
#endif
	pressEnterToContinue();
	return 0;
}