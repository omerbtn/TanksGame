#include <gtest/gtest.h>
#include "board.h"
#include "tank.h"
#include "wall.h"
#include "mine.h"
#include "cell.h"
#include "game_manager.h"
#include "concrete_player_factory.h"
#include "concrete_tank_algorithm_factory.h"

class BoardTest : public ::testing::Test 
{
protected:
    ConcretePlayerFactory playerFactory_;
    ConcreteTankAlgorithmFactory algorithmFactory_;
    Board board;

    BoardTest()
        : playerFactory_(ConcretePlayerFactory()),
          algorithmFactory_(ConcreteTankAlgorithmFactory()),
          board(playerFactory_, algorithmFactory_)
    {}

    void SetUp() override {
        ASSERT_TRUE(board.load_from_file("../test/board.txt").is_valid);
    }
};

TEST_F(BoardTest, TankStepsOnMineAndDies) 
{
    auto tank = board.get_tank(1, 0);  // Get the first tank of player 1

    // Place tank next to a mine
    tank->position() = std::make_pair(2, 2);  // Assume (2,2) has a mine nearby at (2,3)
    board.grid()[2][2] = Cell({2, 2}, tank);

    // Move forward into the mine
    tank->direction() = Direction::R;
    bool result = board.execute_tank_action(tank, ActionRequest::MoveForward);
    board.update();

    EXPECT_TRUE(result);
    EXPECT_FALSE(tank->is_alive());  // Tank should be dead after stepping on mine
}

TEST_F(BoardTest, TankCannotPassThroughWall) 
{
    auto tank = board.get_tank(2, 0);  // Get the first tank of player 2

    tank->position() = std::make_pair(5, 0);
    board.grid()[5][0] = Cell({5, 0}, tank);

    tank->direction() = Direction::L;
    bool result = board.execute_tank_action(tank, ActionRequest::MoveForward);

    EXPECT_FALSE(result);
    EXPECT_TRUE(tank->is_alive());                // Tank is still alive
    EXPECT_EQ(tank->position(), Position(5, 0));  // Didn't move
}

TEST_F(BoardTest, TankShootsAndHitsOtherTank) 
{
    auto tank1 = board.get_tank(1, 0);  // Get the first tank of player 1
    auto tank2 = board.get_tank(2, 0);  // Get the first tank of player 2

    // Place tanks facing each other
    tank1->position() = std::make_pair(4, 5);
    tank2->position() = std::make_pair(5, 5);
    tank1->direction() = Direction::R;
    tank2->direction() = Direction::L;

    board.grid()[4][5] = Cell({4, 5}, tank1);
    board.grid()[5][5] = Cell({5, 5}, tank2);

    // Tank1 shoots
    bool result = board.execute_tank_action(tank1, ActionRequest::Shoot);
    board.update();

    EXPECT_TRUE(result);
    EXPECT_FALSE(tank2->is_alive());  // Tank2 should be destroyed
}

TEST_F(BoardTest, TankMovesForwardCorrectly) 
{
    auto tank = board.get_tank(1, 0);  // Get the first tank of player 1
    Position oldPos = tank->position();

    board.execute_tank_action(tank, ActionRequest::MoveForward);

    EXPECT_NE(tank->position(), oldPos);
}

TEST_F(BoardTest, TankStartsBackwardMoveWithDelay) 
{
    auto tank = board.get_tank(1, 0);  // Get the first tank of player 1
    Position oldPos = tank->position();

    board.execute_tank_action(tank, ActionRequest::MoveBackward);
    EXPECT_TRUE(tank->is_backing());
    EXPECT_EQ(tank->position(), oldPos);

    board.execute_tank_action(tank, ActionRequest::MoveBackward);
    EXPECT_EQ(tank->position(), oldPos);
    board.execute_tank_action(tank, ActionRequest::MoveBackward);
    EXPECT_NE(tank->position(), oldPos);  // Now it moved
}

TEST_F(BoardTest, TankRotateEighthLeft) 
{
    auto tank = board.get_tank(2, 0);  // Get the first tank of player 2
    Direction oldDir = tank->direction();

    board.execute_tank_action(tank, ActionRequest::RotateLeft45);

    EXPECT_EQ(static_cast<int>(tank->direction()), (static_cast<int>(oldDir) + 7) % 8);
}

TEST_F(BoardTest, TankRotateQuarterRight) 
{
    auto tank = board.get_tank(2, 0);  // Get the first tank of player 2
    Direction oldDir = tank->direction();

    board.execute_tank_action(tank, ActionRequest::RotateRight90);

    EXPECT_EQ(static_cast<int>(tank->direction()), (static_cast<int>(oldDir) + 2) % 8);
}

TEST_F(BoardTest, TankShootCooldownPreventsImmediateSecondShot) 
{
    auto tank = board.get_tank(1, 0);  // Get the first tank of player 1

    board.execute_tank_action(tank, ActionRequest::Shoot);
    bool secondShot = board.execute_tank_action(tank, ActionRequest::Shoot);

    EXPECT_FALSE(secondShot);  // Shooting should be blocked
}

TEST_F(BoardTest, TankCanShootAgainAfterCooldown) 
{
    auto tank = board.get_tank(1, 0);  // Get the first tank of player 1

    board.execute_tank_action(tank, ActionRequest::Shoot);
    for (int i = 0; i < 4; ++i) {
        board.execute_tank_action(tank, ActionRequest::DoNothing);
    }

    bool secondShot = board.execute_tank_action(tank, ActionRequest::Shoot);

    EXPECT_TRUE(secondShot);
}

TEST_F(BoardTest, ShellMovesTwoStepsPerTick) 
{
    auto tank = board.get_tank(1, 0);  // Get the first tank of player 1
    tank->direction() = Direction::R;

    board.execute_tank_action(tank, ActionRequest::Shoot);

    Position oldPos;
    for (size_t x = 0; x < board.get_height(); ++x) 
    {
        for (size_t y = 0; y < board.get_width(); ++y) 
        {
            const auto& cell = board.get_cell(Position(x, y));
            if (cell.has(ObjectType::Shell)) 
            {
                oldPos = cell.position();
                break;
            }
        }
    }

    // To be called every half step, so call twice
    board.do_shells_step();  // Move shells one step forward
    board.do_shells_step();  // Move shells another step forward

    Position newPos;
    for (size_t x = 0; x < board.get_height(); ++x) 
    {
        for (size_t y = 0; y < board.get_width(); ++y) 
        {
            const auto& cell = board.get_cell(Position(x, y));
            if (cell.has(ObjectType::Shell)) 
            {
                newPos = cell.position();
                break;
            }
        }
    }

    EXPECT_EQ(newPos.first, oldPos.first + 2);
    EXPECT_EQ(newPos.second, oldPos.second);
}

TEST_F(BoardTest, WallDestroyedAfterTwoShellHits) 
{
    Position wallPos = {5, 5};
    board.grid()[wallPos.second][wallPos.first].add_object(std::make_shared<Wall>());

    auto tank = board.get_tank(1, 0);  // Get the first tank of player 1
    tank->position() = std::make_pair(4, 5);
    tank->direction() = Direction::R;

    board.execute_tank_action(tank, ActionRequest::Shoot);
    board.update();
    board.execute_tank_action(tank, ActionRequest::DoNothing);
    board.execute_tank_action(tank, ActionRequest::DoNothing);
    board.execute_tank_action(tank, ActionRequest::DoNothing);
    board.execute_tank_action(tank, ActionRequest::Shoot);
    board.update();

    EXPECT_FALSE(board.grid()[wallPos.second][wallPos.first].has(ObjectType::Wall));
}

TEST_F(BoardTest, ShellCollisionDestroysBothShells) 
{
    auto tank1 = board.get_tank(1, 0);  // Get the first tank of player 1
    auto tank2 = board.get_tank(2, 0);  // Get the first tank of player 2

    tank1->position() = std::make_pair(4, 5);
    tank2->position() = std::make_pair(6, 5);
    tank1->direction() = Direction::R;
    tank2->direction() = Direction::L;

    board.execute_tank_action(tank1, ActionRequest::Shoot);
    board.execute_tank_action(tank2, ActionRequest::Shoot);

    board.update();
    board.update();

    for (size_t x = 0; x < board.get_height(); ++x) 
    {
        for (size_t y = 0; y < board.get_width(); ++y) 
        {
            const auto& cell = board.get_cell(Position(x,y));
            ASSERT_FALSE(cell.has(ObjectType::Shell)); // Validate no shells remaining.
        }
    }
}

TEST_F(BoardTest, TankWrapsAroundBoardEdges) 
{
    auto tank = board.get_tank(1, 0);  // Get the first tank of player 1
    tank->position() = std::make_pair(9, 9);  // Right edge
    tank->direction() = Direction::R;

    board.execute_tank_action(tank, ActionRequest::MoveForward);

    EXPECT_EQ(tank->position().first, 0);  // Wrapped around horizontally
    EXPECT_EQ(tank->position().second, 9);
}

// Should fix this test, GameManager ctor no longer takes a Board
/*
TEST_F(BoardTest, BothTanksDestroyedLeadsToTie) 
{
    GameManager gm(&board);

    auto& player1 = board.players()[1];
    auto& player2 = board.players()[2];

    auto tank1 = player1.tank();
    auto tank2 = player2.tank();

    tank1->destroy();
    tank2->destroy();

    EXPECT_TRUE(gm.is_game_over());
}
*/