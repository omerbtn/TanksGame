#include "simulator.h"

#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>

#include "arguments_parser.h"
#include "board_satellite_view.h"
#include "errors_logger.h"
#include "global_config.h"
#include "registrations/registrar_adapter.h"
#include "simulator_exception.h"
#include "utils.h"
#include "thread_pool.h"

namespace fs = std::filesystem;
using namespace UserCommon_322573304_322647603;


Simulator::Simulator(SimulatorConfig config) : config_(std::move(config)) {}

Simulator::~Simulator()
{
    AlgorithmRegistrar::getAlgorithmRegistrar().clear();
    GameManagerRegistrar::getGameManagerRegistrar().clear();

    for (auto handle : so_handles_)
    {
        dlclose(handle);
    }
}

void Simulator::run()
{
    if (config_.mode == RunMode::COMPARATIVE)
    {
        runComparative();
    }
    else if (config_.mode == RunMode::COMPETITION)
    {
        runCompetition();
    }
    else if (config_.mode == RunMode::SINGLE)
    {
        runSingle();
    }
}

/*======================================================================================*/

void Simulator::runComparative()
{
    // 1. Load game managers and algorithms .so files
    loadComparativeSharedObjects();

    // 2. Prepare output stream
    std::ofstream file_out;
    std::ostream& out = initOutputStream(file_out, config_.game_managers_folder,
                                         std::string(config::get<std::string_view>("comparative_output_prefix")));

    // 3. Print the header
    printOutputHeader(out, config_.mode);

    // 4. Run the game managers
    std::vector<GameManagerExecutionResult> results;
    size_t map_width, map_height;
    runComparativeGameManagers(results, map_width, map_height);

    // 5. Group and print results
    printGroupedComparativeResults(out, std::move(results), map_width, map_height);
}

void Simulator::loadComparativeSharedObjects()
{
    // Load game managers from the specified folder
    size_t count = loadSharedObjectsFromFolder<GameManagerRegistrar>(config_.game_managers_folder);
    if (count < 1)
    {
        std::ostringstream oss;
        ArgumentsParser::printUsage(oss, "At least one game manager must be registered for competition mode, "
                                         "didn't found any in " +
                                             config_.algorithms_folder);
        throw SimulatorException(oss.str());
    }

    // Load algorithm1
    if (!loadSharedObject<AlgorithmRegistrar>(config_.algorithm1_so))
    {
        throw SimulatorException("Failed to load algorithm1: " + config_.algorithm1_so);
    }

    // Load algorithm2, unless it's the same as algorithm1
    if (config_.algorithm2_so != config_.algorithm1_so &&
        !loadSharedObject<AlgorithmRegistrar>(config_.algorithm2_so))
    {
        throw SimulatorException("Failed to load algorithm2: " + config_.algorithm2_so);
    }
}

void Simulator::runComparativeGameManagers(std::vector<GameManagerExecutionResult>& results,
                                           size_t& map_width, size_t& map_height)
{
    // Load the map info
    const GameMapInfo map_info = loadGameMap(config_.game_map_filename);
    if (!map_info.is_valid)
    {
        throw SimulatorException("Invalid game map: " + config_.game_map_filename);
    }
    map_width = map_info.width;
    map_height = map_info.height;

    // Get algorithm entries from the registrar
    auto& algo_registrar = AlgorithmRegistrar::getAlgorithmRegistrar();
    auto it1 = std::find_if(algo_registrar.begin(), algo_registrar.end(),
                            [this](const auto& entry)
                            { return entry.name() == fs::path(config_.algorithm1_so).stem().string(); });
    auto it2 = std::find_if(algo_registrar.begin(), algo_registrar.end(),
                            [this](const auto& entry)
                            { return entry.name() == fs::path(config_.algorithm2_so).stem().string(); });

    // Shouldn't happen, we validate registrations in loading
    if (it1 == algo_registrar.end() || it2 == algo_registrar.end())
        throw SimulatorException("Algorithms were not registered properly");

    // Get tank algorithm factories
    TankAlgorithmFactory tank_factory1 = it1->getTankAlgorithmFactory();
    TankAlgorithmFactory tank_factory2 = it2->getTankAlgorithmFactory();
    
    bool verbose = config_.verbose;
    std::string algo1_name = it1->name();
    std::string algo2_name = it2->name();

    ThreadPool thread_pool(std::thread::hardware_concurrency());
    
    std::vector<std::future<GameManagerExecutionResult>> futures;
    
    // Iterate over all game managers and submit tasks to thread pool
    auto& gm_registrar = GameManagerRegistrar::getGameManagerRegistrar();
    for (const auto& gm_entry : gm_registrar)
    {
        auto future = thread_pool.enqueue([this, &gm_entry, &it1, &it2, &map_info, &tank_factory1, &tank_factory2, 
                                           verbose, algo1_name, algo2_name]() -> GameManagerExecutionResult {

            // Create GameManager and Players instances
            std::unique_ptr<AbstractGameManager> game_manager = gm_entry.createGameManager(verbose);
            std::unique_ptr<Player> player1 = it1->createPlayer(1, map_info.width, map_info.height, map_info.max_steps, map_info.num_shells);
            std::unique_ptr<Player> player2 = it2->createPlayer(2, map_info.width, map_info.height, map_info.max_steps, map_info.num_shells);

            // Run the game
            GameResult result = runSingleGame(*game_manager, *player1, *player2,
                                              tank_factory1, tank_factory2,
                                              algo1_name, algo2_name, map_info);

            return GameManagerExecutionResult{gm_entry.name(), std::move(result)};
        });
        
        futures.push_back(std::move(future));
    }
    
    // Collect all results
    for (auto& future : futures)
    {
        results.push_back(future.get());
    }
}

void Simulator::printGroupedComparativeResults(std::ostream& out,
                                               std::vector<GameManagerExecutionResult> results,
                                               size_t map_width, size_t map_height)
{
    // Group results by ComparableGameResult
    auto grouped = groupComparativeResults(std::move(results), map_width, map_height);

    // Sort the groups by size (descending)
    auto sorted_groups = sortComparativeGroups(std::move(grouped));

    // Print the grouped results
    bool first = true;
    for (const auto& [key, managers] : sorted_groups)
    {
        if (!first)
            out << '\n'; // Spacing between groups
        first = false;

        // Comma-separated GameManager names
        for (size_t i = 0; i < managers.size(); ++i)
        {
            if (i > 0)
                out << ", ";
            out << managers[i];
        }
        out << '\n';

        // Game result message
        out << resultToString(*key.result) << '\n';

        // Round number
        out << key.result->rounds << "\n";

        // Final board map
        out << key.final_state_str;
    }
}

std::unordered_map<ComparableGameResult, std::vector<std::string>>
Simulator::groupComparativeResults(std::vector<GameManagerExecutionResult> results,
                                   size_t map_width, size_t map_height)
{
    std::unordered_map<ComparableGameResult, std::vector<std::string>> grouped;

    for (auto& r : results)
    {
        ComparableGameResult key(std::move(r.result), map_width, map_height);
        grouped[key].push_back(r.manager_name);
    }

    return grouped;
}

std::vector<std::pair<ComparableGameResult, std::vector<std::string>>>
Simulator::sortComparativeGroups(std::unordered_map<ComparableGameResult, std::vector<std::string>> grouped)
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

/*======================================================================================*/

void Simulator::runCompetition()
{
    // 1. Load game managers and algorithms .so files
    loadCompetitionSharedObjects();

    // 2. Load all game maps from the specified folder
    std::vector<GameMapInfo> maps = loadGameMapsFromFolder(config_.game_maps_folder);

    // 3. Validate maps
    if (maps.empty())
    {
        std::ostringstream oss;
        ArgumentsParser::printUsage(oss, "No valid game maps found in folder: " + config_.game_maps_folder);
        throw SimulatorException(oss.str());
    }

    // 4. Prepare output stream
    std::ofstream file_out;
    std::ostream& out = initOutputStream(file_out, config_.algorithms_folder,
                                         std::string(config::get<std::string_view>("competition_output_prefix")));

    // 5. Print the header
    printOutputHeader(out, config_.mode);

    // 6. Run all competition games
    auto scores = runCompetitionGames(maps);

    // 7. Print the competition results
    printCompetitionResults(out, std::move(scores));
}

void Simulator::loadCompetitionSharedObjects()
{
    // Load GameManager .so
    if (!loadSharedObject<GameManagerRegistrar>(config_.game_manager_so))
    {
        throw SimulatorException("Failed to load game manager: " + config_.game_manager_so);
    }

    // Load all algorithms from the specified folder
    size_t count = loadSharedObjectsFromFolder<AlgorithmRegistrar>(config_.algorithms_folder);
    if (count < 2)
    {
        std::ostringstream oss;
        ArgumentsParser::printUsage(oss, "At least two algorithms must be registered for competition mode, "
                                         "found " +
                                             std::to_string(count) + " valid in " + config_.algorithms_folder);
        throw SimulatorException(oss.str());
    }
}

std::vector<GameMapInfo> Simulator::loadGameMapsFromFolder(const std::string& folder_path)
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

std::unordered_map<std::string, size_t>
Simulator::runCompetitionGames(const std::vector<GameMapInfo>& maps)
{
    std::unordered_map<std::string, size_t> scores;

    // Initialize scores to zero for each algorithm
    auto& algo_registrar = AlgorithmRegistrar::getAlgorithmRegistrar();
    for (const auto& entry : algo_registrar)
    {
        scores[entry.name()] = 0;
    }

    // Create thread pool for parallel execution
    ThreadPool thread_pool(config_.num_threads);
    
    // Collect all futures and player pairs from all maps
    std::vector<std::future<GameResult>> all_futures;
    std::vector<std::pair<std::string, std::string>> all_player_pairs;

    // Submit all games from all maps to the thread pool
    for (size_t k = 0; k < maps.size(); ++k)
    {
        submitCompetitionGamesForMap(maps[k], k, thread_pool, all_futures, all_player_pairs);
    }

    // Collect all results and update scores
    for (size_t i = 0; i < all_futures.size(); ++i)
    {
        GameResult result = all_futures[i].get();
        const auto& [player1_name, player2_name] = all_player_pairs[i];
        updateScores(result, player1_name, player2_name, scores);
    }

    return scores;
}

void Simulator::runCompetitionGamesForMap(const GameMapInfo& map, size_t map_index,
                                          std::unordered_map<std::string, size_t>& scores,
                                          ThreadPool& thread_pool)
{
    auto& gm_registrar = GameManagerRegistrar::getGameManagerRegistrar();
    auto& algo_registrar = AlgorithmRegistrar::getAlgorithmRegistrar();

    const auto& game_manager_entry = *gm_registrar.begin();
    size_t N = algo_registrar.count();

    // Create futures for parallel execution
    std::vector<std::future<GameResult>> futures;
    std::vector<std::pair<std::string, std::string>> player_pairs;

    for (size_t i = 0; i < N; ++i)
    {
        // Get opponent index
        size_t j = (i + 1 + (map_index % (N - 1))) % N;

        // Get Algorithms entries
        const auto& algo1_entry = algo_registrar.getEntry(i);
        const auto& algo2_entry = algo_registrar.getEntry(j);

        // Capture necessary data by value for thread safety
        bool verbose = config_.verbose;
        std::string algo1_name = algo1_entry.name();
        std::string algo2_name = algo2_entry.name();
        TankAlgorithmFactory tank_factory1 = algo1_entry.getTankAlgorithmFactory();
        TankAlgorithmFactory tank_factory2 = algo2_entry.getTankAlgorithmFactory();

        // Store player pair for later score update
        player_pairs.emplace_back(algo1_name, algo2_name);

        auto future = thread_pool.enqueue([this, &game_manager_entry, &algo1_entry, &algo2_entry, &map,
                                          verbose, algo1_name, algo2_name, tank_factory1, tank_factory2]() -> GameResult {
            // Create GameManager and Player instances
            std::unique_ptr<AbstractGameManager> game_manager = game_manager_entry.createGameManager(verbose);
            std::unique_ptr<Player> player1 = algo1_entry.createPlayer(1, map.width, map.height, map.max_steps, map.num_shells);
            std::unique_ptr<Player> player2 = algo2_entry.createPlayer(2, map.width, map.height, map.max_steps, map.num_shells);

            // Run the game and return the result
            return runSingleGame(*game_manager, *player1, *player2,
                                 tank_factory1, tank_factory2,
                                 algo1_name, algo2_name, map);
        });

        futures.push_back(std::move(future));
    }

    // Collect results and update scores
    for (size_t i = 0; i < futures.size(); ++i)
    {
        GameResult result = futures[i].get();
        const auto& [player1_name, player2_name] = player_pairs[i];
        updateScores(result, player1_name, player2_name, scores);
    }
}

void Simulator::submitCompetitionGamesForMap(const GameMapInfo& map, size_t map_index,
                                             ThreadPool& thread_pool,
                                             std::vector<std::future<GameResult>>& all_futures,
                                             std::vector<std::pair<std::string, std::string>>& all_player_pairs)
{
    auto& gm_registrar = GameManagerRegistrar::getGameManagerRegistrar();
    auto& algo_registrar = AlgorithmRegistrar::getAlgorithmRegistrar();

    const auto& game_manager_entry = *gm_registrar.begin();
    size_t N = algo_registrar.count();

    for (size_t i = 0; i < N; ++i)
    {
        // Get opponent index
        size_t j = (i + 1 + (map_index % (N - 1))) % N;

        // Get Algorithms entries
        const auto& algo1_entry = algo_registrar.getEntry(i);
        const auto& algo2_entry = algo_registrar.getEntry(j);

        // Capture necessary data by value for thread safety
        bool verbose = config_.verbose;
        std::string algo1_name = algo1_entry.name();
        std::string algo2_name = algo2_entry.name();
        TankAlgorithmFactory tank_factory1 = algo1_entry.getTankAlgorithmFactory();
        TankAlgorithmFactory tank_factory2 = algo2_entry.getTankAlgorithmFactory();

        // Store player pair for later score update
        all_player_pairs.emplace_back(algo1_name, algo2_name);

        auto future = thread_pool.enqueue([this, &game_manager_entry, &algo1_entry, &algo2_entry, &map,
                                          verbose, algo1_name, algo2_name, tank_factory1, tank_factory2]() -> GameResult {
            // Create GameManager and Player instances
            std::unique_ptr<AbstractGameManager> game_manager = game_manager_entry.createGameManager(verbose);
            std::unique_ptr<Player> player1 = algo1_entry.createPlayer(1, map.width, map.height, map.max_steps, map.num_shells);
            std::unique_ptr<Player> player2 = algo2_entry.createPlayer(2, map.width, map.height, map.max_steps, map.num_shells);

            // Run the game and return the result
            return runSingleGame(*game_manager, *player1, *player2,
                                 tank_factory1, tank_factory2,
                                 algo1_name, algo2_name, map);
        });

        all_futures.push_back(std::move(future));
    }
}

void Simulator::updateScores(const GameResult& result,
                             const std::string& player1_name,
                             const std::string& player2_name,
                             std::unordered_map<std::string, size_t>& scores)
{
    // Score by 3 points for a win, 1 point for tie, 0 points for loss
    if (result.winner == 1)
    {
        scores[player1_name] += 3; // Player 1 wins
    }
    else if (result.winner == 2)
    {
        scores[player2_name] += 3; // Player 2 wins
    }
    else
    {
        scores[player1_name] += 1; // Tie
        scores[player2_name] += 1;
    }
}

void Simulator::printCompetitionResults(std::ostream& out,
                                        std::unordered_map<std::string, size_t> scores)
{
    // Sort scores in descending order
    auto sorted_scores = sortCompetitionScores(std::move(scores));

    for (const auto& [name, score] : sorted_scores)
    {
        out << name << " " << score << '\n';
    }
}

std::vector<std::pair<std::string, size_t>>
Simulator::sortCompetitionScores(std::unordered_map<std::string, size_t> scores)
{
    std::vector<std::pair<std::string, size_t>> sorted{
        std::make_move_iterator(scores.begin()),
        std::make_move_iterator(scores.end())};

    // Sort by score (descending)
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b)
              {
                  return a.second > b.second;
              });

    return sorted;
}

/*======================================================================================*/

void Simulator::runSingle()
{
    // Load .so files
    loadSingleSharedObjects();

    // Load game map
    GameMapInfo map_info = loadGameMap(config_.game_map_filename);
    if (!map_info.is_valid)
    {
        throw SimulatorException("Invalid game map: " + config_.game_map_filename);
    }

    // Get GameManager entry
    const auto& game_manager_entry = *GameManagerRegistrar::getGameManagerRegistrar().begin();

    // Get algorithm entries from the registrar
    auto& algo_registrar = AlgorithmRegistrar::getAlgorithmRegistrar();
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

    // Run the game
    runSingleGame(*game_manager, *player1, *player2,
                  tank_factory1, tank_factory2,
                  it1->name(), it2->name(), map_info);
}

void Simulator::loadSingleSharedObjects()
{
    // Load GameManager .so
    if (!loadSharedObject<GameManagerRegistrar>(config_.game_manager_so))
    {
        throw SimulatorException("Failed to load game manager: " + config_.game_manager_so);
    }

    // Load algorithm1
    if (!loadSharedObject<AlgorithmRegistrar>(config_.algorithm1_so))
    {
        throw SimulatorException("Failed to load algorithm1: " + config_.algorithm1_so);
    }

    // Load algorithm2, unless it's the same as algorithm1
    if (config_.algorithm2_so != config_.algorithm1_so &&
        !loadSharedObject<AlgorithmRegistrar>(config_.algorithm2_so))
    {
        throw SimulatorException("Failed to load algorithm2: " + config_.algorithm2_so);
    }
}

/*======================================================================================*/

template <typename Registrar>
size_t Simulator::loadSharedObjectsFromFolder(const std::string& folder_path)
{
    // Check if the folder exists and is a directory
    if (!fs::exists(folder_path) || !fs::is_directory(folder_path))
    {
        std::ostringstream oss;
        ArgumentsParser::printUsage(oss, "Folder does not exist or is not a directory: " + folder_path);
        throw SimulatorException(oss.str());
    }

    // Iterate through all files in the folder and load valid shared objects
    size_t loaded_count = 0;
    for (const auto& entry : fs::directory_iterator(folder_path))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".so")
            continue;

        std::string path = entry.path().string();
        if (loadSharedObject<Registrar>(path))
        {
            ++loaded_count;
        }
    }

    return loaded_count;
}

template <typename Registrar>
bool Simulator::loadSharedObject(const std::string& path)
{
    std::string name = fs::path(path).stem().string();
    RegistrarAdapter<Registrar>::createEntry(name);

    void* handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
    if (!handle)
    {
        errors_logger_.logGeneral("dlopen failed for ", path, ": ", dlerror());
        RegistrarAdapter<Registrar>::removeLast();
        return false;
    }

    try
    {
        RegistrarAdapter<Registrar>::validateLast();
    }
    catch (const typename RegistrarAdapter<Registrar>::BadRegistrationException& e)
    {
        errors_logger_.logGeneral(RegistrarAdapter<Registrar>::getBadRegistrationDetails(e));
        RegistrarAdapter<Registrar>::removeLast();
        dlclose(handle);
        return false;
    }

    so_handles_.push_back(handle);

    return true;
}

std::ostream& Simulator::initOutputStream(std::ofstream& file_out, const std::string& folder, const std::string& prefix)
{
    std::string time_suffix = getUniqueTimeString();
    std::string filename = prefix + time_suffix + ".txt";
    std::string filepath = fs::path(folder) / filename;

    file_out.open(filepath);
    if (!file_out)
    {
        std::cerr << "Error: Could not create output file at " << filepath << std::endl
                  << "Falling back to standard output." << std::endl;
        return std::cout;
    }

    return file_out;
}

void Simulator::printOutputHeader(std::ostream& out, RunMode mode)
{
    if (mode == RunMode::COMPARATIVE)
    {
        out << "game_map=" << fs::path(config_.game_map_filename).stem().string() << '\n'
            << "algorithm1=" << fs::path(config_.algorithm1_so).stem().string() << '\n'
            << "algorithm2=" << fs::path(config_.algorithm2_so).stem().string() << '\n'
            << '\n';
    }
    else if (mode == RunMode::COMPETITION)
    {
        out << "game_maps_folder=" << config_.game_maps_folder << '\n'
            << "game_manager=" << fs::path(config_.game_manager_so).stem().string() << '\n'
            << '\n';
    }
}

GameMapInfo Simulator::loadGameMap(const std::string& map_filename)
{
    std::ifstream file(map_filename);

    if (!file)
    {
        errors_logger_.logGeneral("Couldn't open map file: ", map_filename);
        return GameMapInfo();
    }

    std::string line;
    std::getline(file, line); // Skip the first line (map name/description)

    auto parse_metadata = [&file, &line, &map_filename, this](const std::string& expected_key, size_t& target) -> bool
    {
        std::getline(file, line);
        auto pos = line.find("=");
        if (pos == std::string::npos || line.find(expected_key) == std::string::npos)
        {
            errors_logger_.logFile(map_filename, "Missing or invalid line for ", expected_key);
            return false;
        }
        try
        {
            target = std::stoi(line.substr(pos + 1));
        }
        catch (...)
        {
            errors_logger_.logFile(map_filename, "Invalid value for ", expected_key, ": ", line.substr(pos + 1), ".");
            return false;
        }
        return true;
    };

    size_t max_steps, num_shells, height, width;

    if (!parse_metadata("MaxSteps", max_steps) ||
        !parse_metadata("NumShells", num_shells) ||
        !parse_metadata("Rows", height) ||
        !parse_metadata("Cols", width))
    {
        errors_logger_.logFile(map_filename, "File structure is invalid, ignoring this file.");
        return GameMapInfo();
    }

    std::vector<std::vector<char>> chars_grid(width, std::vector<char>(height, ' '));
    size_t actual_row_count = 0;

    for (; actual_row_count < height && std::getline(file, line); ++actual_row_count)
    {
        // Remove '\r' at end if present (common in Windows CRLF files, causes problems with parsing)
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line.size() > width)
        {
            errors_logger_.logFile(map_filename, "Row ", actual_row_count, " is too long. Ignoring extra cells.");
        }
        else if (line.size() < width)
        {
            errors_logger_.logFile(map_filename, "Row ", actual_row_count, " is too short. Treating missing cells as empty.");
        }
        line.resize(width, ' ');

        for (size_t x = 0; x < width; ++x)
        {
            char ch = line[x];

            if (ch == '#' || ch == '@' || ch == ' ' || ch == '.' || ch == '1' || ch == '2')
            {
                chars_grid[x][actual_row_count] = ch; // Valid characters
            }
            else
            {
                errors_logger_.logFile(map_filename, "Invalid character '", ch, "' at (", x, ",", actual_row_count, "). Treating as empty.");
                // Treat invalid characters as empty, grid is already initialized to spaces
            }
        }
    }

    if (actual_row_count < height)
    {
        errors_logger_.logFile(map_filename, "The map has only ", actual_row_count, " rows, expected ", height,
                               ". Treating missing rows as empty.");
    }
    else if (std::getline(file, line))
    {
        errors_logger_.logFile(map_filename, "The map has more rows than expected (", height, "). Ignoring extra rows.");
    }

    return GameMapInfo(
        fs::path(map_filename).stem().string(),
        std::make_unique<BoardSatelliteView>(std::move(chars_grid)),
        height, width, max_steps, num_shells);
}

GameResult Simulator::runSingleGame(AbstractGameManager& game_manager,
                                    Player& player1, Player& player2,
                                    const TankAlgorithmFactory& tank_factory1,
                                    const TankAlgorithmFactory& tank_factory2,
                                    const std::string& name1, const std::string& name2,
                                    const GameMapInfo& map_info)
{
    return game_manager.run(
        map_info.width, map_info.height,
        *map_info.satellite_view, map_info.name,
        map_info.max_steps, map_info.num_shells,
        player1, name1, player2, name2,
        tank_factory1, tank_factory2);
}