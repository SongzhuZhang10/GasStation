#ifndef __PUMP_FACILITY_H__
#define __PUMP_FACILITY_H__

#include "rt.h"
#include "common.h"
#include "fuel_tank.h"
#include "pump.h"

#include "customer.h"
#include <string>
#include <vector>
#include "pump_data.h"

/***********************************************
 *                                             *
 *                Tanks                        *
 *                                             *
 ***********************************************/
void setupTanks();




/***********************************************
 *                                             *
 *                Pumps                        *
 *                                             *
 ***********************************************/
void setupPumpFacility();




/***********************************************
 *                                             *
 *                Customers                    *
 *                                             *
 ***********************************************/
UINT __stdcall printCustomers(void* args);


void printCustomerRecord(int idx);




void runPumpFacility();

#endif // __PUMP_FACILITY_H__