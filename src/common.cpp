#include "common.h"

SharedResources sharedResources;

std::string
getName(const std::string& prefix, unsigned int id, const std::string& suffix)
{
    std::stringstream string_buffer;
    string_buffer << id;
    return prefix + string_buffer.str() + suffix;
}

std::string
fuelGradeToString(FuelGrade grade)
{
    switch (grade) {
    case FuelGrade::Oct87:
        return "Oct 87";
    case FuelGrade::Oct89:
        return "Oct 89";
    case FuelGrade::Oct91:
        return "Oct 91";
    case FuelGrade::Oct94:
        return "Oct 94";
    default:
        return "Invalid";
    }
}

int
fuelGradeToInt(FuelGrade grade)
{
    switch (grade) {
    case FuelGrade::Oct87:
        return 0;
    case FuelGrade::Oct89:
        return 1;
    case FuelGrade::Oct91:
        return 2;
    case FuelGrade::Oct94:
        return 3;
    default:
        return -1;
    }
}

FuelGrade
intToFuelGrade(int id)
{
    switch (id) {
    case 0:
        return FuelGrade::Oct87;
    case 1:
        return FuelGrade::Oct89;
    case 2:
        return FuelGrade::Oct91;
    case 3:
        return FuelGrade::Oct94;
    default:
        return FuelGrade::Invalid;
    }
}

void hitToContinue()
{
    bool c = false;
    std::cout << "Press c to continue..." << endl;
    // Start a separate thread to monitor keyboard input
    std::thread inputThread([&c]() {
        std::string input;
        while (!c) {
            // Read input from the user
            std::getline(std::cin, input);

            // Check if the user entered "exit"
            if (input == "c") {
                c = true;
                break;
            }
        }
        });

    // Monitor the exit condition in the main thread
    while (!c) {
        // Perform any other work or operations here
        // This loop will keep running until the user enters "exit"

        // Sleep for a short duration to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // Wait for the input thread to finish
    inputThread.join();
    std::cout << "Continue...\n";
}

void waitForKeyPress() {
    std::cin.get();
}



void
resizeToFit(std::string& destination, const std::string& source) {
    if (source.length() > destination.capacity()) {
        std::cout << "Resize the string <" << source << ">" << std::endl;
        destination.resize(source.length());
    }
    destination = source;
}

void
waitForKeys(char first_key, char second_key)
{
    std::string input;

    while (true) {
        std::getline(std::cin, input);
        if (input.size() >= 2 && input[input.size() - 2] == first_key && input[input.size() - 1] == second_key) {
            return;
        }
    }
}

std::string
waitForCmd()
{
    std::string input = "";
    std::getline(std::cin, input);
    return input;
}

char
digitToChar(int num)
{
    char character = '0' + num;
    return character;
}

std::string txnStatusToString(TxnStatus status)
{
    switch (status) {
    case TxnStatus::Approved:
        return "Approved";
    case TxnStatus::Disapproved:
        return "Disapproved";
    case TxnStatus::Pending:
        return "Wait";
    case TxnStatus::Done:
        return "Done";
    case TxnStatus::Archived:
        return "Archived";
    default:
        return "Invalid";
    }
}

