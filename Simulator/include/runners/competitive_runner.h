#pragma once

#include <fstream>

#include "runner.h"
#include "loader/shared_object_loader.h"
#include "simulator.h"
#include "game_map_info.h"

namespace simulator
{
namespace runners
{

class CompetitiveRunner : public Runner {
public:
    virtual ~CompetitiveRunner();
    explicit CompetitiveRunner(SimulatorConfig config);
    CompetitiveRunner(const CompetitiveRunner&) = delete;
    CompetitiveRunner& operator=(const CompetitiveRunner&) = delete;
    CompetitiveRunner(CompetitiveRunner&&) = delete;
    CompetitiveRunner& operator=(CompetitiveRunner&&) = delete;

    void loadSharedObjects() override;
    void prepare() override;
    void run() override;
    void printResults() override;

    std::vector<GameMapInfo> loadGameMapsFromFolder(const std::string& folder_path);
    void runCompetition();
    void loadCompetitionSharedObjects();
    void runCompetitionGames();
    void submitCompetitionGamesForMap(const GameMapInfo& map, size_t map_index,
                                      ThreadPool& thread_pool,
                                      std::vector<std::future<GameResult>>& all_futures,
                                      std::vector<std::pair<std::string, std::string>>& all_player_pairs);
    void submitCompetitionMatch(ThreadPool& thread_pool,
                                const GameMapInfo& map,
                                const simulator::registrations::AlgorithmRegistrar& algo_registrar,
                                size_t idx1, size_t idx2,
                                const simulator::registrations::GameManagerRegistrar::value_type& game_manager_entry,
                                std::vector<std::future<GameResult>>& all_futures,
                                std::vector<std::pair<std::string, std::string>>& all_player_pairs);
    void runCompetitionGamesForMapSingleThread(const GameMapInfo& map, size_t map_index);
    void updateScores(const GameResult& result,
                      const std::string& player1_name,
                      const std::string& player2_name);
    void printCompetitionResults();
    std::vector<std::pair<std::string, size_t>> sortCompetitionScores();

private:
    std::vector<GameManagerExecutionResult> results_;
    std::ofstream file_;
    size_t map_width_;
    size_t map_height_;
    std::vector<GameMapInfo> maps_;

    std::unordered_map<std::string, size_t> scores_;
};

} // namespace runners
} // namespace simulator