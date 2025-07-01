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
        // runCompetition();
    }
}

void Simulator::runComparative()
{
    // 1. Load .so files
    loadSharedObjectsFromFolder<GameManagerRegistrar>(config_.game_managers_folder);

    // Handle algorithm1
    if (!loadSharedObject<AlgorithmRegistrar>(config_.algorithm1_so))
    {
        throw SimulatorException("Failed to load algorithm1: " + config_.algorithm1_so);
    }

    // Handle algorithm2, unless it's the same path as algorithm1
    if (config_.algorithm2_so != config_.algorithm1_so &&
        !loadSharedObject<AlgorithmRegistrar>(config_.algorithm2_so))
    {
        throw SimulatorException("Failed to load algorithm2: " + config_.algorithm2_so);
    }

    // 2. Prepare output stream
    std::ofstream file_out;
    std::ostream& out = initOutputStream(file_out, config_.game_managers_folder,
                                         std::string(config::get<std::string_view>("comparative_output_prefix")));

    // 3. Print the header
    printComparativeHeader(out);

    // 4. Run the game managers
    std::vector<GameManagerExecutionResult> results;
    size_t map_width, map_height;
    runComparativeGameManagers(results, map_width, map_height);

    // 5. Group and print results
    printGroupedComparativeResults(out, std::move(results), map_width, map_height);
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
        throw SimulatorException("Algorithm entries not found in registrar");


    // Get tank algorithm factories
    TankAlgorithmFactory tank_factory1 = it1->getTankAlgorithmFactory();
    TankAlgorithmFactory tank_factory2 = it2->getTankAlgorithmFactory();

    // Iterate over all game managers and run the game
    auto& gm_registrar = GameManagerRegistrar::getGameManagerRegistrar();
    for (const auto& gm_entry : gm_registrar)
    {
        // Create game manager
        std::unique_ptr<AbstractGameManager> game_manager = gm_entry.createGameManager(config_.verbose);

        // Create players
        std::unique_ptr<Player> player1 = it1->createPlayer(1, map_info.width, map_info.height, map_info.max_steps, map_info.num_shells);
        std::unique_ptr<Player> player2 = it2->createPlayer(2, map_info.width, map_info.height, map_info.max_steps, map_info.num_shells);

        // Run the game
        GameResult result = game_manager->run(
            map_info.width, map_info.height, *map_info.satellite_view,
            map_info.max_steps, map_info.num_shells,
            *player1, *player2,
            tank_factory1, tank_factory2);

        results.emplace_back(gm_entry.name(), std::move(result));
    }
}

void Simulator::printGroupedComparativeResults(
    std::ostream& out,
    std::vector<GameManagerExecutionResult>&& results,
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
Simulator::groupComparativeResults(std::vector<GameManagerExecutionResult>&& results,
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
Simulator::sortComparativeGroups(
    std::unordered_map<ComparableGameResult, std::vector<std::string>>&& grouped)
{
    std::vector<std::pair<ComparableGameResult, std::vector<std::string>>> sorted{
        std::make_move_iterator(grouped.begin()),
        std::make_move_iterator(grouped.end())};

    // Sort bt group size (descending)
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b)
              {
                  return a.second.size() > b.second.size();
              });

    return sorted;
}

template <typename Registrar>
void Simulator::loadSharedObjectsFromFolder(const std::string& folder_path)
{
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

void Simulator::printComparativeHeader(std::ostream& out)
{
    out << "game_map=" << config_.game_map_filename << '\n'
        << "algorithm1=" << config_.algorithm1_so << '\n'
        << "algorithm2=" << config_.algorithm2_so << '\n'
        << '\n';
}

GameMapInfo Simulator::loadGameMap(const std::string& map_filename)
{
    InputErrorLogger error_logger;
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
        target = std::stoi(line.substr(pos + 1));
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
        std::make_unique<BoardSatelliteView>(std::move(chars_grid)),
        height, width, max_steps, num_shells);
}