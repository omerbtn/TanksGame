#include "simulator.h"

#include "runners/comparative_runner.h"
#include "runners/competitive_runner.h"
#include "runners/single_mode_runner.h"

namespace simulator
{

namespace fs = std::filesystem;
using namespace UserCommon_322573304_322647603;

Simulator::Simulator(SimulatorConfig config) : config_(std::move(config)) {}

void Simulator::run()
{
    std::unique_ptr<runners::Runner> runner;
    if (config_.mode == RunMode::COMPARATIVE)
    {
        runner = std::make_unique<runners::ComparativeRunner>(config_);
    }
    else if (config_.mode == RunMode::COMPETITION)
    {
        runner = std::make_unique<runners::CompetitiveRunner>(config_);
    }
    else if (config_.mode == RunMode::SINGLE)
    {
        runner = std::make_unique<runners::SingleModeRunner>(config_);
    }

    runner->execute();
}

} // namespace simulator