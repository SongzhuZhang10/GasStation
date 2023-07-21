#include <iostream>
#include <thread>
#include <sstream>
#include "command_processor.h"

using namespace std;

#define DISPLAY_OUTPUT 0

CommandProcessor::CommandProcessor(FuelPrice& fuelPrice, vector<unique_ptr<Pump>>& pumps)
    : fuelPrice_(fuelPrice), pumps_(pumps)
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

    command_map_int["GC"] = [this](int n) { this->generateCustomers(n); };

    command_map_void["PT"] = [this]() { this->printTxn(); };

    command_map_int_float["CP"] = [this](int grade, float price) { this->changeUnitPrice(grade, price); };
   
    commands_with_int.insert("OP");
    commands_with_int.insert("RF");
    commands_with_int.insert("GC");

    commands_with_int_float.insert("CP");

    attendent = make_unique<Attendent>();

    for (int i = 0; i < MAX_NUM_CUSTOMERS; i++) {
        customers.emplace_back(make_unique<Customer>(pumps_, fuelPrice_));
    }
}

void
CommandProcessor::openPump(int n)
{

    {
#if DISPLAY_OUTPUT
        std::lock_guard<std::mutex> lock(outputMutex); // Lock acquired
        // Critical section
        std::cout << "Opening pump " << n << " ..." << std::endl;
#endif
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

void
CommandProcessor::generateCustomers(int n)
{
    static int i = 0;
    static int max_count = 0;
    {
#if DISPLAY_OUTPUT
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cout << "Generating " << n << " customers ..." << std::endl;
#endif
        max_count += n;
        for (; i < max_count; i++) {
            if (i > MAX_NUM_CUSTOMERS)
                cerr << "Error: Exceeded maximum number of customers" << endl;
            else
                customers[i]->Resume();
        }
    }

    std::lock_guard<std::mutex> lock(commandMutex);
    commandCompleted = true;
    cv.notify_one();
}

vector<unique_ptr<Customer>>&
CommandProcessor::getCustomers()
{
    return customers;
}

void
CommandProcessor::printTxn()
{
    {
#if DISPLAY_OUTPUT
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cout << "Printing transaction ..." << std::endl;
#endif
        attendent->printTxns();
    }

    std::lock_guard<std::mutex> lock(commandMutex);
    commandCompleted = true;
    cv.notify_one();
}

void
CommandProcessor::refillTank(int n)
{
    {
#if DISPLAY_OUTPUT
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cout << "Refilling the tank ..." << std::endl;
#endif
        attendent->refillTank(n);
    }

    std::lock_guard<std::mutex> lock(commandMutex);
    commandCompleted = true;
    cv.notify_one();
}

void
CommandProcessor::changeUnitPrice(int grade, float price)
{
    {
#if DISPLAY_OUTPUT
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cout << "Changing the unit price of " << fuelGradeToString(intToFuelGrade(grade)) << " to " << price << " per liter." << std::endl;
#endif
        fuelPrice_.setFuelPrice(intToFuelGrade(grade), price);
    }

    std::lock_guard<std::mutex> lock(commandMutex);
    commandCompleted = true;
    cv.notify_one();
}

void
CommandProcessor::run()
{
    while (true) {
        std::string input, command;
        int number = 0;

        int grade = -1;
        float price = 0.0f;
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
#if DISPLAY_OUTPUT
        {
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout << "[Command]: ";
        }
#endif

        char c;
        while (true) {
#ifdef _WIN32
            c = _getch();
#else
            struct termios old_t, new_t;
            tcgetattr(STDIN_FILENO, &old_t); // get current terminal info
            new_t = old_t;
            new_t.c_lflag &= ~ECHO; // disable echo
            tcsetattr(STDIN_FILENO, TCSANOW, &new_t); // apply new terminal settings
            c = getchar();
            tcsetattr(STDIN_FILENO, TCSANOW, &old_t); // restore old terminal settings
#endif

            if (c == '\n' || c == '\r') {
                std::cout << std::endl;
                break;
            }
            input.push_back(c);
        }

        // Check if the input string is too short
        if (input.size() < 2) {
#if DISPLAY_OUTPUT
            std::cout << "Invalid command size.\n";
#endif
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
#if DISPLAY_OUTPUT
                std::cout << "This command requires a number.\n";
#endif
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
#if DISPLAY_OUTPUT
                std::cout << "Command should be followed by an integer number.\n";
#endif
                commandCompleted = true;
                continue;
            }

            if (command != "GC" && (number < 0 || number > NUM_PUMPS - 1)) {
#if DISPLAY_OUTPUT
                std::cout << "Number must be the range of 0 to " << NUM_PUMPS - 1 << ".\n";
#endif
                commandCompleted = true;
                continue;
            }

            if (command == "GC" && number < 1) {
#if DISPLAY_OUTPUT
                std::cout << "At least one customer must be generated each time.\n";
#endif
                commandCompleted = true;
                continue;
            }
        }
        else if (commands_with_int_float.find(command) != commands_with_int_float.end()) {
            std::size_t found = input.find(' ');
            if (found == std::string::npos) {
#if DISPLAY_OUTPUT
                std::cout << "Please enter a command followed by an integer and a float number separated by a space.\n";
#endif
                commandCompleted = true;
                continue;
            }

            std::stringstream ss(input.substr(2));
            /**
             * `>> grade >> price`: This attempts to extract values from the stringstream into the variables `grade` and `price`.
             * The >> operator first tries to convert the next sequence of characters that could represent an int into an int
             * value and store it into `grade`. Then it does the same for a float value and stores it into `price`.
             * `if (!(ss >> grade >> price))`: This is checking whether the extraction operation was successful for both `grade`
             * and `price`. If the operation fails (for example, because the next sequence of characters can't be converted into
             * an int or a float, or because there aren't enough values left), then ss will be in a "fail" state, and when it's
             * used in a boolean context (like in an if statement), it will evaluate to false.
             */
            if (!(ss >> grade >> price)) {
#if DISPLAY_OUTPUT
                std::cout << "The command must be followed by an integer and a float number.\n";
                std::cout << "Grade = " << grade << "       Price = " << price << std::endl;
#endif
                commandCompleted = true;
                continue;
            }

            if (grade < 0 || grade > NUM_TANKS - 1) {
#if DISPLAY_OUTPUT
                std::cout << "Fuel grade must be an integer in the range of 0 to " << NUM_TANKS - 1 << std::endl;
#endif
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
        else if (command_map_int_float.find(command) != command_map_int_float.end()) {
            std::thread t(command_map_int_float[command], grade, price);
            t.detach();
        }

        else {
#if DISPLAY_OUTPUT
            std::cout << "Invalid command entered\n";
#endif
            commandCompleted = true;
        }
    }
}
