#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "AbstractGameManager.h"
#include "GameResult.h"
#include "arguments_parser.h"
#include "comparable_game_result.h"
#include "errors_logger.h"
#include "game_map_info.h"


class Simulator
{
public:
    explicit Simulator(SimulatorConfig config);
    ~Simulator();

    void run(); // Entrypoint to trigger execution

private:
    // =========== Comparative mode functions ===========
    void runComparative();
    void loadComparativeSharedObjects();
    struct GameManagerExecutionResult
    {
        std::string manager_name;
        GameResult result;
    };
    void runComparativeGameManagers(std::vector<GameManagerExecutionResult>& results,
                                    size_t& map_width, size_t& map_height);
    void printGroupedComparativeResults(std::ostream& out,
                                        std::vector<GameManagerExecutionResult> results,
                                        size_t map_width, size_t map_height);

    std::unordered_map<ComparableGameResult, std::vector<std::string>>
    groupComparativeResults(std::vector<GameManagerExecutionResult> results,
                            size_t map_width, size_t map_height);
    std::vector<std::pair<ComparableGameResult, std::vector<std::string>>>
    sortComparativeGroups(std::unordered_map<ComparableGameResult, std::vector<std::string>> grouped);

    // ========== Competition mode functions ===========
    void runCompetition();
    void loadCompetitionSharedObjects();
    std::vector<GameMapInfo> loadGameMapsFromFolder(const std::string& folder_path);
    std::unordered_map<std::string, size_t>
    runCompetitionGames(const std::vector<GameMapInfo>& maps);
    void runCompetitionGamesForMap(const GameMapInfo& map, size_t map_index,
                                   std::unordered_map<std::string, size_t>& scores);
    void updateScores(const GameResult& result,
                      const std::string& player1_name,
                      const std::string& player2_name,
                      std::unordered_map<std::string, size_t>& scores);
    void printCompetitionResults(std::ostream& out,
                                 std::unordered_map<std::string, size_t> scores);
    std::vector<std::pair<std::string, size_t>>
    sortCompetitionScores(std::unordered_map<std::string, size_t> scores);

    // ============ Shared helper functions ============
    template <typename Registrar>
    bool loadSharedObject(const std::string& path);

    template <typename Registrar>
    size_t loadSharedObjectsFromFolder(const std::string& folder_path);

    std::ostream& initOutputStream(std::ofstream& file_out, const std::string& folder, const std::string& prefix);
    void printOutputHeader(std::ostream& out, RunMode mode);
    GameMapInfo loadGameMap(const std::string& map_filename);
    GameResult runSingleGame(AbstractGameManager& game_manager,
                             Player& player1, Player& player2,
                             const TankAlgorithmFactory& tank_factory1,
                             const TankAlgorithmFactory& tank_factory2,
                             const std::string& name1, const std::string& name2,
                             const GameMapInfo& map_info);

    // ============ Data members ============
    SimulatorConfig config_;
    std::vector<void*> so_handles_; // To keep track of loaded shared objects, to be closed at destruction
    ErrorsLogger errors_logger_;
};