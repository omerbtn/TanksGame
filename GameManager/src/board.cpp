#include "board.h"

#include <algorithm>
#include <fstream>
#include <iostream>

#include "Player.h"
#include "board_satellite_view.h"
#include "global_config.h"
#include "utils.h"


Board::Board() {}

std::vector<std::vector<Cell>>& Board::grid()
{
    return grid_;
}

const std::vector<std::vector<Cell>>& Board::getGrid() const
{
    return grid_;
}

TankAlgorithm* Board::getAlgorithm(int player_id, int tank_id)
{
    auto it = algorithms_.find(std::make_pair(player_id, tank_id));
    if (it != algorithms_.end())
    {
        return it->second.get();
    }

    return nullptr;
}

GameInfo Board::loadFromSatelliteView(size_t map_width, size_t map_height,
                                      const SatelliteView& map,
                                      size_t max_steps, size_t num_shells,
                                      Player& player1, Player& player2,
                                      TankAlgorithmFactory player1_tank_algo_factory,
                                      TankAlgorithmFactory player2_tank_algo_factory)
{
    width_ = map_width;
    height_ = map_height;

    std::vector<std::shared_ptr<Tank>> ordered_tanks;

    grid_.resize(width_, std::vector<Cell>(height_));

    for (size_t y = 0; y < height_; ++y)
    {
        for (size_t x = 0; x < width_; ++x)
        {
            Position pos{x, y};
            char ch = map.getObjectAt(x, y);
            switch (ch)
            {
            case '#':
            {
                grid_[x][y] = Cell(pos, std::make_shared<Wall>());
                break;
            }
            case '@':
            {
                grid_[x][y] = Cell(pos, std::make_shared<Mine>());
                break;
            }
            case '1':
                [[fallthrough]];
            case '2':
            {
                int player_index = ch - '0';
                std::shared_ptr<Tank> tank;
                auto direction = getSeedDirection(player_index);

                if (player_tanks_.find(player_index) == player_tanks_.end())
                {
                    auto& player = (player_index == 1) ? player1 : player2;
                    player_tanks_.emplace(player_index, std::make_pair(std::ref(player), std::vector<std::shared_ptr<Tank>>{}));
                }

                auto player_it = player_tanks_.find(player_index);
                size_t tank_index = player_it->second.second.size();
                tank = std::make_shared<Tank>(player_index, tank_index, pos, direction, num_shells);
                player_it->second.second.push_back(tank);

                ordered_tanks.emplace_back(tank);
                algorithms_[{player_index, tank_index}] =
                    (player_index == 1) ? player1_tank_algo_factory(player_index, tank_index) : player2_tank_algo_factory(player_index, tank_index);
                grid_[x][y] = Cell(pos, tank);
                break;
            }
            case '.':
                [[fallthrough]];
            case ' ':
            {
                grid_[x][y] = Cell(pos);
                break;
            }
            default:
            {
                // Unexpected character
                grid_[x][y] = Cell(pos);
                break;
            }
            }
        }
    }

    // Initialize prev_grid_ with the current grid state
    prev_grid_ = grid_;

    GameInfo game_info(width_, height_, max_steps, num_shells, std::move(ordered_tanks));

    return game_info;
}

void Board::print() const
{
    printGrid(grid_);
}

const std::shared_ptr<Tank> Board::getTank(int player_id, int tank_id) const
{
    if (auto it = player_tanks_.find(player_id); it != player_tanks_.end())
    {
        const auto& tanks = it->second.second;
        if (static_cast<size_t>(tank_id) < tanks.size())
        {
            return tanks[tank_id];
        }
    }

    return nullptr;
}

const std::vector<std::shared_ptr<Tank>>& Board::getPlayerTanks(int player_id) const
{
    static const std::vector<std::shared_ptr<Tank>> empty_vector;

    if (auto it = player_tanks_.find(player_id); it != player_tanks_.end())
    {
        return it->second.second;
    }

    return empty_vector; // Return empty vector if invalid player id
}

bool Board::executeTankAction(std::shared_ptr<Tank> tank, ActionRequest& action)
{
    if (!tank || !tank->isAlive())
    {
        if constexpr (config::get<bool>("verbose_debug"))
            std::cout << "[Board] Tank " << (tank ? (std::to_string(tank->tankId()) + " of Player " + std::to_string(tank->playerId())) : "")
                      << " is not alive or invalid." << std::endl;
        return false;
    }

    tank->decreaseCooldown();

    Position current_pos = tank->position();

    if (tank->waitingBackMove() && !tank->isBacking())
    {
        if (action == ActionRequest::MoveForward)
        {
            tank->resetBackwait();
            return true;
        }

        tank->setWaitingBackMove(false);
        action = ActionRequest::MoveBackward;
        return moveTankBackward(tank, current_pos);
    }

    switch (action)
    {
    case ActionRequest::MoveForward:
    {
        return moveTankForward(tank, current_pos);
    }

    case ActionRequest::MoveBackward:
    {
        return handleBackMovement(tank, current_pos);
    }

    case ActionRequest::RotateLeft45:
        [[fallthrough]];
    case ActionRequest::RotateRight45:
        [[fallthrough]];
    case ActionRequest::RotateLeft90:
        [[fallthrough]];
    case ActionRequest::RotateRight90:
    {
        return rotateTank(tank, action);
    }

    case ActionRequest::Shoot:
    {
        return shoot(tank, current_pos);
    }

    case ActionRequest::GetBattleInfo:
    {
        return getBattleInfo(tank);
    }

    case ActionRequest::DoNothing:
    {
        return doNothing(tank);
    }

    default:
        return false;
    }
}

bool Board::moveTankForward(std::shared_ptr<Tank> tank, const Position& current_pos)
{
    if constexpr (config::get<bool>("verbose_debug"))
        std::cout << "[Board] Executing MoveForward for Tank " << tank->tankId() << " of Player " << tank->playerId() << std::endl;

    if (tank->isBacking())
    {
        // Only move forward action is able to reset the back movement
        tank->resetBackwait();
        return true;
    }

    Position new_pos = forwardPosition(current_pos, tank->direction(), width_, height_);

    Cell& current_cell = grid_[current_pos.first][current_pos.second];
    Cell& new_cell = grid_[new_pos.first][new_pos.second];

    if (new_cell.has(ObjectType::Wall))
    {
        // Illegal move, can't move into walls
        return false;
    }

    new_cell.addObject(tank);
    current_cell.removeObject(tank);
    tank->position() = new_pos;
    cells_to_update_.insert(new_pos);

    // Store the old position of the tank
    // Should never be two tanks in the same position in a valid game state, so should be ok
    old_tanks_positions_[current_pos] = tank;

    return true;
}

bool Board::shoot(std::shared_ptr<Tank> tank, const Position& current_pos)
{
    if constexpr (config::get<bool>("verbose_debug"))
        std::cout << "[Board] Executing Shoot for Tank " << tank->tankId() << " of Player " << tank->playerId() << std::endl;

    tank->tickBackwait();

    if (tank->canShoot())
    {
        Position shell_pos = forwardPosition(current_pos, tank->direction(), width_, height_);
        auto& cell = grid_[shell_pos.first][shell_pos.second];
        tank->shoot();
        std::shared_ptr<Shell> shell = std::make_shared<Shell>(tank->direction());
        cell.addObject(shell);
        active_shells_.emplace_back(shell_pos, shell);
        cells_to_update_.insert(shell_pos);
        return true;
    }

    return false;
}

bool Board::getBattleInfo(std::shared_ptr<Tank> tank)
{
    if constexpr (config::get<bool>("verbose_debug"))
        std::cout << "[Board] Executing GetBattleInfo for Tank " << tank->tankId() << " of Player " << tank->playerId() << std::endl;

    bool is_backing = tank->isBacking();
    tank->tickBackwait();
    if (is_backing)
    {
        return false;
    }

    auto player_it = player_tanks_.find(tank->playerId());
    if (player_it == player_tanks_.end())
    {
        // Player not found, return false
        if constexpr (config::get<bool>("verbose_debug"))
            std::cerr << "[Board] Player " << tank->playerId() << " not found for GetBattleInfo." << std::endl;
        return false;
    }

    // Provide satellite view to the player
    BoardSatelliteView satelliteView(prev_grid_, tank->position());
    auto algorithm = getAlgorithm(tank->playerId(), tank->tankId());
    if (!algorithm)
    {
        // Algorithm not found, return false
        if constexpr (config::get<bool>("verbose_debug"))
            std::cerr << "[Board] Algorithm not found for Player " << tank->playerId()
                      << " Tank " << tank->tankId() << " for GetBattleInfo." << std::endl;
        return false;
    }

    Player& player = player_it->second.first;
    player.updateTankWithBattleInfo(*algorithm, satelliteView);

    return true;
}

bool Board::doNothing(std::shared_ptr<Tank> tank)
{
    if constexpr (config::get<bool>("verbose_debug"))
        std::cout << "[Board] Executing DoNothing for Tank " << tank->tankId() << " of Player " << tank->playerId() << std::endl;

    bool is_backing = tank->isBacking();
    tank->tickBackwait();
    return !is_backing;
}

bool Board::moveTankBackward(std::shared_ptr<Tank> tank, const Position& current_pos)
{
    Position new_pos = backwardPosition(tank->position(), tank->direction(), width_, height_);

    Cell& current_cell = grid_[current_pos.first][current_pos.second];
    Cell& new_cell = grid_[new_pos.first][new_pos.second];

    if (new_cell.has(ObjectType::Wall))
    {
        // Illegal move, can't move into walls
        return false;
    }

    new_cell.addObject(tank);
    current_cell.removeObject(tank);
    tank->position() = new_pos;
    cells_to_update_.insert(new_pos);

    // Store the old position of the tank
    // Should never be two tanks in the same position in a valid game state, so should be ok
    old_tanks_positions_[current_pos] = tank;

    return true;
}

bool Board::handleBackMovement(std::shared_ptr<Tank> tank, const Position& current_pos)
{
    if constexpr (config::get<bool>("verbose_debug"))
        std::cout << "[Board] Executing MoveBackward for Tank " << tank->tankId() << " of Player " << tank->playerId() << std::endl;

    if (!tank->isBacking() && tank->lastAction() != ActionRequest::MoveBackward)
    {
        tank->startBackwait();
        tank->setWaitingBackMove(true);
        return true;
    }
    else
    {
        if (tank->lastAction() != ActionRequest::MoveBackward)
        {
            tank->tickBackwait(); // keep counting
        }

        if (tank->readyToMoveBack())
        {
            return moveTankBackward(tank, current_pos);
        }

        if (tank->lastAction() == ActionRequest::MoveBackward)
        {
            tank->tickBackwait();
        }
    }

    return false; // still waiting (Asking moving back while waiting for a move back should be ignored)
}

bool Board::rotateTank(std::shared_ptr<Tank> tank, ActionRequest action)
{
    if constexpr (config::get<bool>("verbose_debug"))
        std::cout << "[Board] Executing " << tankActionToString(action) << " for Tank " << tank->tankId() << " of Player " << tank->playerId() << std::endl;

    bool is_backing = tank->isBacking();
    tank->tickBackwait();
    if (is_backing)
    {
        return false;
    }

    tank->direction() = getDirectionAfterRotation(tank->direction(), action);
    return true;
}

// Moves all the shells one step forward, does not resolve collisions (besides crossing shells)
void Board::updateActiveShells()
{
    std::vector<std::tuple<Position, Position, std::shared_ptr<Shell>>> moves; // (from, to, shell)

    // First pass: prepeare moves
    for (const auto& [pos, shell] : active_shells_)
    {
        Position new_pos = forwardPosition(pos, shell->direction(), width_, height_);
        moves.emplace_back(pos, new_pos, shell);
    }

    std::unordered_set<std::shared_ptr<Shell>> shells_to_remove;

    // Check for collisions (crossing shells)
    for (size_t i = 0; i < moves.size(); ++i)
    {
        for (size_t j = i + 1; j < moves.size(); ++j)
        {
            auto& [from1, to1, shell1] = moves[i];
            auto& [from2, to2, shell2] = moves[j];

            if (to1 == from2 && to2 == from1)
            {
                // Shells are crossing each other, should mark as collision
                shells_to_remove.insert(shell1);
                shells_to_remove.insert(shell2);
            }
        }
    }

    // Second pass: move shells on the board, excluding removed shells
    for (auto& [from, to, shell] : moves)
    {
        grid_[from.first][from.second].removeObject(shell);
        if (shells_to_remove.count(shell) == 0)
        {
            grid_[to.first][to.second].addObject(shell);
            cells_to_update_.insert(to);

            // Update the position in active_shells_
            auto it = std::find_if(active_shells_.begin(), active_shells_.end(), [&shell](const auto& p)
                                   { return p.second == shell; });
            if (it != active_shells_.end())
            {
                it->first = to; // update position
            }
        }
        // Else: crossing shell, should be removed
    }

    // Remove crossing shells from the active shells list
    active_shells_.erase(std::remove_if(active_shells_.begin(), active_shells_.end(),
                                        [&shells_to_remove](const auto& pair)
                                        {
                                            const auto& shell = pair.second;
                                            return shells_to_remove.count(shell) > 0;
                                        }),
                         active_shells_.end());
}

void Board::resolveCollisions(Cell& cell)
{
    if ((((cell.has(ObjectType::Shell) && !cell.has(ObjectType::Mine)) ||
          (!cell.has(ObjectType::Shell) && cell.has(ObjectType::Mine))) &&
         cell.getObjectsCount() > 1) ||
        (cell.has(ObjectType::Shell) && cell.has(ObjectType::Mine) && cell.getObjectsCount() > 2))
    {
        // If there's shell or mine and at least one another object, we have an explosion
        // If there're both and at least one another object, we have an explosion
        // Want to allow shell and mine to be in the same cell without exploding
        onExplosion(cell);
    }

    else if (cell.getObjectsByType(ObjectType::Tank).size() > 1)
    {
        // Two (or more) tanks collided, all are destroyed
        for (auto& tank : cell.getObjectsByType(ObjectType::Tank))
        {
            auto tank_ptr = std::static_pointer_cast<Tank>(tank);
            tank_ptr->destroy();
        }

        // Remove all tanks from the cell
        cell.removeObjectsByType(ObjectType::Tank);
    }
}

void Board::onExplosion(Cell& cell)
{
    // We have an explosion, all the objects must get hurt

    std::vector<std::shared_ptr<GameObjectInterface>> objects_to_remove;

    // Weaken wall if exists
    if (cell.has(ObjectType::Wall))
    {
        for (auto& wall : cell.getObjectsByType(ObjectType::Wall))
        {
            auto wall_ptr = std::static_pointer_cast<Wall>(wall);
            wall_ptr->weaken();
            if (wall_ptr->isDestroyed())
            {
                objects_to_remove.push_back(wall);
            }
        }
    }

    // Destroy all tanks
    if (cell.has(ObjectType::Tank))
    {
        for (auto& tank : cell.getObjectsByType(ObjectType::Tank))
        {
            auto tank_ptr = std::static_pointer_cast<Tank>(tank);
            tank_ptr->destroy();
            objects_to_remove.push_back(tank);
        }
    }

    // Destroy all shells
    if (cell.has(ObjectType::Shell))
    {
        for (auto& shell : cell.getObjectsByType(ObjectType::Shell))
        {
            auto shell_ptr = std::static_pointer_cast<Shell>(shell);

            // Remove the shell from the active shells list
            Position shell_position = cell.position();
            auto it = std::find_if(active_shells_.begin(), active_shells_.end(),
                                   [&shell_position, &shell_ptr](const std::pair<Position, std::shared_ptr<Shell>>& active_shell)
                                   {
                                       return active_shell.first == shell_position && active_shell.second == shell_ptr;
                                   });
            if (it != active_shells_.end())
            {
                active_shells_.erase(it);
            }

            // Mark for removal from the cell
            objects_to_remove.push_back(shell);
        }
    }

    // Destroy mine if exists
    if (cell.has(ObjectType::Mine))
    {
        for (auto& mine : cell.getObjectsByType(ObjectType::Mine))
        {
            objects_to_remove.push_back(mine);
        }
    }

    // Remove all collected objects safely, to avoid invalidating iterators
    for (auto& object : objects_to_remove)
    {
        cell.removeObject(object);
    }
}

void Board::doShellsStep(bool shells_only)
{
    // Move all the shells one step forward
    updateActiveShells();

    // Update the board and resolve collisions
    update();

    if (shells_only)
    {
        // Update the previous grid with the current grid
        // Only in shells-only step, for saving the previous turn state for GetBattleInfo
        prev_grid_ = grid_;
    }
}

void Board::update()
{
    // Check for crossing tanks
    for (auto it1 = old_tanks_positions_.begin(); it1 != old_tanks_positions_.end(); ++it1)
    {
        for (auto it2 = std::next(it1); it2 != old_tanks_positions_.end(); ++it2)
        {
            Position old_pos1 = it1->first;
            auto tank1 = it1->second;

            Position old_pos2 = it2->first;
            auto tank2 = it2->second;

            if (tank1->position() == old_pos2 && tank2->position() == old_pos1)
            {
                // Tanks are crossing each other, both should be destroyed
                tank1->destroy();
                grid_[tank1->position().first][tank1->position().second].removeObject(tank1);

                tank2->destroy();
                grid_[tank2->position().first][tank2->position().second].removeObject(tank2);
            }
        }
    }

    // Clear the old tanks positions for the next turn
    old_tanks_positions_.clear();

    // Resolve collisions for all the cells from this turn
    for (auto& cell_pos : cells_to_update_)
    {
        resolveCollisions(grid_[cell_pos.first][cell_pos.second]);
    }

    // Clear the cells to update set for the next turn
    cells_to_update_.clear();
}

const Cell& Board::getCell(Position position) const
{
    int x = position.first % width_;
    int y = position.second % height_;
    return grid_[x][y];
}

size_t Board::getHeight() const
{
    return height_;
}

size_t Board::getWidth() const
{
    return width_;
}
