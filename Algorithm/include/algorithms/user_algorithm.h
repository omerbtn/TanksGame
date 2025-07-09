#pragma once

#include "TankAlgorithm.h"


namespace Algorithm_322573304_322647603
{

// Algorithm that allows the user to control his tanks manually.
// Used for testing purposes.
class UserAlgorithm : public TankAlgorithm
{
public:
    virtual ~UserAlgorithm() = default;
    UserAlgorithm(int player_index, int tank_index)
        : player_index_(player_index), tank_index_(tank_index) {}

    UserAlgorithm(const UserAlgorithm&) = delete;
    UserAlgorithm& operator=(const UserAlgorithm&) = delete;

    UserAlgorithm(UserAlgorithm&&) = delete;
    UserAlgorithm& operator=(UserAlgorithm&&) = delete;

    virtual ActionRequest getAction() override;
    void updateBattleInfo(BattleInfo&) override;

private:
    static std::string trim(const std::string& s);
    static void print_help();

private:
    int player_index_;
    int tank_index_;
};

} // namespace Algorithm_322573304_322647603