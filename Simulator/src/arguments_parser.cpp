#include "arguments_parser.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <set>
#include <sstream>

#include "simulator_exception.h"


const std::vector<std::string> ArgumentsParser::comparative_required_keys = {
    "game_map", "game_managers_folder", "algorithm1", "algorithm2"};

const std::vector<std::string> ArgumentsParser::competition_required_keys = {
    "game_maps_folder", "game_manager", "algorithms_folder"};

const std::vector<std::string> ArgumentsParser::optional_keys = {
    "num_threads"};

ArgumentsParser::ArgumentsParser(int argc, char* argv[])
{
    parse(argc, argv);
    validate();
}

void ArgumentsParser::parse(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::ostringstream oss;
        printUsage(oss, "Missing mode argument (-comparative or -competition)");
        throw SimulatorException(oss.str());
    }

    std::string mode_arg = argv[1];
    if (mode_arg == "-comparative")
    {
        mode_ = RunMode::COMPARATIVE;
    }
    else if (mode_arg == "-competition")
    {
        mode_ = RunMode::COMPETITION;
    }
    else
    {
        std::ostringstream oss;
        printUsage(oss, "First argument must be -comparative or -competition");
        throw SimulatorException(oss.str());
    }

    std::vector<std::string> tokens;
    for (int i = 2; i < argc; ++i)
    {
        tokens.emplace_back(argv[i]);
    }

    size_t i = 0;
    while (i < tokens.size())
    {
        std::string token = tokens[i];

        if (token == "-verbose")
        {
            verbose_ = true;
            ++i;
            continue;
        }

        std::string key, value;
        int next_i = parseKeyValueTokens(tokens, i, key, value);
        if (next_i > 0)
        {
            args_[key] = value;
            i = static_cast<size_t>(next_i);
        }
        else
        {
            unsupported_args_.push_back(token);
            ++i;
        }
    }
}

int ArgumentsParser::parseKeyValueTokens(const std::vector<std::string>& tokens, size_t start_idx, std::string& key, std::string& value) const
{
    if (start_idx >= tokens.size())
        return -1;

    const std::string& token1 = tokens[start_idx];
    size_t eq_pos = token1.find('=');

    if (eq_pos != std::string::npos)
    {
        key = token1.substr(0, eq_pos);
        value = token1.substr(eq_pos + 1);

        // Case 1: key=value (single token)
        if (!value.empty())
        {
            if (key.empty())
                return -1; // Invalid key
            return start_idx + 1;
        }
        // Case 2: key= value (value in next token)
        else if (start_idx + 1 < tokens.size())
        {
            value = tokens[start_idx + 1];
            if (key.empty() || value.empty())
                return -1;
            return start_idx + 2;
        }
        else
        {
            return -1; // No value token available
        }
    }

    // Check if this could be "key =value" or "key = value" pattern
    if (start_idx + 1 < tokens.size())
    {
        const std::string& token2 = tokens[start_idx + 1];

        // Case 3: key = value (separate tokens)
        if (token2 == "=" && start_idx + 2 < tokens.size())
        {
            key = token1;
            value = tokens[start_idx + 2];
            if (key.empty() || value.empty())
                return -1;
            return start_idx + 3;
        }

        // Case 4: key =value (next token starts with '=')
        if (!token2.empty() && token2[0] == '=')
        {
            key = token1;
            value = token2.substr(1);
            if (key.empty() || value.empty())
                return -1;
            return start_idx + 2;
        }
    }

    return -1; // Not a valid key=value pattern
}

void ArgumentsParser::validate()
{
    const auto& required = (mode_ == RunMode::COMPARATIVE) ? comparative_required_keys : competition_required_keys;

    auto missing = getMissingKeys(required);
    auto unrecognized = getUnrecognizedKeys(required);
    unrecognized.insert(unrecognized.end(), unsupported_args_.begin(), unsupported_args_.end());

    if (!missing.empty() || !unrecognized.empty())
    {
        std::ostringstream oss;
        if (!missing.empty())
        {
            oss << "Missing required arguments: ";
            for (const auto& k : missing)
                oss << k << " ";
            oss << std::endl;
        }
        if (!unrecognized.empty())
        {
            oss << "Unsupported arguments: ";
            for (const auto& k : unrecognized)
                oss << k << " ";
            oss << std::endl;
        }
        printUsage(oss);
        throw SimulatorException(oss.str());
    }

    if (args_.count("num_threads"))
    {
        try
        {
            std::stoi(args_.at("num_threads"));
        }
        catch (...)
        {
            std::ostringstream oss;
            printUsage(oss, "Invalid number of threads: must be an integer");
            throw SimulatorException(oss.str());
        }
    }
}

std::vector<std::string> ArgumentsParser::getMissingKeys(const std::vector<std::string>& required_keys) const
{
    std::vector<std::string> missing;
    for (const auto& key : required_keys)
    {
        if (!args_.count(key))
            missing.push_back(key);
    }
    return missing;
}

std::vector<std::string> ArgumentsParser::getUnrecognizedKeys(const std::vector<std::string>& required_keys) const
{
    std::set<std::string> valid_keys(required_keys.begin(), required_keys.end());
    valid_keys.insert(optional_keys.begin(), optional_keys.end());

    std::vector<std::string> unrecognized;
    for (const auto& [key, _] : args_)
    {
        if (!valid_keys.count(key))
            unrecognized.push_back(key);
    }
    return unrecognized;
}

SimulatorConfig ArgumentsParser::getConfig() const
{
    SimulatorConfig config;
    config.mode = mode_;
    config.verbose = verbose_;
    if (args_.count("num_threads"))
        config.num_threads = std::stoi(args_.at("num_threads"));

    if (mode_ == RunMode::COMPARATIVE)
    {
        config.game_map_filename = args_.at("game_map");
        config.game_managers_folder = args_.at("game_managers_folder");
        config.algorithm1_so = args_.at("algorithm1");
        config.algorithm2_so = args_.at("algorithm2");
    }
    else
    {
        config.game_maps_folder = args_.at("game_maps_folder");
        config.game_manager_so = args_.at("game_manager");
        config.algorithms_folder = args_.at("algorithms_folder");
    }

    return config;
}

void ArgumentsParser::printUsage(std::ostream& os, const std::string& error_msg)
{
    if (!error_msg.empty())
        os << error_msg << std::endl;

    os << "\nUsage:\n"
       << "  ./simulator_<ids> -comparative game_map=<file> game_managers_folder=<folder> \\\n"
       << "    algorithm1=<file> algorithm2=<file> [num_threads=<n>] [-verbose]\n\n"
       << "  ./simulator_<ids> -competition game_maps_folder=<folder> game_manager=<file> \\\n"
       << "    algorithms_folder=<folder> [num_threads=<n>] [-verbose]\n\n";
}
