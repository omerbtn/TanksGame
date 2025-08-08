#include "test_base.h"

class IntegrationCLI : public SimulatorIntegrationBase {};

TEST_F(IntegrationCLI, MissingArgsShowsUsage)
{
    ASSERT_TRUE(fs::exists(simulator_path_));
    fs::path out = root_path_ / "build" / "cli_missing.txt";
    runCapture(simulator_path_.string() + " -comparative", out);
    std::string text = readAll(out);
    ASSERT_NE(text.find("Missing required arguments"), std::string::npos);
}

TEST_F(IntegrationCLI, UnsupportedArgsReported)
{
    fs::path out = root_path_ / "build" / "cli_unsupported.txt";
    std::string cmd = simulator_path_.string() + " -comparative game_map=x game_managers_folder=y algorithm1=z algorithm2=w foo=bar";
    runCapture(cmd, out);
    std::string text = readAll(out);
    ASSERT_NE(text.find("Unsupported arguments"), std::string::npos);
}

TEST_F(IntegrationCLI, InvalidThreadsReported)
{
    fs::path out = root_path_ / "build" / "cli_threads.txt";
    std::string cmd = simulator_path_.string() + " -comparative game_map=x game_managers_folder=y algorithm1=z algorithm2=w num_threads=abc";
    runCapture(cmd, out);
    std::string text = readAll(out);
    ASSERT_NE(text.find("Invalid number of threads"), std::string::npos);
}

TEST_F(IntegrationCLI, ComparativeInvalidMap)
{
    fs::path out = root_path_ / "build" / "cli_badmap.txt";
    std::string cmd = simulator_path_.string() + " -comparative game_map=/no/such/file.txt game_managers_folder=" + game_managers_folder_.string() +
        " algorithm1=" + libraryExtension(defaultAlgorithmBase()) + " algorithm2=" + libraryExtension(defaultAlgorithmBase());
    runCapture(cmd, out);
    std::string text = readAll(out);
    ASSERT_NE(text.find("Invalid game map"), std::string::npos);
}

TEST_F(IntegrationCLI, SingleModeRuns)
{
    fs::path out = root_path_ / "build" / "cli_single.txt";
    std::string cmd = simulator_path_.string() + " -single game_manager=" + libraryExtension(defaultGameManagerBase()) + " game_map=" + maps_folder_.string() + "/input_a.txt" +
        " algorithm1=" + libraryExtension(defaultAlgorithmBase()) + " algorithm2=" + libraryExtension(defaultAlgorithmBase());
    int code = run(cmd);
    ASSERT_EQ(code, 0);
}

TEST_F(IntegrationCLI, CompetitionRequiresTwoAlgorithms)
{
    fs::path out = root_path_ / "build" / "cli_comp_need_two.txt";
    std::string cmd = simulator_path_.string() + " -competition game_maps_folder=" + maps_folder_.string() +
        " game_manager=" + libraryExtension(defaultGameManagerBase()) + " algorithms_folder=" + algorithms_folder_.string();
    runCapture(cmd, out);
    std::string text = readAll(out);
    ASSERT_NE(text.find("At least two algorithms"), std::string::npos);
}

TEST_F(IntegrationCLI, CompetitionNoMaps)
{
    fs::path tempMaps = root_path_ / "build" / "empty_maps";
    fs::create_directories(tempMaps);
    // Prepare algorithms folder with two copies so we bypass the "need two algos" error
    fs::path tmpAlgos = root_path_ / "build" / "tmp_algos_two";
    fs::create_directories(tmpAlgos);
    const std::string srcAlgo = libraryExtension(defaultAlgorithmBase());
    const std::string ext = fs::path(srcAlgo).extension().string();
    fs::copy_file(srcAlgo, tmpAlgos / fs::path(std::string("AlgoA") + ext), fs::copy_options::overwrite_existing);
    fs::copy_file(srcAlgo, tmpAlgos / fs::path(std::string("AlgoB") + ext), fs::copy_options::overwrite_existing);
    fs::path out = root_path_ / "build" / "cli_no_maps.txt";
    std::string cmd = simulator_path_.string() + " -competition game_maps_folder=" + tempMaps.string() +
        " game_manager=" + libraryExtension(defaultGameManagerBase()) + " algorithms_folder=" + tmpAlgos.string();
    runCapture(cmd, out);
    std::string text = readAll(out);
    ASSERT_NE(text.find("No valid game maps found"), std::string::npos);
}

TEST_F(IntegrationCLI, ComparativeMultiThreadCreatesOutput)
{
    const fs::path map = maps_folder_ / "input_c.txt";
    const fs::path gms = game_managers_folder_;
    ASSERT_TRUE(fs::exists(map));
    std::regex compNameRe("^comparative_results_[0-9]{9}\\.txt$");
    // Cleanup old files
    for (const auto& e : fs::directory_iterator(gms)) {
        if (e.is_regular_file() && std::regex_match(e.path().filename().string(), compNameRe)) {
            std::error_code ec; 
            fs::remove(e.path(), ec);
        }
    }
    std::string cmd = simulator_path_.string() + " -comparative num_threads=4 game_map=" + map.string() +
        " game_managers_folder=" + gms.string() + " algorithm1=" + libraryExtension(defaultAlgorithmBase()) +
        " algorithm2=" + libraryExtension(defaultAlgorithmBase());
    int code = run(cmd);
    ASSERT_EQ(code, 0);
    bool found = false;
    for (const auto& e : fs::directory_iterator(gms)) {
        if (e.is_regular_file() && std::regex_match(e.path().filename().string(), compNameRe)) {
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);
}

TEST_F(IntegrationCLI, CompetitionProducesScores)
{
    const fs::path maps = root_path_ / "integration_tests" / "cases" / "maps";
    ASSERT_TRUE(fs::exists(maps));
    const fs::path tmpAlgos = root_path_ / "build" / "tmp_algos_comp";
    fs::create_directories(tmpAlgos);
    const std::string srcAlgo = libraryExtension(defaultAlgorithmBase());
    const std::string ext = fs::path(srcAlgo).extension().string();
    fs::copy_file(srcAlgo, tmpAlgos / fs::path(std::string("Algo1") + ext), fs::copy_options::overwrite_existing);
    fs::copy_file(srcAlgo, tmpAlgos / fs::path(std::string("Algo2") + ext), fs::copy_options::overwrite_existing);

    // Cleanup previous competition files
    std::regex nameRe("^competition_[0-9]{9}\\.txt$");
    for (const auto& e : fs::directory_iterator(tmpAlgos)) {
        if (e.is_regular_file() && std::regex_match(e.path().filename().string(), nameRe)) {
            std::error_code ec; 
            fs::remove(e.path(), ec);
        }
    }

    std::string cmd = simulator_path_.string() + " -competition game_maps_folder=" + maps.string() +
        " game_manager=" + libraryExtension(defaultGameManagerBase()) + " algorithms_folder=" + tmpAlgos.string();
    int code = run(cmd);
    ASSERT_EQ(code, 0);

    fs::path outFile;
    for (const auto& e : fs::directory_iterator(tmpAlgos)) {
        if (e.is_regular_file() && std::regex_match(e.path().filename().string(), nameRe)) { 
            outFile = e.path(); 
            break; 
        }
    }
    ASSERT_FALSE(outFile.empty());
    auto content = readAll(outFile);
    // Expect header lines
    ASSERT_NE(content.find("game_maps_folder="), std::string::npos);
    ASSERT_NE(content.find("game_manager="), std::string::npos);
    // Expect two score lines
    size_t lines = 0; 
    for (char c : content) {
        if (c == '\n') {
            ++lines;
        }
    }
    ASSERT_GE(lines, 5u);
}
