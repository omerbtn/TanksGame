#pragma once

#include "runner.h"
#include "loader/shared_object_loader.h"

namespace simulator
{
namespace runners
{

class SingleModeRunner : public Runner {
public:
    virtual ~SingleModeRunner();
    explicit SingleModeRunner(SimulatorConfig config);
    SingleModeRunner(const SingleModeRunner&) = delete;
    SingleModeRunner& operator=(const SingleModeRunner&) = delete;
    SingleModeRunner(SingleModeRunner&&) = delete;
    SingleModeRunner& operator=(SingleModeRunner&&) = delete;

    void loadSharedObjects() override;
    void prepare() override;
    void run() override;
    void printResults() override;
};

} // namespace runners
} // namespace simulator