#pragma once

#include <fstream>
#include <ostream>

#include "runner.h"
#include "loader/shared_object_loader.h"

namespace simulator
{
namespace runners
{

class ComparativeRunner : public Runner {
public:
    virtual ~ComparativeRunner();
    explicit ComparativeRunner(SimulatorConfig config);
    ComparativeRunner(const ComparativeRunner&) = delete;
    ComparativeRunner& operator=(const ComparativeRunner&) = delete;
    ComparativeRunner(ComparativeRunner&&) = delete;
    ComparativeRunner& operator=(ComparativeRunner&&) = delete;

    virtual void loadSharedObjects() override;
    virtual void run() override;

    void runComparativeGameManagers();
    void printGroupedComparativeResults(std::ostream& out);

    std::unordered_map<ComparableGameResult, std::vector<std::string>> groupComparativeResults();
    std::vector<std::pair<ComparableGameResult, std::vector<std::string>>> sortComparativeGroups(std::unordered_map<ComparableGameResult, std::vector<std::string>>&& grouped);

private:
    GameManagerExecutionResult runSingleComparativeGame(
        const registrations::GameManagerRegistrar::value_type& gm_entry,
        const TankAlgorithmFactory& tank_factory1,
        const TankAlgorithmFactory& tank_factory2,
        const std::string& algo1_name,
        const std::string& algo2_name,
        const GameMapInfo& map_info,
        bool verbose);
    GameMapInfo loadAndValidateMap();
    auto resolveAlgorithmFactories() -> std::pair<registrations::AlgorithmRegistrar::value_type, registrations::AlgorithmRegistrar::value_type>;

    std::vector<GameManagerExecutionResult> results_;
    size_t map_width_;
    size_t map_height_;
};

} // namespace runners
} // namespace simulator