#include <gtest/gtest.h>

#include "board.h"
#include "cell.h"
#include "concrete_player_factory.h"
#include "concrete_tank_algorithm_factory.h"
#include "game_manager.h"
#include "mine.h"
#include "tank.h"
#include "wall.h"


class BoardTest : public ::testing::Test
{
protected:
    ConcretePlayerFactory playerFactory_;
    ConcreteTankAlgorithmFactory algorithmFactory_;
    Board board;

    BoardTest()
        : playerFactory_(ConcretePlayerFactory()),
          algorithmFactory_(ConcreteTankAlgorithmFactory()),
          board(playerFactory_, algorithmFactory_) {}

    void SetUp() override
    {
        ASSERT_TRUE(board.loadFromFile("../test/board.txt").is_valid);
    }
};

TEST_F(BoardTest, TankStepsOnMineAndDies)
{
    auto tank = board.getTank(1, 0); // Get the first tank of player 1

    // Place tank next to a mine
    tank->position() = std::make_pair(2, 2); // Assume (2,2) has a mine nearby at (2,3)
    board.grid()[2][2] = Cell({2, 2}, tank);

    // Move forward into the mine
    tank->direction() = Direction::R;
    auto action = ActionRequest::MoveForward;
    bool result = board.executeTankAction(tank, action);
    board.update();

    EXPECT_TRUE(result);
    EXPECT_FALSE(tank->isAlive()); // Tank should be dead after stepping on mine
}

TEST_F(BoardTest, TankCannotPassThroughWall)
{
    auto tank = board.getTank(2, 0); // Get the first tank of player 2

    tank->position() = std::make_pair(5, 0);
    board.grid()[5][0] = Cell({5, 0}, tank);

    tank->direction() = Direction::L;
    auto action = ActionRequest::MoveForward;
    bool result = board.executeTankAction(tank, action);

    EXPECT_FALSE(result);
    EXPECT_TRUE(tank->isAlive());                // Tank is still alive
    EXPECT_EQ(tank->position(), Position(5, 0)); // Didn't move
}

TEST_F(BoardTest, TankShootsAndHitsOtherTank)
{
    auto tank1 = board.getTank(1, 0); // Get the first tank of player 1
    auto tank2 = board.getTank(2, 0); // Get the first tank of player 2

    // Place tanks facing each other
    tank1->position() = std::make_pair(4, 5);
    tank2->position() = std::make_pair(5, 5);
    tank1->direction() = Direction::R;
    tank2->direction() = Direction::L;

    board.grid()[4][5] = Cell({4, 5}, tank1);
    board.grid()[5][5] = Cell({5, 5}, tank2);

    // Tank1 shoots
    auto action = ActionRequest::Shoot;
    bool result = board.executeTankAction(tank1, action);
    board.update();

    EXPECT_TRUE(result);
    EXPECT_FALSE(tank2->isAlive()); // Tank2 should be destroyed
}

TEST_F(BoardTest, TankMovesForwardCorrectly)
{
    auto tank = board.getTank(1, 0); // Get the first tank of player 1
    Position oldPos = tank->position();

    auto action = ActionRequest::MoveForward;
    board.executeTankAction(tank, action);

    EXPECT_NE(tank->position(), oldPos);
}

TEST_F(BoardTest, TankStartsBackwardMoveWithDelay)
{
    auto tank = board.getTank(1, 0); // Get the first tank of player 1
    Position oldPos = tank->position();

    auto action = ActionRequest::MoveBackward;
    board.executeTankAction(tank, action);
    tank->setLastAction(ActionRequest::MoveBackward);
    EXPECT_TRUE(tank->isBacking());
    EXPECT_EQ(tank->position(), oldPos);

    board.executeTankAction(tank, action);
    tank->setLastAction(ActionRequest::MoveBackward);
    EXPECT_EQ(tank->position(), oldPos);
    board.executeTankAction(tank, action);
    tank->setLastAction(ActionRequest::MoveBackward);
    EXPECT_NE(tank->position(), oldPos); // Now it moved
}

TEST_F(BoardTest, TankRotateEighthLeft)
{
    auto tank = board.getTank(2, 0); // Get the first tank of player 2
    Direction oldDir = tank->direction();

    auto action = ActionRequest::RotateLeft45;
    board.executeTankAction(tank, action);

    EXPECT_EQ(static_cast<int>(tank->direction()), (static_cast<int>(oldDir) + 7) % 8);
}

TEST_F(BoardTest, TankRotateQuarterRight)
{
    auto tank = board.getTank(2, 0); // Get the first tank of player 2
    Direction oldDir = tank->direction();

    auto action = ActionRequest::RotateRight90;
    board.executeTankAction(tank, action);

    EXPECT_EQ(static_cast<int>(tank->direction()), (static_cast<int>(oldDir) + 2) % 8);
}

TEST_F(BoardTest, TankShootCooldownPreventsImmediateSecondShot)
{
    auto tank = board.getTank(1, 0); // Get the first tank of player 1

    auto action = ActionRequest::Shoot;
    board.executeTankAction(tank, action);
    bool secondShot = board.executeTankAction(tank, action);

    EXPECT_FALSE(secondShot); // Shooting should be blocked
}

TEST_F(BoardTest, TankCanShootAgainAfterCooldown)
{
    auto tank = board.getTank(1, 0); // Get the first tank of player 1

    auto action = ActionRequest::Shoot;
    board.executeTankAction(tank, action);
    for (int i = 0; i < 4; ++i)
    {
        auto action = ActionRequest::DoNothing;
        board.executeTankAction(tank, action);
    }

    bool secondShot = board.executeTankAction(tank, action);

    EXPECT_TRUE(secondShot);
}

TEST_F(BoardTest, ShellMovesTwoStepsPerTick)
{
    auto tank = board.getTank(1, 0); // Get the first tank of player 1
    tank->direction() = Direction::R;

    auto action = ActionRequest::Shoot;
    board.executeTankAction(tank, action);

    Position oldPos;
    for (size_t x = 0; x < board.getHeight(); ++x)
    {
        for (size_t y = 0; y < board.getWidth(); ++y)
        {
            const auto& cell = board.getCell(Position(x, y));
            if (cell.has(ObjectType::Shell))
            {
                oldPos = cell.position();
                break;
            }
        }
    }

    // To be called every half step, so call twice
    board.doShellsStep(); // Move shells one step forward
    board.doShellsStep(); // Move shells another step forward

    Position newPos;
    for (size_t x = 0; x < board.getHeight(); ++x)
    {
        for (size_t y = 0; y < board.getWidth(); ++y)
        {
            const auto& cell = board.getCell(Position(x, y));
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
    board.grid()[wallPos.second][wallPos.first].addObject(std::make_shared<Wall>());

    auto tank = board.getTank(1, 0); // Get the first tank of player 1
    tank->position() = std::make_pair(4, 5);
    tank->direction() = Direction::R;

    auto shoot = ActionRequest::Shoot;
    board.executeTankAction(tank, shoot);
    board.update();
    auto do_nothing = ActionRequest::DoNothing;
    board.executeTankAction(tank, do_nothing);
    board.executeTankAction(tank, do_nothing);
    board.executeTankAction(tank, do_nothing);
    board.executeTankAction(tank, shoot);
    board.update();

    EXPECT_FALSE(board.grid()[wallPos.second][wallPos.first].has(ObjectType::Wall));
}

TEST_F(BoardTest, ShellCollisionDestroysBothShells)
{
    auto tank1 = board.getTank(1, 0); // Get the first tank of player 1
    auto tank2 = board.getTank(2, 0); // Get the first tank of player 2

    tank1->position() = std::make_pair(4, 5);
    tank2->position() = std::make_pair(6, 5);
    tank1->direction() = Direction::R;
    tank2->direction() = Direction::L;

    auto shoot = ActionRequest::Shoot;
    board.executeTankAction(tank1, shoot);
    board.executeTankAction(tank2, shoot);

    board.update();
    board.update();

    for (size_t x = 0; x < board.getHeight(); ++x)
    {
        for (size_t y = 0; y < board.getWidth(); ++y)
        {
            const auto& cell = board.getCell(Position(x, y));
            ASSERT_FALSE(cell.has(ObjectType::Shell)); // Validate no shells remaining.
        }
    }
}

TEST_F(BoardTest, TankWrapsAroundBoardEdges)
{
    auto tank = board.getTank(1, 0);         // Get the first tank of player 1
    tank->position() = std::make_pair(9, 9); // Right edge
    tank->direction() = Direction::R;

    auto action = ActionRequest::MoveForward;
    board.executeTankAction(tank, action);

    EXPECT_EQ(tank->position().first, 0); // Wrapped around horizontally
    EXPECT_EQ(tank->position().second, 9);
}
