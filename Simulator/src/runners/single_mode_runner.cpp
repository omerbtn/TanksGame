#include "runners/single_mode_runner.h"
#include "registrations/game_manager_registrar.h"
#include "registrations/algorithm_registrar.h"
#include "simulator_exception.h"
#include "simulator.h"
#include "loader/shared_object_loader.h"

namespace simulator
{
namespace runners
{

namespace fs = std::filesystem;
using namespace UserCommon_322573304_322647603;

SingleModeRunner::~SingleModeRunner() {
    registrations::AlgorithmRegistrar::getAlgorithmRegistrar().clear();
    registrations::GameManagerRegistrar::getGameManagerRegistrar().clear();
}

SingleModeRunner::SingleModeRunner(SimulatorConfig config) : Runner(std::move(config)) {}

void SingleModeRunner::loadSharedObjects() {
    // Load GameManager .so
    if (!shared_object_loader_.loadSharedObject<registrations::GameManagerRegistrar>(config_.game_manager_so))
    {
        throw SimulatorException("Failed to load game manager: " + config_.game_manager_so);
    }

    // Load algorithm1
    if (!shared_object_loader_.loadSharedObject<registrations::AlgorithmRegistrar>(config_.algorithm1_so))
    {
        throw SimulatorException("Failed to load algorithm1: " + config_.algorithm1_so);
    }

    // Load algorithm2, unless it's the same as algorithm1
    if (config_.algorithm2_so != config_.algorithm1_so &&
        !shared_object_loader_.loadSharedObject<registrations::AlgorithmRegistrar>(config_.algorithm2_so))
    {
        throw SimulatorException("Failed to load algorithm2: " + config_.algorithm2_so);
    }
}

void SingleModeRunner::run() {
    GameMapInfo map_info = loadGameMap(config_.game_map_filename);
    if (!map_info.is_valid)
    {
        throw SimulatorException("Invalid game map: " + config_.game_map_filename);
    }

    // Get GameManager entry
    const auto& game_manager_entry = *registrations::GameManagerRegistrar::getGameManagerRegistrar().begin();

    // Get algorithm entries from the registrar
    auto& algo_registrar = registrations::AlgorithmRegistrar::getAlgorithmRegistrar();
    auto it1 = std::find_if(algo_registrar.begin(), algo_registrar.end(),
                            [this](const auto& entry)
                            { return entry.name() == fs::path(config_.algorithm1_so).stem().string(); });
    auto it2 = std::find_if(algo_registrar.begin(), algo_registrar.end(),
                            [this](const auto& entry)
                            { return entry.name() == fs::path(config_.algorithm2_so).stem().string(); });

    // Get tank algorithm factories
    TankAlgorithmFactory tank_factory1 = it1->getTankAlgorithmFactory();
    TankAlgorithmFactory tank_factory2 = it2->getTankAlgorithmFactory();

    // Create GameManager and Players instances
    std::unique_ptr<AbstractGameManager> game_manager = game_manager_entry.createGameManager(config_.verbose);
    std::unique_ptr<Player> player1 = it1->createPlayer(1, map_info.width, map_info.height, map_info.max_steps, map_info.num_shells);
    std::unique_ptr<Player> player2 = it2->createPlayer(2, map_info.width, map_info.height, map_info.max_steps, map_info.num_shells);

    runSingleGame(*game_manager, *player1, *player2,
        tank_factory1, tank_factory2,
        it1->name(), it2->name(), map_info);
}

} // namespace runners
} // namespace simulator