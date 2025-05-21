#include "user_algorithm.h"

#include <string>
#include <iostream>
#include <unordered_map>
#include <algorithm>

static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\n\r");
    auto end = s.find_last_not_of(" \t\n\r");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

static void print_help() {
    std::cout << "\n[Command Help] Available Actions:\n"
              << "  f / forward        = Move Forward\n"
              << "  b / back           = Move Backward\n"
              << "  l / left90         = Rotate Left 90°\n"
              << "  r / right90        = Rotate Right 90°\n"
              << "  l45 / left45       = Rotate Left 45°\n"
              << "  r45 / right45      = Rotate Right 45°\n"
              << "  s / shoot          = Shoot\n"
              << "  i / info           = Get Battle Info\n"
              << "  x / skip / nothing = Do Nothing\n"
              << "  help               = Show this help menu\n";
}

ActionRequest UserAlgorithm::getAction() {
    print_help();

    while (true) {
        std::cout << "\n[Action] Type command for Player " << player_index_ << ", Tank " << tank_index_ << " (type 'help' to list options): ";
        std::string input;
        std::getline(std::cin, input);
        std::transform(input.begin(), input.end(), input.begin(), [](char c) { return std::tolower(c); });
        input = trim(input);

        if (input == "help") {
            print_help();
            continue;
        }

        static const std::unordered_map<std::string, ActionRequest> input_map = {
            {"f", ActionRequest::MoveForward},     {"forward", ActionRequest::MoveForward},
            {"b", ActionRequest::MoveBackward},    {"back", ActionRequest::MoveBackward},
            {"l", ActionRequest::RotateLeft90},    {"left90", ActionRequest::RotateLeft90},
            {"r", ActionRequest::RotateRight90},   {"right90", ActionRequest::RotateRight90},
            {"l45", ActionRequest::RotateLeft45},  {"left45", ActionRequest::RotateLeft45},
            {"r45", ActionRequest::RotateRight45}, {"right45", ActionRequest::RotateRight45},
            {"s", ActionRequest::Shoot},           {"shoot", ActionRequest::Shoot},
            {"i", ActionRequest::GetBattleInfo},   {"info", ActionRequest::GetBattleInfo},
            {"x", ActionRequest::DoNothing},       {"skip", ActionRequest::DoNothing},
            {"nothing", ActionRequest::DoNothing}};

        auto it = input_map.find(input);
        if (it != input_map.end()) {
            return it->second;
        }

        std::cout << "[Error] Unknown command: '" << input << "'. Type 'help' to see valid options.\n";
    }
}

void UserAlgorithm::updateBattleInfo(BattleInfo&) {
    // No implementation needed for user algorithm
    // This function is just a placeholder to satisfy the interface
    std::cout << "You see the map all the time! Don't waste turns on this ;)\n";
}
