#include "runners/comparative_runner.h"

#include "arguments_parser.h"
#include "errors_logger.h"
#include "loader/shared_object_loader.h"
#include "registrations/game_manager_registrar.h"
#include "registrations/algorithm_registrar.h"
#include "simulator_exception.h"
#include "simulator.h"

namespace simulator
{
namespace runners
{

namespace fs = std::filesystem;
using namespace UserCommon_322573304_322647603;

ComparativeRunner::ComparativeRunner(SimulatorConfig config) : Runner(std::move(config)) {}

ComparativeRunner::~ComparativeRunner() {
    registrations::AlgorithmRegistrar::getAlgorithmRegistrar().clear();
    registrations::GameManagerRegistrar::getGameManagerRegistrar().clear();
}

void ComparativeRunner::loadSharedObjects() {
    // Load game managers from the specified folder
    size_t count = shared_object_loader_.loadSharedObjectsFromFolder<registrations::GameManagerRegistrar>(config_.game_managers_folder);
    if (count < 1)
    {
        std::ostringstream oss;
        ArgumentsParser::printUsage(oss, "At least one game manager must be registered for competition mode, "
                                         "didn't found any in " +
                                             config_.algorithms_folder);
        throw SimulatorException(oss.str());
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

void ComparativeRunner::prepare() {
    initOutputStream(file_, config_.game_managers_folder,
        std::string(config::get<std::string_view>("comparative_output_prefix")));

    printOutputHeader(file_, config_.mode);
}

void ComparativeRunner::run() {
    runComparativeGameManagers();
}

void ComparativeRunner::printResults() {
    printGroupedComparativeResults();
}

GameMapInfo ComparativeRunner::loadAndValidateMap() {
    GameMapInfo info = loadGameMap(config_.game_map_filename);
    if (!info.is_valid) {
        throw SimulatorException("Invalid game map: " + config_.game_map_filename);
    }
    return info;
}


auto ComparativeRunner::resolveAlgorithmFactories()
    -> std::pair<registrations::AlgorithmRegistrar::value_type, registrations::AlgorithmRegistrar::value_type>
{
    auto& registrar = registrations::AlgorithmRegistrar::getAlgorithmRegistrar();
    std::string name1 = fs::path(config_.algorithm1_so).stem().string();
    std::string name2 = fs::path(config_.algorithm2_so).stem().string();

    auto it1 = std::find_if(registrar.begin(), registrar.end(),
                            [&name1](const auto& e) { return e.name() == name1; });
    auto it2 = std::find_if(registrar.begin(), registrar.end(),
                            [&name2](const auto& e) { return e.name() == name2; });

    if (it1 == registrar.end() || it2 == registrar.end()) {
        throw SimulatorException("Algorithms were not registered properly");
    }

    return std::make_pair(*it1, *it2);
}

GameManagerExecutionResult ComparativeRunner::runSingleComparativeGame(
    const registrations::GameManagerRegistrar::value_type& gm_entry,
    const TankAlgorithmFactory& tank_factory1,
    const TankAlgorithmFactory& tank_factory2,
    const std::string& algo1_name,
    const std::string& algo2_name,
    const GameMapInfo& map_info,
    bool verbose)
{
    std::unique_ptr<AbstractGameManager> gm = gm_entry.createGameManager(verbose);

    auto& algo_registrar = registrations::AlgorithmRegistrar::getAlgorithmRegistrar();

    auto it1 = std::find_if(algo_registrar.begin(), algo_registrar.end(),
                            [&algo1_name](const auto& e) { return e.name() == algo1_name; });
    auto it2 = std::find_if(algo_registrar.begin(), algo_registrar.end(),
                            [&algo2_name](const auto& e) { return e.name() == algo2_name; });
    
    auto player1 = it1->createPlayer(1, map_info.width, map_info.height, map_info.max_steps, map_info.num_shells);
    auto player2 = it2->createPlayer(2, map_info.width, map_info.height, map_info.max_steps, map_info.num_shells);

    GameResult result = runSingleGame(
        *gm, *player1, *player2,
        tank_factory1, tank_factory2,
        algo1_name, algo2_name, map_info);

    return { gm_entry.name(), std::move(result) };
}


void ComparativeRunner::runComparativeGameManagers() {
    const GameMapInfo map_info = loadAndValidateMap();
    map_width_ = map_info.width;
    map_height_ = map_info.height;

    bool verbose = config_.verbose;
    auto [algo1, algo2] = resolveAlgorithmFactories();

    const auto& gm_registrar = registrations::GameManagerRegistrar::getGameManagerRegistrar();

    if (config_.num_threads <= 1) {
        for (const auto& gm_entry : gm_registrar) {
            results_.push_back(runSingleComparativeGame(
                gm_entry, algo1.getTankAlgorithmFactory(), algo2.getTankAlgorithmFactory(), algo1.name(), algo2.name(), map_info, verbose));
        }
    } else {
        ThreadPool thread_pool(config_.num_threads);
        std::vector<std::future<GameManagerExecutionResult>> futures;

        for (const auto& gm_entry : gm_registrar) {
            futures.push_back(thread_pool.enqueue([&, gm_entry]() {
                return runSingleComparativeGame(
                    gm_entry, algo1.getTankAlgorithmFactory(), algo2.getTankAlgorithmFactory(), algo1.name(), algo2.name(), map_info, verbose);
            }));
        }

        for (auto& fut : futures) {
            results_.push_back(fut.get());
        }
    }
}

void ComparativeRunner::printGroupedComparativeResults()
{
    // Group results by ComparableGameResult
    auto grouped = groupComparativeResults();

    // Sort the groups by size (descending)
    auto sorted_groups = sortComparativeGroups(std::move(grouped));

    // Print the grouped results
    bool first = true;
    for (const auto& [key, managers] : sorted_groups)
    {
        if (!first)
            file_ << '\n'; // Spacing between groups
        first = false;

        // Comma-separated GameManager names
        for (size_t i = 0; i < managers.size(); ++i)
        {
            if (i > 0)
                file_ << ", ";
            file_ << managers[i];
        }
        file_ << '\n';

        // Game result message
        file_ << resultToString(*key.result) << '\n';

        // Round number
        file_ << key.result->rounds << "\n";

        // Final board map
        file_ << key.final_state_str;
    }
}

std::unordered_map<ComparableGameResult, std::vector<std::string>>
ComparativeRunner::groupComparativeResults()
{
    std::unordered_map<ComparableGameResult, std::vector<std::string>> grouped;

    for (auto& r : results_)
    {
        ComparableGameResult key(std::move(r.result), map_width_, map_height_);
        grouped[key].push_back(r.manager_name);
    }

    return grouped;
}

std::vector<std::pair<ComparableGameResult, std::vector<std::string>>>
ComparativeRunner::sortComparativeGroups(std::unordered_map<ComparableGameResult, std::vector<std::string>>&& grouped)
{
    std::vector<std::pair<ComparableGameResult, std::vector<std::string>>> sorted{
        std::make_move_iterator(grouped.begin()),
        std::make_move_iterator(grouped.end())};

    // Sort by group size (descending)
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b)
              {
                  return a.second.size() > b.second.size();
              });

    return sorted;
}

} // namespace runners
} // namespace simulator