#include "test_base.h"

class CoverageTests : public SimulatorIntegrationBase {};

TEST_F(CoverageTests, ComparativeSingleVsMultiThreadSameOutput)
{
    const fs::path gms = game_managers_folder_;
    const fs::path map = maps_folder_ / "input_b.txt";
    const fs::path algo = defaultAlgorithmBase();
    std::regex compNameRe("^comparative_results_[0-9]{9}\\.txt$");

    removeIfMatches(gms, compNameRe);
    std::string sim = simulatorPath();
    std::string common = " -comparative game_map=" + map.string() +
        " game_managers_folder=" + gms.string() +
        " algorithm1=" + libraryExtension(algo) + " algorithm2=" + libraryExtension(algo);

    ASSERT_EQ(run(sim + common + " num_threads=1"), 0);
    auto file1 = findFirstMatchingFile(gms, compNameRe);
    ASSERT_FALSE(file1.empty());
    auto body1 = bodyFromComparative(readAllLines(file1));

    removeIfMatches(gms, compNameRe);
    ASSERT_EQ(run(sim + common + " num_threads=16"), 0);
    auto file2 = findFirstMatchingFile(gms, compNameRe);
    ASSERT_FALSE(file2.empty());
    auto body2 = bodyFromComparative(readAllLines(file2));

    // Compare all content beyond header exactly
    ASSERT_EQ(body1, body2);
}

TEST_F(CoverageTests, ComparativeDifferentAlgorithmNames)
{
    const fs::path gms = game_managers_folder_;
    const fs::path map = maps_folder_ / "input_a.txt";
    const fs::path baseAlgo = defaultAlgorithmBase();
    const std::string src = libraryExtension(baseAlgo);
    const std::string ext = fs::path(src).extension().string();

    const fs::path tmp = root_path_ / "build" / "tmp_algos_cmp";
    fs::create_directories(tmp);
    const fs::path algoA = tmp / fs::path(std::string("AlgoX") + ext);
    const fs::path algoB = tmp / fs::path(std::string("AlgoY") + ext);
    fs::copy_file(src, algoA, fs::copy_options::overwrite_existing);
    fs::copy_file(src, algoB, fs::copy_options::overwrite_existing);

    std::regex compNameRe("^comparative_results_[0-9]{9}\\.txt$");
    removeIfMatches(gms, compNameRe);

    std::string sim = simulatorPath();
    std::string cmd = sim + " -comparative game_map=" + map.string() +
        " game_managers_folder=" + gms.string() +
        " algorithm1=" + algoA.string() + " algorithm2=" + algoB.string();
    ASSERT_EQ(run(cmd), 0);
    auto outFile = findFirstMatchingFile(gms, compNameRe);
    ASSERT_FALSE(outFile.empty());
    auto lines = readAllLines(outFile);
    ASSERT_GE(lines.size(), 3u);
    ASSERT_NE(lines[1].find("algorithm1=AlgoX"), std::string::npos);
    ASSERT_NE(lines[2].find("algorithm2=AlgoY"), std::string::npos);
}

TEST_F(CoverageTests, CompetitionWhitespaceAndThreads)
{
    const fs::path maps = root_path_ / "integration_tests" / "cases" / "maps";
    const fs::path baseAlgo = defaultAlgorithmBase();
    const std::string src = libraryExtension(baseAlgo);
    const std::string ext = fs::path(src).extension().string();

    const fs::path tmp = root_path_ / "build" / "tmp_algos_ws";
    fs::create_directories(tmp);
    fs::copy_file(src, tmp / fs::path(std::string("Algo1") + ext), fs::copy_options::overwrite_existing);
    fs::copy_file(src, tmp / fs::path(std::string("Algo2") + ext), fs::copy_options::overwrite_existing);

    std::string sim = simulatorPath();
    std::string cmd = sim +
        " -competition game_maps_folder =" + maps.string() +
        " game_manager = " + libraryExtension(defaultGameManagerBase()) +
        " algorithms_folder= " + tmp.string() +
        " num_threads=8";
    ASSERT_EQ(run(cmd), 0);

    // Find output and compare that body contains exactly two lines (two algos) with scores
    std::regex compNameRe("^competition_[0-9]{9}\\.txt$");
    auto outFile = findFirstMatchingFile(tmp, compNameRe);
    ASSERT_FALSE(outFile.empty());
    auto lines = readAllLines(outFile);
    auto body = bodyFromCompetition(lines);
    // Two algorithms
    ASSERT_EQ(body.size(), 2u);
}
