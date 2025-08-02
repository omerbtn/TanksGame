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
        simulator::ArgumentsParser parser(argc, argv);
        simulator::Simulator simulator(parser.getConfig());
        simulator.run();
        return 0;
    }
    catch (const simulator::SimulatorException& e)
    {
        std::cerr << "Simulator error: " << e.what() << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Unexpected error." << std::endl;
    }

    return 1;
}
