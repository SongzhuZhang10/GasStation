#ifndef __COMMAND_PROCESSOR_H__
#define __COMMAND_PROCESSOR_H__

#include <string>
#include <map>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <set>
#include "attendent.h"

class CommandProcessor {
private:
    // Map from commands to functions
    std::map<std::string, std::function<void(int)>> command_map_int;
    std::map<std::string, std::function<void()>> command_map_void;

    std::set<std::string> commands_with_int;

    // Condition variable and mutexes for synchronization
    std::condition_variable cv;

    // `commandMutex` is used to ensure that only one command is processed at a time.
    std::mutex commandMutex, outputMutex;
    bool commandCompleted = true; // No command running at start

    std::unique_ptr<Attendent> attendent;

public:
    CommandProcessor();
    void openPump(int n);
    void printTxn();
    void refillTank(int n);
    void run();
};


#endif // !__COMMAND_PROCESSOR_H__
