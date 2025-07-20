#include <dlfcn.h>
#include <filesystem>
#include <iostream>
#include <vector>

#include "arguments_parser.h"
#include "simulator.h"
#include "simulator_exception.h"


int main(int argc, char* argv[])
{
    try
    {
        ArgumentsParser parser(argc, argv);
        Simulator simulator(parser.getConfig());
        simulator.run();
    }
    catch (const SimulatorException& e)
    {
        std::cerr << "Simulator error: " << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unexpected error." << std::endl;
        return 1;
    }

    return 0;
}
