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


    // ArgumentsParser parser(argc, argv);
    // SimulatorConfig config = parser.getConfig();

    // std::cout << "Run mode: "
    //           << (config.mode == RunMode::COMPARATIVE ? "Comparative" : "Competition") << std::endl;

    // std::cout << "Verbose: " << std::boolalpha << config.verbose << std::endl;
    // std::cout << "Number of threads: " << config.num_threads << std::endl;

    // if (config.mode == RunMode::COMPARATIVE)
    // {
    //     std::cout << "Game map: " << config.game_map_filename << std::endl;
    //     std::cout << "Game managers folder: " << config.game_managers_folder << std::endl;
    //     std::cout << "Algorithm 1: " << config.algorithm1_so << std::endl;
    //     std::cout << "Algorithm 2: " << config.algorithm2_so << std::endl;
    // }
    // else
    // {
    //     std::cout << "Game maps folder: " << config.game_maps_folder << std::endl;
    //     std::cout << "Game manager: " << config.game_manager_so << std::endl;
    //     std::cout << "Algorithms folder: " << config.algorithms_folder << std::endl;
    // }

    // (void)argc;
    // (void)argv;

    // std::cout << "You are in main!" << std::endl;

    // static constexpr auto so_files = {"GameManager/GameManager_098765432_123456789.so"};
    // std::vector<void*> handles;
    // auto& registrar = GameManagerRegistrar::getGameManagerRegistrar();

    // for (const auto& file : so_files)
    // {
    //     std::cout << "Loading shared object file: " << file << std::endl;
    //     std::string name = std::filesystem::path(file).stem().string();
    //     registrar.createGameManagerEntry(name);
    //     void* handle = dlopen(file, RTLD_LAZY | RTLD_GLOBAL);
    //     if (!handle)
    //     {
    //         std::cerr << "dlopen failed: " << dlerror() << std::endl;
    //         exit(1);
    //     }
    //     handles.push_back(handle);

    //     try
    //     {
    //         registrar.validateLastRegistration();
    //     }
    //     catch(const GameManagerRegistrar::BadRegistrationException& e)
    //     {
    //         std::cout << "---------------------------------" << std::endl;
    //         std::cout << "BadRegistrationException for: " << name << std::endl;
    //         std::cout << "Name as registered: " << e.name << std::endl;
    //         std::cout << "Has game manager factory? " << std::boolalpha << e.has_game_manager_factory << std::endl;
    //         std::cout << "---------------------------------" << std::endl;
    //         registrar.removeLast();
    //     }

    // }

    // for (const auto& manager : registrar)
    // {
    //     auto mgr = manager.createGameManager(false);
    //     std::cout << "GameManager: " << manager.name() << std::endl;
    // }

    // registrar.clear();

    // for (const auto handle : handles)
    // {
    //     std::cout << "Closing shared object file." << std::endl;
    //     dlclose(handle);
    // }

    return 0;
}
