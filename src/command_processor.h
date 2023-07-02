#ifndef __COMMAND_PROCESSOR_H__
#define __COMMAND_PROCESSOR_H__

#include <string>
#include <memory>
#include <map>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <set>
#include "pump.h"
#include "customer.h"
#include "attendent.h"
#include "fuel_price.h"

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

class CommandProcessor {
private:
    // Map from commands to functions
    std::map<std::string, std::function<void(int)>> command_map_int;
    std::map<std::string, std::function<void()>> command_map_void;
    std::map<std::string, std::function<void(int, float)>> command_map_int_float;
    std::set<std::string> commands_with_int;
    std::set<std::string> commands_with_int_float;

    // Condition variable and mutexes for synchronization
    std::condition_variable cv;

    // `commandMutex` is used to ensure that only one command is processed at a time.
    std::mutex commandMutex, outputMutex;
    bool commandCompleted = true; // No command running at start

    std::unique_ptr<Attendent> attendent;

    FuelPrice& fuelPrice_;
    std::vector<std::unique_ptr<Pump>>& pumps_;

    std::vector<std::unique_ptr<Customer>> customers;

public:
    CommandProcessor(FuelPrice& fuelPrice, std::vector<std::unique_ptr<Pump>>& pumps);
    void openPump(int n);
    void changeUnitPrice(int grade, float price);
    void printTxn();
    void refillTank(int n);
    void generateCustomers(int n);
    std::vector<std::unique_ptr<Customer>>& getCustomers();
    void run();
};


#endif // !__COMMAND_PROCESSOR_H__
