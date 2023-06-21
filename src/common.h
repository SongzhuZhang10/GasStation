#ifndef __COMMON_H__
#define __COMMON_H__

/*
 * We do not need to have a top level file where we create instances
 * of CProcess defined in the `rt` library to create process of the 
 * projects in the solution. This is because the projects' executables
 * are run automatically without creating CProcess instances for them when we set all
 * projects in the solution to be startup projects for debugging purposes (i.e., being able to
 * set break points in all projects).
 * In this case, if you insist on using CProcess to run the projects' executables, you will
 * see a duplicated DOS window for each project.
 * The dependencies between projects are handled by setting the properties
 * of the solution.
 */
#include <iostream>
#include <sstream>
#include <map>
#include <cstdlib>
#include <vector>
#include <memory>
#include <cassert>
#include <iomanip>
#include <string>
#include <cmath> // for std::fabs
#include <thread>
#include "rt.h"
#include <cassert>
#include <random>
#include <optional>
#include <unordered_map>

constexpr int NUM_TANKS = 4;
constexpr int NUM_PUMPS = 4;

constexpr int NUM_CUSTOMERS = 8;

constexpr float TANK_CAPACITY = 500.0f;
constexpr float FLOW_RATE = 5.0f;


constexpr unsigned int TANK_UI_POSITION = 5;
constexpr unsigned int CMD_POSITION = 10;
constexpr unsigned int PUMP_STATUS_POSITION = 4;
constexpr unsigned int CUSTOMER_STATUS_POSITION = 54;
constexpr unsigned int TXN_LIST_POSITION = 11;

//constexpr unsigned int tank_debug_info_position = 40;

constexpr unsigned int UI_HEIGHT = 10;

constexpr unsigned int REFRESH_RATE = 2000;



constexpr unsigned int DEBUG_1 = 140;
constexpr unsigned int DEBUG_2 = 180;

#define DEBUG_MODE 1

// You don't need operator overloading for comparing enum class instances.
enum class FuelGrade
{
	Oct87,
	Oct89,
	Oct91,
	Oct94,
	Invalid
};

enum class Cmd
{
	PrintTxn,
	Invalid
};
struct TankData
{
	// One cannot use float TANK_CAPACITY = 500.0f; to specify the default value
	// because it is not supported by Visual Studio and thus uneffective.
	float remainingVolume;
	FuelGrade fuelGrade;
};

enum class TxnStatus
{
	Approved,
	Disapproved,
	Pending,
	Done,
	Archived
};

struct CustomerRecord
{
	std::string name;
	std::string creditCardNumber;
	FuelGrade grade;
	float requestedVolume;
	float receivedVolume;
	float unitCost;
	float cost;
	int pumpId;
	TxnStatus txnStatus;
	// Default member initializer
	CustomerRecord() :
		// default value of creditCardNumber cannot exceeds 4-digits in string format.
		// Or, memcpy exception is thrown when trying to read/write this data.
		grade(FuelGrade::Invalid),
		requestedVolume(0.0f),
		receivedVolume(0.0f),
		unitCost(0.0f),
		cost(0.0f),
		pumpId(-1),
		txnStatus(TxnStatus::Pending)
	{
		// so far, the number of characters in any string cannot exceed 15. Or, the program fails.
		creditCardNumber = "0000 0000 0000";
		name = "Unknown";
	}

	void resetToDefault()
	{
		grade = FuelGrade::Invalid;
		requestedVolume = 0.0f;
		receivedVolume = 0.0f;
		unitCost = 0.0f;
		cost = 0.0f;
		pumpId = -1;
		txnStatus = TxnStatus::Pending;

		creditCardNumber = "0000 0000 0000";
		name = "Unknown";
	}

	// overload the `operator==` to provide a more natural way to compare two instances of the struct.
	bool operator==(const CustomerRecord& other) const
	{
		float epsilon = 0.00001f; // Define your own tolerance
		return	name == other.name &&
				creditCardNumber == other.creditCardNumber &&
				grade == other.grade &&
				std::fabs(requestedVolume - other.requestedVolume) < epsilon &&
				std::fabs(receivedVolume - other.receivedVolume) < epsilon &&
				std::fabs(unitCost - other.unitCost) < epsilon &&
				std::fabs(cost - other.cost) < epsilon &&
				pumpId == other.pumpId &&
				txnStatus == other.txnStatus;
	}

	bool operator!=(const CustomerRecord& other) const
	{
		float epsilon = 0.00001f; // Define your own tolerance
		return	name != other.name ||
				creditCardNumber != other.creditCardNumber ||
				grade != other.grade ||
				std::fabs(requestedVolume - other.requestedVolume) > epsilon ||
				std::fabs(receivedVolume - other.receivedVolume) > epsilon ||
				std::fabs(unitCost - other.unitCost) > epsilon ||
				std::fabs(cost - other.cost) > epsilon ||
				pumpId != other.pumpId ||
				txnStatus != other.txnStatus;
	}
};


struct PumpStatus
{
	// Because transaction being completed does not mean the pump is released by
	// the current customer and is ready to be used by the next customer and vice versa, we need
	// two bool flags.
	bool isTransactionCompleted;
	bool busy;
	PumpStatus() :
		busy(false),
		isTransactionCompleted(true) // indicating the previous transaction was completed
		{} 
		
};




std::string getName(const std::string& prefix, unsigned int id, const std::string& suffix);

std::string fuelGradeToString(FuelGrade grade);
int fuelGradeToInt(FuelGrade grade);
FuelGrade intToFuelGrade(int id);

void hitToContinue();

void waitForKeyPress();

void resizeToFit(std::string& destination, const std::string& source);

//void printPumpData(CustomerRecord& record, unique_ptr<CMutex>& window_mutex, int id);

char digitToChar(int num);

void waitForKeys(char first_key, char second_key);

std::string txnStatusToString(TxnStatus status);

std::string waitForCmd();

#endif // !__COMMON_H__
