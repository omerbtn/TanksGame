#pragma once

#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

enum class RunMode
{
    COMPARATIVE,
    COMPETITION
};

struct SimulatorConfig
{
    RunMode mode;
    std::string game_map_filename;
    std::string game_maps_folder;
    std::string game_managers_folder;
    std::string game_manager_so;
    std::string algorithm1_so;
    std::string algorithm2_so;
    std::string algorithms_folder;
    int num_threads = 1;
    bool verbose = false;
};


class ArgumentsParser
{
public:
    ArgumentsParser(int argc, char* argv[]);
    SimulatorConfig getConfig() const;

    static void printUsage(std::ostream& os, const std::string& error_msg = "");

private:
    void parse(int argc, char* argv[]);
    void validate();
    int parseKeyValueTokens(const std::vector<std::string>& tokens, size_t start_idx, std::string& key, std::string& value) const;
    std::vector<std::string> getUnrecognizedKeys(const std::vector<std::string>& required_keys) const;
    std::vector<std::string> getMissingKeys(const std::vector<std::string>& required_keys) const;

    static const std::vector<std::string> comparative_required_keys;
    static const std::vector<std::string> competition_required_keys;
    static const std::vector<std::string> optional_keys;

    std::unordered_map<std::string, std::string> args_;
    std::vector<std::string> unsupported_args_;
    RunMode mode_;
    bool verbose_ = false;
};
