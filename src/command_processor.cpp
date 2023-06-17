#include <iostream>
#include <thread>
#include <sstream>
#include "command_processor.h"

CommandProcessor::CommandProcessor()
{
    command_map_int["OP"] = [this](int n) { this->openPump(n); };
    command_map_void["PT"] = [this]() { this->printTxn(); };

    commands_with_int.insert("OP");

    attendent = make_unique<Attendent>();
}

void CommandProcessor::openPump(int n)
{
    {
        std::lock_guard<std::mutex> lock(outputMutex); // Lock acquired
        // Critical section
        attendent->approveTxn(n);
        // Access and manipulate shared resources
        std::cout << "Opening pump " << n << " ..." << std::endl;
    } // Lock released here

    std::lock_guard<std::mutex> lock(m);
    commandCompleted = true;
    cv.notify_one();
}

void CommandProcessor::printTxn()
{
    {
        std::lock_guard<std::mutex> lock(outputMutex);
        attendent->printTxns();
        std::cout << "Printing transaction ..." << std::endl;
    }

    std::lock_guard<std::mutex> lock(m);
    commandCompleted = true;
    cv.notify_one();
}

void CommandProcessor::run()
{
    while (true) {
        std::string input, command;
        int number = 0;

        // Wait for the previous command to complete
        std::unique_lock<std::mutex> lock(m);

        /**
         * Make the main thread wait until commandCompleted becomes true.
         * The `&` in the lambda function's declaration is a capture clause
         * that allows the lambda function to access all in-scope variables
         * by reference. In this case, it allows the lambda function to access
         * `commandCompleted`.
         */
        cv.wait(lock, [&] { return commandCompleted; });

        commandCompleted = false; // Set to false before starting next command
        lock.unlock(); // This allows other threads to lock the mutex.

        {
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout << "Enter command and number: ";
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

            if (!(ss >> number)) {
                std::cout << "Please enter a command followed by an integer number.\n";
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
