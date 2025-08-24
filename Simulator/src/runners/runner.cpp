#include "runners/runner.h"

#include <fstream>
#include <filesystem>
#include <string>

#include "board_satellite_view.h"
#include "global_config.h"
#include "utils.h"

namespace simulator
{
namespace runners
{

namespace fs = std::filesystem;
using namespace UserCommon_322573304_322647603;

Runner::Runner(SimulatorConfig config) : config_(std::move(config)), shared_object_loader_(errors_logger_) {}

void Runner::execute() {
    loadSharedObjects();
    run();
}

std::ostream& Runner::initOutputStream(std::ofstream& file_out, const std::string& folder, const std::string& prefix)
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

void Runner::printOutputHeader(std::ostream& out, RunMode mode)
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

GameMapInfo Runner::loadGameMap(const std::string& map_filename)
{
    std::ifstream file(map_filename);
    if (!file) {
        errors_logger_.logGeneral("Couldn't open map file: ", map_filename);
        return {};
    }

    std::string line;
    std::getline(file, line); // Skip first line

    size_t max_steps, num_shells, height, width;
    if (!parseMetadataFields(file, map_filename, max_steps, num_shells, height, width)) {
        errors_logger_.logFile(map_filename, "File structure is invalid, ignoring this file.");
        return {};
    }

    auto grid = parseMapGrid(file, map_filename, width, height);
    return GameMapInfo(
        fs::path(map_filename).stem().string(),
        std::make_unique<BoardSatelliteView>(std::move(grid)),
        height, width, max_steps, num_shells);
}

bool Runner::parseMetadataFields(std::ifstream& file, const std::string& map_filename,
                                 size_t& max_steps, size_t& num_shells, size_t& height, size_t& width)
{
    auto parseLine = [&](const std::string& key, size_t& out) -> bool {
        std::string line;
        if (!std::getline(file, line)) return false;
        auto pos = line.find('=');
        if (pos == std::string::npos || line.find(key) == std::string::npos) {
            errors_logger_.logFile(map_filename, "Missing or invalid line for ", key, ".");
            return false;
        }
        try {
            out = std::stoul(line.substr(pos + 1));
            return true;
        } catch (...) {
            errors_logger_.logFile(map_filename, "Invalid value for ", key, ": ", line.substr(pos + 1), ".");
            return false;
        }
    };

    return parseLine("MaxSteps", max_steps) &&
           parseLine("NumShells", num_shells) &&
           parseLine("Rows", height) &&
           parseLine("Cols", width);
}

std::vector<std::vector<char>> Runner::parseMapGrid(std::ifstream& file, const std::string& map_filename,
                                                    size_t width, size_t height)
{
    std::vector<std::vector<char>> grid(width, std::vector<char>(height, ' '));
    std::string line;
    const std::string valid = "#@ .12";
    size_t row = 0;

    for (; row < height && std::getline(file, line); ++row) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.size() != width) {
            errors_logger_.logFile(map_filename, "Row ", row, " length mismatch. Adjusting.");
            line.resize(width, ' ');
        }

        for (size_t col = 0; col < width; ++col) {
            char ch = line[col];
            if (valid.find(ch) != std::string::npos)
                grid[col][row] = ch;
            else
                errors_logger_.logFile(map_filename, "Invalid char '", ch, "' at (", col, ",", row, "). Treated as empty.");
        }
    }

    if (row < height)
        errors_logger_.logFile(map_filename, "Only ", row, " rows found, expected ", height);
    else if (std::getline(file, line))
        errors_logger_.logFile(map_filename, "More rows than expected. Ignoring extras.");

    return grid;
}


GameResult Runner::runSingleGame(AbstractGameManager& game_manager,
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

} // namespace runners
} // namespace simulator