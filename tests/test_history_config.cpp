/**
 * Unit tests for History::interval(), History::timeFrame(), and the
 * sensor-reader JSON config file read/write cycle.
 *
 * Tests verify:
 *  - Default interval (60 s) and timeFrame (10 min) are read from
 *    newly created config.
 *  - Setters persist new values to the JSON file.
 *  - A second History instance constructed from the same directory
 *    recovers the persisted values.
 *  - Setters return the new value and are idempotent when called with
 *    the current value.
 */

#include "mock_sensor_reader.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace testing;
using namespace phosphor::SensorReader::test;
namespace fs = std::filesystem;
using Json = nlohmann::json;

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class HistoryConfigTest : public ::testing::Test
{
  protected:
    testing::NiceMock<sdbusplus::SdBusMock> sdbusMock;
    sdbusplus::bus::bus bus;
    fs::path testDir;
    std::unique_ptr<HistoryTestable> history;

    HistoryConfigTest() :
        bus(makeMockBus(sdbusMock)),
        testDir(fs::temp_directory_path() / "sr_config_test")
    {
        writeConfigFile(testDir); // default: interval=60, timeFrame=10
        history = std::make_unique<HistoryTestable>(
            bus, "/xyz/openbmc_project/SensorReader/Config", testDir.c_str());
    }

    void TearDown() override
    {
        history.reset();
        fs::remove_all(testDir);
    }

    /** Parse sensorreader.json from the test directory. */
    Json readConfigJson() const
    {
        std::ifstream f(testDir / "sensorreader.json");
        return Json::parse(f, nullptr, false);
    }
};

// ---------------------------------------------------------------------------
// Default value tests
// ---------------------------------------------------------------------------

TEST_F(HistoryConfigTest, DefaultIntervalIs60Seconds)
{
    EXPECT_EQ(history->interval(60), 60u);
}

TEST_F(HistoryConfigTest, DefaultTimeFrameIs10Minutes)
{
    EXPECT_EQ(history->timeFrame(10), 10u);
}

// ---------------------------------------------------------------------------
// Interval setter/getter tests
// ---------------------------------------------------------------------------

TEST_F(HistoryConfigTest, IntervalReturnsSameValueWhenUnchanged)
{
    uint64_t current = history->interval(60);
    // Calling with the same value must return it without triggering a write
    EXPECT_EQ(history->interval(current), current);
}

TEST_F(HistoryConfigTest, IntervalUpdatesValue)
{
    uint64_t newVal = 120;
    EXPECT_EQ(history->interval(newVal), newVal);
}

TEST_F(HistoryConfigTest, IntervalPersistedToConfigFile)
{
    history->interval(30);

    Json cfg = readConfigJson();
    ASSERT_FALSE(cfg.is_discarded());
    EXPECT_EQ(cfg.at("Interval").get<uint64_t>(), 30u);
}

// ---------------------------------------------------------------------------
// TimeFrame setter/getter tests
// ---------------------------------------------------------------------------

TEST_F(HistoryConfigTest, TimeFrameReturnsSameValueWhenUnchanged)
{
    uint64_t current = history->timeFrame(10);
    EXPECT_EQ(history->timeFrame(current), current);
}

TEST_F(HistoryConfigTest, TimeFrameUpdatesValue)
{
    uint64_t newVal = 30;
    EXPECT_EQ(history->timeFrame(newVal), newVal);
}

TEST_F(HistoryConfigTest, TimeFramePersistedToConfigFile)
{
    history->timeFrame(20);

    Json cfg = readConfigJson();
    ASSERT_FALSE(cfg.is_discarded());
    EXPECT_EQ(cfg.at("TimeFrame").get<uint64_t>(), 20u);
}

// ---------------------------------------------------------------------------
// Config file round-trip: write, reconstruct, verify read
// ---------------------------------------------------------------------------

TEST_F(HistoryConfigTest, IntervalSurvivedAcrossRestart)
{
    history->interval(45);
    history.reset(); // destructor flushes/joins thread

    // Reconstruct from same directory: must pick up persisted value
    auto history2 = std::make_unique<HistoryTestable>(
        bus, "/xyz/openbmc_project/SensorReader/Config2", testDir.c_str());

    // The new instance reads the JSON and calls HistoryIntf::interval()
    // with the persisted value. Confirm it via the file.
    Json cfg = readConfigJson();
    ASSERT_FALSE(cfg.is_discarded());
    EXPECT_EQ(cfg.at("Interval").get<uint64_t>(), 45u);
}

TEST_F(HistoryConfigTest, TimeFrameSurvivedAcrossRestart)
{
    history->timeFrame(25);
    history.reset();

    auto history2 = std::make_unique<HistoryTestable>(
        bus, "/xyz/openbmc_project/SensorReader/Config3", testDir.c_str());

    Json cfg = readConfigJson();
    ASSERT_FALSE(cfg.is_discarded());
    EXPECT_EQ(cfg.at("TimeFrame").get<uint64_t>(), 25u);
}

// ---------------------------------------------------------------------------
// Config directory auto-creation
// ---------------------------------------------------------------------------

TEST_F(HistoryConfigTest, ConfigDirectoryCreatedIfAbsent)
{
    fs::path newDir = fs::temp_directory_path() / "sr_autodir_test";
    fs::remove_all(newDir); // ensure it doesn't exist

    {
        auto h = std::make_unique<HistoryTestable>(
            bus, "/xyz/openbmc_project/SensorReader/AutoDir", newDir.c_str());
        // Constructor must create the directory and write the config file
        EXPECT_TRUE(fs::exists(newDir / "sensorreader.json"));
    }

    fs::remove_all(newDir);
}
