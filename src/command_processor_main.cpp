#include "command_processor.h"

int main() {
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << "                           User Command Interface                               " << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    CommandProcessor cp;
    cp.run();
    return 0;
}
