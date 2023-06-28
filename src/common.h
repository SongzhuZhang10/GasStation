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

const int NUM_TANKS = 4;
const int NUM_PUMPS = 4;

const int NUM_CUSTOMERS = 8;

const float TANK_CAPACITY = 500.0f;
const float FLOW_RATE = 5.0f;
const float LOW_FUEL_VOLUME = 200.0f;

constexpr int TANK_UI_POSITION = 5;
constexpr int PUMP_STATUS_POSITION = TANK_UI_POSITION + 6;
constexpr int TXN_LIST_POSITION = PUMP_STATUS_POSITION + 50;

const int CUSTOMER_STATUS_POSITION = 4;

/*
	0 - Black
	1 - Dark Blue
	2 - Dark Green
	3 - Dark Cyan
	4 - Dark Red
	5 - Dark Magenta
	6 - Dark Yellow
	7 - Grey
	8 - Black(again)
	9 - Blue
	10 - Green
	11 - Cyan
	12 - Red
	13 - Magenta
	14 - Yellow
	15 - White
*/
const int BLACK = 0;
const int GREEN = 10;
const int CYAN = 11;
const int RED = 12;
const int WHITE = 15;

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
		name = "___Unknown___";
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
		name = "___Unknown___";
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
#if 0
	// Overloading assignment operator
	CustomerRecord& operator=(const CustomerRecord& other)
	{
		// Check for self-assignment
		if (this != &other) {
			// Deep copy
			name = other.name;
			creditCardNumber = other.creditCardNumber;
			grade = other.grade;
			requestedVolume = other.requestedVolume;
			receivedVolume = other.receivedVolume;
			unitCost = other.unitCost;
			cost = other.cost;
			pumpId = other.pumpId;
			txnStatus = other.txnStatus;
		}
		return *this;
	}
#endif
};

/***********************************************
 *                                             *
 *           Function Prototypes               *
 *                                             *
 ***********************************************/
std::string getName(const std::string& prefix, unsigned int id, const std::string& suffix);

std::string fuelGradeToString(FuelGrade grade);
int fuelGradeToInt(FuelGrade grade);
FuelGrade intToFuelGrade(int id);

void hitToContinue();

void waitForKeyPress();

void resizeToFit(std::string& destination, const std::string& source);

char digitToChar(int num);

void waitForKeys(char first_key, char second_key);

std::string txnStatusToString(TxnStatus status);

std::string waitForCmd();

template<typename T, typename... Args>
void createAndAdd(std::vector<std::shared_ptr<T>>& vec, Args&&... args) {
	vec.emplace_back(std::make_shared<T>(std::forward<Args>(args)...));
}


class SharedResources
{
private:
	shared_ptr<CMutex> pumpWindowMutex;
	shared_ptr<CMutex> computerWindowMutex;

	shared_ptr<CTypedPipe<Cmd>> attendentPipe;
	vector<shared_ptr<CTypedPipe<CustomerRecord>>> pumpPipes;

	shared_ptr<CRendezvous> rndv;
	vector<shared_ptr<CEvent>> txnApprovedEvents;

	vector<shared_ptr<CDataPool>> tankDps;
	vector<shared_ptr<TankData>> tankDpDataPtrs;
	vector<shared_ptr<CMutex>> tankDpDataMutexes;

	vector<shared_ptr<CDataPool>> pumpDps;
	vector<shared_ptr<CustomerRecord>> pumpDpDataPtrs;
	vector<shared_ptr<CReadersWritersMutex>> pumpDpDataMutexes;

	vector<shared_ptr<CSemaphore>> producers, consumers;

public:
	SharedResources() {
		pumpWindowMutex = make_shared<CMutex>("PumpScreenMutex");
		computerWindowMutex = make_shared<CMutex>("ComputerWindowMutex");
		rndv = make_shared<CRendezvous>("PumpRendezvous",
										NUM_PUMPS +
										// readTank threads of Computer
										NUM_TANKS +
										// main function thread of pump facility
										1);
		attendentPipe = make_shared<CTypedPipe<Cmd>>("AttendentPipe", 1);
		
		

		for (int i = 0; i < NUM_TANKS; i++) {
			tankDpDataMutexes.emplace_back(make_shared<CMutex>(getName("FuelTankDataPoolMutex", i, "")));
			tankDps.emplace_back(make_shared<CDataPool>(getName("FuelTankDataPool", i, ""), sizeof(TankData)));
			tankDpDataPtrs.emplace_back(static_cast<TankData*>(tankDps[i]->LinkDataPool()));
		}

		for (int i = 0; i < NUM_PUMPS; i++) {
			pumpDpDataMutexes.emplace_back(make_shared<CReadersWritersMutex>(getName("PumpDataPoolMutex", i, "")));
			pumpDps.emplace_back(make_shared<CDataPool>(getName("PumpDataPool", i, ""), sizeof(CustomerRecord)));
			pumpDpDataPtrs.emplace_back(static_cast<CustomerRecord*>(pumpDps[i]->LinkDataPool()));

			// semaphore with initial value 0 and max value 1
			producers.emplace_back(make_shared<CSemaphore>(getName("PS", i, ""), 0, 1));
			// semaphore with initial value 1 and max value 1
			consumers.emplace_back(make_shared<CSemaphore>(getName("CS", i, ""), 1, 1));

			pumpPipes.emplace_back(make_shared<CTypedPipe<CustomerRecord>>(getName("Pipe", i, ""), 1));

			txnApprovedEvents.emplace_back(make_shared<CEvent>(getName("TxnApprovedByPump", i, "")));
		}
	}

	auto getTankDpDataVec() const { return tankDpDataPtrs; }
	auto getTankDpDataMutexVec() const { return tankDpDataMutexes; }
	auto getPumpPipeVec() const { return pumpPipes; }
	auto getTxnApprovedEventVec() const { return txnApprovedEvents; }
	auto getPumpDataPooMutexlVec() const { return pumpDpDataMutexes; }
	auto getPumpDpDataPtrVec() const { return pumpDpDataPtrs; }

	/**
	 * In the context of multithreaded programming, returning by value (i.e., making a copy) ensures that
	 * the shared_ptr and the object it owns will remain valid for the caller, even if other threads are
	 * concurrently decreasing the reference count and might potentially delete the object.
	 * 
	 * Remove redundant const for return by value: For primitive types and pointers, const in the return
	 * type doesn't prevent modification of the copied value in the calling code, so it can be removed.
	 */
	shared_ptr<TankData> getTankDpDataPtr(int n) const { return tankDpDataPtrs[n]; }

	shared_ptr<CMutex> getTankDpMutex(int n) const { return tankDpDataMutexes[n]; }
	shared_ptr<CMutex> getPumpWindowMutex() const { return pumpWindowMutex; }
	shared_ptr<CMutex> getComputerWindowMutex() const { return computerWindowMutex; }

	
	shared_ptr<CRendezvous> getRndv() const { return rndv; }
	shared_ptr<CTypedPipe<Cmd>> getAttendentPipe() const { return attendentPipe; }

	auto getPumpDpDataMutex(int n) const { return pumpDpDataMutexes[n]; }
	shared_ptr<CustomerRecord> getPumpDpDataPtr(int n) const { return pumpDpDataPtrs[n]; }

	shared_ptr<CSemaphore> getProducer(int n) const { return producers[n]; }
	shared_ptr<CSemaphore> getConsumer(int n) const { return consumers[n]; }

	shared_ptr<CTypedPipe<CustomerRecord>> getPumpPipe(int n) const { return pumpPipes[n]; }

	shared_ptr<CEvent> getTxnApprovedEvent(int n) const { return txnApprovedEvents[n]; }
};

extern SharedResources sharedResources;

#endif // !__COMMON_H__
