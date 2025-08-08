#include "test_base.h"

class ComparativeIntegrationTest : public SimulatorIntegrationBase, public ::testing::WithParamInterface<const char*> {};

INSTANTIATE_TEST_SUITE_P(Maps, ComparativeIntegrationTest,
                         ::testing::Values("input_a.txt", "input_b.txt", "input_c.txt"));

TEST_P(ComparativeIntegrationTest, ComparativeRunProducesExpectedHeaderAndBody)
{
    const fs::path map = maps_folder_ / static_cast<std::string>(GetParam());
    const fs::path gms_folder = game_managers_folder_;
    const fs::path alg1 = defaultAlgorithmBase();
    const fs::path alg2 = defaultAlgorithmBase();

    ASSERT_TRUE(fs::exists(map));
    ASSERT_TRUE(fs::exists(gms_folder));
    ASSERT_TRUE(fs::exists(alg1.string() + ".dylib") || fs::exists(alg1.string() + ".so"));

    // Clean previous comparative files to make validation easier
    std::regex compNameRe("^comparative_results_\\d{9}\\.txt$");
    removeIfMatches(gms_folder, compNameRe);

    const std::string sim = SimulatorIntegrationBase::simulatorPath();
    ASSERT_TRUE(fs::exists(sim)) << "Simulator not found at " << sim;

    std::string cmd = sim +
        " -comparative game_map=" + map.string() +
        " game_managers_folder=" + gms_folder.string() +
        " algorithm1=" + libraryExtension(alg1) +
        " algorithm2=" + libraryExtension(alg2);

    int code = run(cmd);
    ASSERT_EQ(code, 0) << "Simulator failed: " << cmd;

    // Find newly created comparative_results_*.txt
    fs::path outFile;
    for (const auto& entry : fs::directory_iterator(gms_folder)) {
        if (!entry.is_regular_file()) continue;
        const auto name = entry.path().filename().string();
        if (std::regex_match(name, compNameRe)) {
            outFile = entry.path();
            break;
        }
    }
    ASSERT_TRUE(!outFile.empty()) << "No comparative_results_*.txt found";

    auto lines = readAllLines(outFile);
    ASSERT_GE(lines.size(), 8u) << "Unexpected output format";

    EXPECT_TRUE(lines[0].rfind("game_map=", 0) == 0);
    EXPECT_TRUE(lines[1].rfind("algorithm1=", 0) == 0);
    EXPECT_TRUE(lines[2].rfind("algorithm2=", 0) == 0);
    EXPECT_TRUE(lines[3].empty());

    ASSERT_FALSE(lines[6].empty());
    for (char c : lines[6]) {
        ASSERT_TRUE(std::isdigit(static_cast<unsigned char>(c)) || std::isspace(static_cast<unsigned char>(c)))
            << "Round line must be numeric";
    }

    const std::string valid = "#@ .12*%";
    for (size_t i = 7; i < lines.size(); ++i) {
        for (char ch : lines[i]) {
            ASSERT_NE(valid.find(ch), std::string::npos) << "Invalid map char '" << ch << "' at line " << i;
        }
    }
}

TEST_P(ComparativeIntegrationTest, SameInputDeterminismAcrossRuns)
{
    const fs::path map = maps_folder_ / static_cast<std::string>(GetParam());
    const fs::path gms_folder = game_managers_folder_;
    const fs::path alg = defaultAlgorithmBase();

    std::regex compNameRe("^comparative_results_[0-9]{9}\\.txt$");
    removeIfMatches(gms_folder, compNameRe);

    const std::string sim = SimulatorIntegrationBase::simulatorPath();
    std::string base = std::string(" -comparative game_map=") + map.string() +
        " game_managers_folder=" + gms_folder.string() + " algorithm1=" + libraryExtension(alg) +
        " algorithm2=" + libraryExtension(alg);

    ASSERT_EQ(run(sim + base), 0);
    fs::path first = findFirstMatchingFile(gms_folder, compNameRe);
    ASSERT_FALSE(first.empty());
    auto firstLines = bodyFromComparative(readAllLines(first));

    ASSERT_EQ(run(sim + base), 0);
    fs::path second = findFirstMatchingFile(gms_folder, compNameRe);
    ASSERT_FALSE(second.empty());
    auto secondLines = bodyFromComparative(readAllLines(second));

    ASSERT_EQ(firstLines, secondLines);
}
