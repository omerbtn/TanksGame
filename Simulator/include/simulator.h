#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "GameResult.h"
#include "arguments_parser.h"
#include "comparable_game_result.h"
#include "game_map_info.h"


class Simulator
{
public:
    explicit Simulator(SimulatorConfig config);
    ~Simulator();

    void run(); // Entrypoint to trigger execution

private:
    // Comparative mode
    void runComparative();
    void printComparativeHeader(std::ostream& out);
    struct GameManagerExecutionResult
    {
        std::string manager_name;
        GameResult result;
    };
    void runComparativeGameManagers(std::vector<GameManagerExecutionResult>& results,
                                    size_t& map_width, size_t& map_height);
    void printGroupedComparativeResults(
        std::ostream& out,
        std::vector<GameManagerExecutionResult>&& results,
        size_t map_width, size_t map_height);

    std::unordered_map<ComparableGameResult, std::vector<std::string>>
    groupComparativeResults(std::vector<GameManagerExecutionResult>&& results,
                            size_t map_width, size_t map_height);
    std::vector<std::pair<ComparableGameResult, std::vector<std::string>>>
    sortComparativeGroups(
        std::unordered_map<ComparableGameResult, std::vector<std::string>>&& grouped);

    // Competition mode
    void runCompetition();

    // Shared helper functions
    template <typename Registrar>
    bool loadSharedObject(const std::string& path);

    template <typename Registrar>
    void loadSharedObjectsFromFolder(const std::string& folder_path);

    std::ostream& initOutputStream(std::ofstream& file_out, const std::string& folder, const std::string& prefix);
    GameMapInfo loadGameMap(const std::string& map_filename);

    SimulatorConfig config_;
    std::vector<void*> so_handles_; // To keep track of loaded shared objects, to be closed at destruction
};