#include "simulator.h"

#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "arguments_parser.h"
#include "board_satellite_view.h"
#include "global_config.h"
#include "input_errors_logger.h"
#include "registrations/registrar_adapter.h"
#include "simulator_exception.h"
#include "utils.h"

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
        std::cout << "Running in comparative mode..." << std::endl;
        runComparative();
    }
    else if (config_.mode == RunMode::COMPETITION)
    {
        std::cout << "Running in competition mode..." << std::endl;
        runCompetition();
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
    loadSharedObjectsFromFolder<GameManagerRegistrar>(config_.game_managers_folder);

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
    GameMapInfo map_info = loadGameMap(config_.game_map_filename);
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

    if (it1 == algo_registrar.end() || it2 == algo_registrar.end())
        throw SimulatorException("Algorithms were not registered properly");


    // Get tank algorithm factories
    TankAlgorithmFactory tank_factory1 = it1->getTankAlgorithmFactory();
    TankAlgorithmFactory tank_factory2 = it2->getTankAlgorithmFactory();

    // Iterate over all game managers and run the game
    auto& gm_registrar = GameManagerRegistrar::getGameManagerRegistrar();
    for (const auto& gm_entry : gm_registrar)
    {
        // Create GameManager and Players instances
        std::unique_ptr<AbstractGameManager> game_manager = gm_entry.createGameManager(config_.verbose);
        std::unique_ptr<Player> player1 = it1->createPlayer(1, map_info.width, map_info.height, map_info.max_steps, map_info.num_shells);
        std::unique_ptr<Player> player2 = it2->createPlayer(2, map_info.width, map_info.height, map_info.max_steps, map_info.num_shells);

        // Run the game
        GameResult result = runSingleGame(*game_manager, *player1, *player2,
                                          tank_factory1, tank_factory2,
                                          it1->name(), it2->name(), map_info);

        results.emplace_back(gm_entry.name(), std::move(result));
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

    // 3. Validate competition requirements
    validateCompetitionRequirements(maps);

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
    if (size_t count = loadSharedObjectsFromFolder<AlgorithmRegistrar>(config_.algorithms_folder); count < 2)
    {
        std::ostringstream oss;
        ArgumentsParser::printUsage(oss, "At least two algorithms must be registered for competition mode, "
                                         "found: " +
                                             std::to_string(count) + " in " + config_.algorithms_folder);
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
        if (!entry.is_regular_file())
            continue;

        GameMapInfo map_info = loadGameMap(entry.path().string());
        if (map_info.is_valid)
        {
            maps.push_back(std::move(map_info));
        }
    }

    return maps;
}

void Simulator::validateCompetitionRequirements(const std::vector<GameMapInfo>& maps)
{
    if (maps.empty())
    {
        std::ostringstream oss;
        ArgumentsParser::printUsage(oss, "No valid maps found in folder: " + config_.game_maps_folder);
        throw SimulatorException(oss.str());
    }

    const auto& gm_registrar = GameManagerRegistrar::getGameManagerRegistrar();

    if (gm_registrar.count() < 1)
    {
        throw SimulatorException("Game Manager was not registered properly.");
    }

    const auto& algo_registrar = AlgorithmRegistrar::getAlgorithmRegistrar();

    if (algo_registrar.count() < 2)
    {
        throw SimulatorException("Algorithms were not registered properly.");
    }
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

    // Run all competition games for each map
    for (size_t k = 0; k < maps.size(); ++k)
    {
        runCompetitionGamesForMap(maps[k], k, scores);
    }

    return scores;
}

void Simulator::runCompetitionGamesForMap(const GameMapInfo& map, size_t map_index,
                                          std::unordered_map<std::string, size_t>& scores)
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
        updateScores(result, algo1_entry.name(), algo2_entry.name(), scores);
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

    if constexpr (config::get<bool>("verbose_debug"))
    {
        std::cout << "Loading shared objects from folder: " << folder_path << std::endl;
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

    if (loaded_count == 0)
    {
        std::ostringstream oss;
        ArgumentsParser::printUsage(oss, "No valid .so files found in: " + folder_path);
        throw SimulatorException(oss.str());
    }

    return loaded_count;
}

template <typename Registrar>
bool Simulator::loadSharedObject(const std::string& path)
{
    if constexpr (config::get<bool>("verbose_debug"))
    {
        std::cout << "Loading shared object: " << path << std::endl;
    }

    std::string name = fs::path(path).stem().string();
    RegistrarAdapter<Registrar>::createEntry(name);

    void* handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
    if (!handle)
    {
        std::cerr << "dlopen failed: " << dlerror() << std::endl;
        RegistrarAdapter<Registrar>::removeLast();
        return false;
    }

    try
    {
        RegistrarAdapter<Registrar>::validateLast();
    }
    catch (const typename RegistrarAdapter<Registrar>::BadRegistrationException& e)
    {
        RegistrarAdapter<Registrar>::printBadRegistrationDetails(e);
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
    InputErrorsLogger error_logger;
    std::ifstream file(map_filename);

    if (!file)
    {
        error_logger.log("Couldn't open map file: ", map_filename);
        return GameMapInfo();
    }

    std::string line;
    std::getline(file, line); // Skip the first line (map name/description)

    auto parse_metadata = [&file, &line, &error_logger](const std::string& expected_key, size_t& target) -> bool
    {
        std::getline(file, line);
        auto pos = line.find("=");
        if (pos == std::string::npos || line.find(expected_key) == std::string::npos)
        {
            error_logger.log("Missing or invalid line for ", expected_key);
            return false;
        }
        try
        {
            target = std::stoi(line.substr(pos + 1));
        }
        catch (const std::exception&)
        {
            error_logger.log("Invalid value for ", expected_key, ": ", line.substr(pos + 1));
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
        error_logger.log("File structure is invalid: ", map_filename);
        return GameMapInfo();
    }

    std::vector<std::vector<char>> chars_grid(width, std::vector<char>(height, ' '));

    for (size_t y = 0; y < height; ++y)
    {
        std::getline(file, line);
        if (line.size() > width)
        {
            error_logger.log("Warning: The row ", y, " is too long. Ignoring extra cells.");
        }
        else if (line.size() < width)
        {
            line += std::string(width - line.size(), ' ');
            error_logger.log("Warning: The row ", y, " is too short. Treating missing cells as empty.");
        }

        for (size_t x = 0; x < width && x < line.size(); ++x)
        {
            char ch = line[x];

            if (ch == '#' || ch == '@' || ch == ' ' || ch == '.' || (ch >= '1' && ch <= '9'))
            {
                chars_grid[x][y] = ch; // Valid characters
            }
            else
            {
                error_logger.log("Warning: Invalid character '", ch, "' at (", x, ",", y, "). Treating as empty.");
                chars_grid[x][y] = ' '; // Treat invalid characters as empty
            }
        }
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