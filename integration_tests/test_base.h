#pragma once

#include <gtest/gtest.h>

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

#include "global_config.h"

namespace fs = std::filesystem;

class SimulatorIntegrationBase : public ::testing::Test {
protected:
    static constexpr const char* kSubmitterIds = "322573304_322647603";

    SimulatorIntegrationBase() {
        root_path_ = projectRoot();
        simulator_path_ = simulatorPath();
        maps_folder_ = resourcesMapsFolder();
        game_managers_folder_ = resourcesGameManagersFolder();
        algorithms_folder_ = resourcesAlgorithmsFolder();
    }

    static std::string projectRoot() {
        fs::path cur = fs::current_path();
        for (int i = 0; i < 8; ++i) {
            if (fs::exists(cur / "config.ini")) {
                return cur.string();
            }
            cur = cur.parent_path();
        }
        return fs::current_path().string();
    }

    static std::string simulatorPath() {
        const fs::path root = projectRoot();
        fs::path in_build = root / "build" / "bin" / (std::string("simulator_") + kSubmitterIds);
        if (fs::exists(in_build)) {
            return in_build.string();
        }
        fs::path in_sim = root / "Simulator" / (std::string("simulator_") + kSubmitterIds);
        return in_sim.string();
    }

    static std::string libraryExtension(const fs::path& base) {
        return base.string() + static_cast<std::string>(UserCommon_322573304_322647603::config::get<std::string_view>("shared_library_extension"));
    }

    // Resources folders and default basenames
    static fs::path resourcesMapsFolder() {
        return fs::path(projectRoot()) / "resources" / "game_maps";
    }

    static fs::path resourcesGameManagersFolder() {
        return fs::path(projectRoot()) / "resources" / "game_managers";
    }

    static fs::path resourcesAlgorithmsFolder() {
        return fs::path(projectRoot()) / "resources" / "algorithms";
    }

    static fs::path defaultAlgorithmBase() {
        return resourcesAlgorithmsFolder() / (std::string("Algorithm_") + kSubmitterIds);
    }

    static fs::path defaultGameManagerBase() {
        return resourcesGameManagersFolder() / (std::string("GameManager_") + kSubmitterIds);
    }

    static std::vector<std::string> readAllLines(const fs::path& file) {
        std::ifstream in(file);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(in, line)) {
            lines.push_back(line);
        }
        return lines;
    }

    static std::string readAll(const fs::path& file) {
        std::ifstream in(file);
        return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    }

    static int run(const std::string& cmd) {
        return std::system(cmd.c_str());
    }

    static void runCapture(const std::string& cmd, const fs::path& out) {
        std::string full = cmd + " >" + out.string() + " 2>&1";
        (void)std::system(full.c_str());
    }

    static void removeIfMatches(const fs::path& folder, const std::regex& nameRe) {
        if (!fs::exists(folder)) return;
        for (const auto& e : fs::directory_iterator(folder)) {
            if (!e.is_regular_file()) {
                continue;
            }
            const auto name = e.path().filename().string();
            if (std::regex_match(name, nameRe)) {
                std::error_code ec; 
                fs::remove(e.path(), ec);
            }
        }
    }

    static fs::path findFirstMatchingFile(const fs::path& folder, const std::regex& nameRe) {
        for (const auto& e : fs::directory_iterator(folder)) {
            if (!e.is_regular_file()) {
                continue;
            }
            const auto name = e.path().filename().string();
            if (std::regex_match(name, nameRe)) {
                return e.path();
            }
        }
        return {};
    }

    static std::vector<std::string> bodyFromComparative(const std::vector<std::string>& lines) {
        // For comparative: header is 4 lines, body starts at 4
        if (lines.size() <= 4) return {};
        return std::vector<std::string>(lines.begin() + 4, lines.end());
    }

    static std::vector<std::string> bodyFromCompetition(const std::vector<std::string>& lines) {
        // For competition: header is 3 lines (2 lines + blank), body starts at 3
        if (lines.size() <= 3) return {};
        return std::vector<std::string>(lines.begin() + 3, lines.end());
    }

    fs::path root_path_;
    fs::path simulator_path_;
    fs::path maps_folder_;
    fs::path game_managers_folder_;
    fs::path algorithms_folder_;
};
