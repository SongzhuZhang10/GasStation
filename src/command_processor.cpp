#include <iostream>
#include <thread>
#include <sstream>
#include "command_processor.h"

using namespace std;

CommandProcessor::CommandProcessor()
{
    /**
     * This line adds an entry to the map. The key is the string `"OP"`, and
     * the value is a lambda function that takes an integer `n` as an argument
     * and calls the `openPump(n)` method of the current object (`this`).
     * The [this] in the lambda function's declaration is a capture clause that
     * allows the lambda function to access the current object's member functions
     * and variables. The lambda function can then be used just like any other
     * function and passed as an argument to std::thread to run in a new thread.
     */
    command_map_int["OP"] = [this](int n) { this->openPump(n); };


    command_map_int["RF"] = [this](int n) { this->refillTank(n); };

    command_map_void["PT"] = [this]() { this->printTxn(); };
   
    commands_with_int.insert("OP");
    commands_with_int.insert("RF");

    attendent = make_unique<Attendent>();
}

void CommandProcessor::openPump(int n)
{
    {
        std::lock_guard<std::mutex> lock(outputMutex); // Lock acquired
        // Critical section
        std::cout << "Opening pump " << n << " ..." << std::endl;
        attendent->approveTxn(n); // execute a command
    } // Lock released here

    std::lock_guard<std::mutex> lock(commandMutex);
    commandCompleted = true;

    /**
     * This line notifies one thread that is waiting on the condition variable cv.
     * This is part of the coordination between the main thread and the worker threads.
     * When a worker thread finishes executing a command, it sets `commandCompleted` to
     * true and calls cv.notify_one() to wake up the main thread, which is waiting for
     * `commandCompleted` to become true.
     */
    cv.notify_one();
}

void CommandProcessor::printTxn()
{
    {
        // TODO: How does this work?
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cout << "Printing transaction ..." << std::endl;
        attendent->printTxns();
        
    }

    std::lock_guard<std::mutex> lock(commandMutex);
    commandCompleted = true;
    cv.notify_one();
}

void CommandProcessor::refillTank(int n)
{
    {
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cout << "Refilling the tank ..." << std::endl;
        attendent->refillTank(n);
    }

    std::lock_guard<std::mutex> lock(commandMutex);
    commandCompleted = true;
    cv.notify_one();
}

void CommandProcessor::run()
{
    while (true) {
        std::string input, command;
        int number = 0;

        /**
         * `lock` is an instance (a variable) of `std::unique_lock<std::mutex>`.
         * This lock automatically locks the mutex `commandMutex` when it is created
         * and unlocks it when it is destroyed. This is an example of RAII
         * (Resource Acquisition Is Initialization), a common idiom in C++ where resource
         * management (like locking and unlocking a mutex) is tied to the lifetime of an object..
         * Here, the `lock` is protecting the `commandCompleted` variable. Only one thread can
         * lock the mutex at a time.
         */
        std::unique_lock<std::mutex> lock(commandMutex); // Wait for the previous command to complete

        /**
         * Make the main thread wait until commandCompleted becomes true.
         * The `&` in the lambda function's declaration is a capture clause
         * that allows the lambda function to access all in-scope variables
         * by reference. In this case, it allows the lambda function to access
         * `commandCompleted`.
         * The cv.wait() function takes a unique lock and a predicate as
         * arguments, and it blocks the current thread until the condition
         * variable cv is notified.
         * Here, `lock` is the mutex that's being locked and unlocked around
         * the condition variable cv.
         * The `wait` function takes two arguments:
         *  1. The unique lock (`lock`) which must be locked by the current thread.
         *     This is required to ensure safe access to shared data.
         *  2. A lambda function `([&] { return commandCompleted; })` that represents
         *     the condition to be satisfied via the `cv` variable. This condition is
         *     checked each time the wait function is signaled. The thread will only
         *     proceed if the condition is satisfied; otherwise, it will go back to waiting.
         */
        cv.wait(lock, [&] { return commandCompleted; });

        commandCompleted = false; // Set to false before starting next command

        lock.unlock(); // This allows other threads to lock the mutex.

        {
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout << "[Command]: ";
        }

        std::getline(std::cin, input);

        // Check if the input string is too short
        if (input.size() < 2) {
            std::cout << "Please enter a valid command.\n";
            commandCompleted = true;
            continue;
        }

        /**
         * Split command and number from the input
         * Extract the first two (non-empty) characters from the `input` string as the command.
         * The `substr` function takes two arguments: the starting index and the
         * length of the substring. In this case, it starts from index 0 and takes 2 characters.
         */
        command = input.substr(0, 2);
        
        /**
         * Achieve case-insensitive command comparison by converting the command entered by the
         * user to uppercase before processing it.
         * This function applies `::toupper` to each character in the `command` string from
         * beginning to end, storing the result in command from the beginning, effectively
         * converting the entire string to uppercase.
         */
        std::transform(
            command.begin(),    // iterator that denotes the beginning of the string
            command.end(),      // iterator that denotes the end of the string
            // Overwrite the original characters in the string with their uppercase counterparts,
            // starting at the beginning of the `command` string
            command.begin(),    // iterator that tells this funciton where to store the result.
            /**
             * `::toupper` is a function that converts a given character to uppercase if it is a
             * lowercase letter; or, it returns the original character.
             * The double colon (`::`) is the scope resolution operator. It is used here because
             * `toupper` is in the global scope (not in a namespace or class).
             */
            ::toupper);

        // Check if the command exists in our command map
        if (commands_with_int.find(command) != commands_with_int.end()) {
            if (input.size() < 3) {
                std::cout << "This command requires a number.\n";
                commandCompleted = true;
                continue;
            }

            /**
             * Create a stringstream `ss` from the rest of the `input` string (starting from index 2,
             * which is the character after the command). The stringstream can then be used to extract
             * the number following the command.
             */
            std::stringstream ss(input.substr(2));

            /**
             * In `ss >> number`, the extraction operator (`>>`) is used to extract an integer from the
             * stringstream `ss` and store it in the variable `number`. This is similar to how you might
             * use `std::cin >> number` to read an integer from the console.
             * 
             * When you use the extraction operator with a stringstream, the operator tries to interpret
             * the sequence of characters stored in the stringstream `ss` as an integer and store it in
             * the integer variable `number`.
             * If it is able to do this, it returns a reference to the stringstream. This is considered
             * a true value, meaning that if you use it in a condition like if (ss >> number), the
             * condition will be true if the extraction was successful.
             */
            if (!(ss >> number)) {
                std::cout << "Please enter a command followed by an integer number.\n";
                commandCompleted = true;
                continue;
            }

            if (number < 0 || number > 3) {
                std::cout << "Please enter a number that is in the range of 0 to " << NUM_PUMPS - 1 << ".\n";
                commandCompleted = true;
                continue;
            }
        }

        // Check if exit command was given
        if (command == "EX") {
            break;
        }

        if (command_map_int.find(command) != command_map_int.end()) {
            std::thread t(command_map_int[command], number);
            t.detach();
        }
        else if (command_map_void.find(command) != command_map_void.end()) {
            std::thread t(command_map_void[command]);
            t.detach();
        }
        else {
            std::cout << "Invalid command entered\n";
            commandCompleted = true;
        }
    }
}
