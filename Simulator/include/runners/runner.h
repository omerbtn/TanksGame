#pragma once

#include "simulator.h"
#include "errors_logger.h"
#include "loader/shared_object_loader.h"

namespace simulator
{
namespace runners
{

struct GameManagerExecutionResult
{
    std::string manager_name;
    GameResult result;
};

class Runner {
public:
    virtual ~Runner() = default;
    explicit Runner(SimulatorConfig config);
    Runner(const Runner&) = delete;
    Runner& operator=(const Runner&) = delete;
    Runner(Runner&&) = delete;
    Runner& operator=(Runner&&) = delete;

    void execute();

protected:
    std::ostream& initOutputStream(std::ofstream& file_out, const std::string& folder, const std::string& prefix);

    void printOutputHeader(std::ostream& out, RunMode mode);

    GameMapInfo loadGameMap(const std::string& map_filename);

    GameResult runSingleGame(AbstractGameManager& game_manager,
                            Player& player1, Player& player2,
                            const TankAlgorithmFactory& tank_factory1,
                            const TankAlgorithmFactory& tank_factory2,
                            const std::string& name1, const std::string& name2,
                            const GameMapInfo& map_info);

    SimulatorConfig config_;
    ErrorsLogger errors_logger_;
    simulator::loader::SharedObjectLoader shared_object_loader_;

private:
    virtual void loadSharedObjects() = 0;
    virtual void run() = 0;

    bool parseMetadataFields(std::ifstream& file, const std::string& map_filename,
                             size_t& max_steps, size_t& num_shells, size_t& height, size_t& width);
    std::vector<std::vector<char>> parseMapGrid(std::ifstream& file, const std::string& map_filename,
                                                size_t width, size_t height);
};

} // namespace runners
} // namespace simulator