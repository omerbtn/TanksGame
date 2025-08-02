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
#include "loader/shared_object_loader.h"
#include "thread_pool.h"


namespace simulator
{
class Simulator
{
public:
    explicit Simulator(SimulatorConfig config);
    ~Simulator() = default;
    Simulator(const Simulator&) = delete;
    Simulator& operator=(const Simulator&) = delete;
    Simulator(Simulator&&) = delete;
    Simulator& operator=(Simulator&&) = delete;

    void run();

private:
    SimulatorConfig config_;
};

} // namespace simulator