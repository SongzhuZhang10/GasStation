#ifndef __PUMP_FACILITY_H__
#define __PUMP_FACILITY_H__

#include "rt.h"
#include "common.h"
#include "fuel_tank.h"
#include "pump.h"

#include "customer.h"
#include <string>
#include <vector>
#include "pump_controller.h"

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


void printCustomerRecord(int idx, std::vector<std::unique_ptr<Customer>>& customers);


UINT __stdcall runCommandProcessor(void* args);

void runPumpFacility();

#endif // __PUMP_FACILITY_H__