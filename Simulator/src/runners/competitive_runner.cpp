#include "runners/competitive_runner.h"

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

CompetitiveRunner::~CompetitiveRunner() {
    registrations::AlgorithmRegistrar::getAlgorithmRegistrar().clear();
    registrations::GameManagerRegistrar::getGameManagerRegistrar().clear();
}

CompetitiveRunner::CompetitiveRunner(SimulatorConfig config) : Runner(std::move(config)) {}

void CompetitiveRunner::loadSharedObjects() {
    // Load GameManager .so
    if (!shared_object_loader_.loadSharedObject<registrations::GameManagerRegistrar>(config_.game_manager_so))
    {
        throw SimulatorException("Failed to load game manager: " + config_.game_manager_so);
    }

    // Load all algorithms from the specified folder
    size_t count = shared_object_loader_.loadSharedObjectsFromFolder<registrations::AlgorithmRegistrar>(config_.algorithms_folder);
    if (count < 2)
    {
        std::ostringstream oss;
        ArgumentsParser::printUsage(oss, "At least two algorithms must be registered for competition mode, "
                                         "found " +
                                             std::to_string(count) + " valid in " + config_.algorithms_folder);
        throw SimulatorException(oss.str());
    }
}

void CompetitiveRunner::prepare() {
    maps_ = loadGameMapsFromFolder(config_.game_maps_folder);

    // 3. Validate maps
    if (maps_.empty())
    {
        std::ostringstream oss;
        ArgumentsParser::printUsage(oss, "No valid game maps found in folder: " + config_.game_maps_folder);
        throw SimulatorException(oss.str());
    }

    // 4. Prepare output stream
    initOutputStream(file_, config_.algorithms_folder,
                     std::string(config::get<std::string_view>("competition_output_prefix")));

    // 5. Print the header
    printOutputHeader(file_, config_.mode);
}

void CompetitiveRunner::run() {
    runCompetitionGames();
}

void CompetitiveRunner::printResults() {
    printCompetitionResults();
}

std::vector<GameMapInfo> CompetitiveRunner::loadGameMapsFromFolder(const std::string& folder_path)
{
    // Check if the folder exists and is a directory
    if (!fs::exists(folder_path) || !fs::is_directory(folder_path))
    {
        std::ostringstream oss;
        ArgumentsParser::printUsage(oss, "Folder does not exist or is not a directory: " + folder_path);
        throw SimulatorException(oss.str());
    }

    std::vector<GameMapInfo> maps;

    // Iterate through all files in the folder and load valid game maps
    for (const auto& entry : fs::directory_iterator(folder_path))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".txt")
            continue;

        GameMapInfo map_info = loadGameMap(entry.path().string());
        if (map_info.is_valid)
        {
            maps.push_back(std::move(map_info));
        }
    }

    return maps;
}

void CompetitiveRunner::runCompetitionGames()
{
    // Initialize scores to zero for each algorithm
    auto& algo_registrar = registrations::AlgorithmRegistrar::getAlgorithmRegistrar();
    for (const auto& entry : algo_registrar)
    {
        scores_[entry.name()] = 0;
    }

    if (config_.num_threads <= 1)
    {
        // Single-threaded execution - run directly
        for (size_t k = 0; k < maps_.size(); ++k)
        {
            runCompetitionGamesForMapSingleThread(maps_[k], k);
        }
    }
    else
    {
        // Multi-threaded execution - use thread pool
        ThreadPool thread_pool(config_.num_threads);
        
        // Collect all futures and player pairs from all maps
        std::vector<std::future<GameResult>> all_futures;
        std::vector<std::pair<std::string, std::string>> all_player_pairs;

        // Submit all games from all maps to the thread pool
        for (size_t k = 0; k < maps_.size(); ++k)
        {
            submitCompetitionGamesForMap(maps_[k], k, thread_pool, all_futures, all_player_pairs);
        }

        // Collect all results and update scores
        for (size_t i = 0; i < all_futures.size(); ++i)
        {
            GameResult result = all_futures[i].get();
            const auto& [player1_name, player2_name] = all_player_pairs[i];
            updateScores(result, player1_name, player2_name);
        }
    }

}

void CompetitiveRunner::submitCompetitionGamesForMap(const GameMapInfo& map, size_t map_index,
                                             ThreadPool& thread_pool,
                                             std::vector<std::future<GameResult>>& all_futures,
                                             std::vector<std::pair<std::string, std::string>>& all_player_pairs)
{
    const auto& gm_registrar = registrations::GameManagerRegistrar::getGameManagerRegistrar();
    const auto& algo_registrar = registrations::AlgorithmRegistrar::getAlgorithmRegistrar();
    const auto& game_manager_entry = *gm_registrar.begin();
    const size_t N = algo_registrar.count();

    for (size_t i = 0; i < N; ++i) {
        size_t j = (i + 1 + (map_index % (N - 1))) % N;
        submitCompetitionMatch(thread_pool, map, algo_registrar, i, j, game_manager_entry, all_futures, all_player_pairs);
    }
}

void CompetitiveRunner::submitCompetitionMatch(ThreadPool& thread_pool,
                                    const GameMapInfo& map,
                                    const registrations::AlgorithmRegistrar& algo_registrar,
                                    size_t idx1, size_t idx2,
                                    const registrations::GameManagerRegistrar::value_type& game_manager_entry,
                                    std::vector<std::future<GameResult>>& all_futures,
                                    std::vector<std::pair<std::string, std::string>>& all_player_pairs)
{
    const auto& algo1_entry = algo_registrar.getEntry(idx1);
    const auto& algo2_entry = algo_registrar.getEntry(idx2);

    bool verbose = config_.verbose;
    std::string algo1_name = algo1_entry.name();
    std::string algo2_name = algo2_entry.name();
    TankAlgorithmFactory tank_factory1 = algo1_entry.getTankAlgorithmFactory();
    TankAlgorithmFactory tank_factory2 = algo2_entry.getTankAlgorithmFactory();

    all_player_pairs.emplace_back(algo1_name, algo2_name);

    all_futures.push_back(thread_pool.enqueue(
        [this, &game_manager_entry, &algo1_entry, &algo2_entry, &map,
         verbose, algo1_name, algo2_name, tank_factory1, tank_factory2]() -> GameResult {

            auto game_manager = game_manager_entry.createGameManager(verbose);
            auto player1 = algo1_entry.createPlayer(1, map.width, map.height, map.max_steps, map.num_shells);
            auto player2 = algo2_entry.createPlayer(2, map.width, map.height, map.max_steps, map.num_shells);

            return runSingleGame(*game_manager, *player1, *player2,
                                 tank_factory1, tank_factory2,
                                 algo1_name, algo2_name, map);
        }));
}

void CompetitiveRunner::runCompetitionGamesForMapSingleThread(const GameMapInfo& map, size_t map_index)
{
    auto& gm_registrar = registrations::GameManagerRegistrar::getGameManagerRegistrar();
    auto& algo_registrar = registrations::AlgorithmRegistrar::getAlgorithmRegistrar();

    const auto& game_manager_entry = *gm_registrar.begin();
    size_t N = algo_registrar.count();

    for (size_t i = 0; i < N; ++i)
    {
        // Get opponent index
        size_t j = (i + 1 + (map_index % (N - 1))) % N;

        // Get Algorithms entries
        const auto& algo1_entry = algo_registrar.getEntry(i);
        const auto& algo2_entry = algo_registrar.getEntry(j);

        // Create GameManager and Player instances
        std::unique_ptr<AbstractGameManager> game_manager = game_manager_entry.createGameManager(config_.verbose);
        std::unique_ptr<Player> player1 = algo1_entry.createPlayer(1, map.width, map.height, map.max_steps, map.num_shells);
        std::unique_ptr<Player> player2 = algo2_entry.createPlayer(2, map.width, map.height, map.max_steps, map.num_shells);

        // Run the game
        GameResult result = runSingleGame(*game_manager, *player1, *player2,
                                          algo1_entry.getTankAlgorithmFactory(),
                                          algo2_entry.getTankAlgorithmFactory(),
                                          algo1_entry.name(), algo2_entry.name(),
                                          map);

        // Update scores based on the result
        updateScores(result, algo1_entry.name(), algo2_entry.name());
    }
}

void CompetitiveRunner::updateScores(const GameResult& result,
                             const std::string& player1_name,
                             const std::string& player2_name)
{
    // Score by 3 points for a win, 1 point for tie, 0 points for loss
    if (result.winner == 1)
    {
        scores_[player1_name] += 3; // Player 1 wins
    }
    else if (result.winner == 2)
    {
        scores_[player2_name] += 3; // Player 2 wins
    }
    else
    {
        scores_[player1_name] += 1; // Tie
        scores_[player2_name] += 1;
    }
}

void CompetitiveRunner::printCompetitionResults()
{
    // Sort scores in descending order
    auto sorted_scores = sortCompetitionScores();

    for (const auto& [name, score] : sorted_scores)
    {
        file_ << name << " " << score << '\n';
    }
}

std::vector<std::pair<std::string, size_t>> CompetitiveRunner::sortCompetitionScores()
{
    std::vector<std::pair<std::string, size_t>> sorted{
        std::make_move_iterator(scores_.begin()),
        std::make_move_iterator(scores_.end())};

    // Sort by score (descending)
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b)
              {
                  return a.second > b.second;
              });

    return sorted;
}

} // namespace runners
} // namespace simulator