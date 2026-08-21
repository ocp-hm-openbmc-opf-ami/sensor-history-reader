/**
 * Unit tests for History::read()
 *
 * Tests verify that read() returns the correct sensor history data from
 * the in-memory sensorHistory map without any D-Bus interaction.
 */

#include "mock_sensor_reader.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <string>

using namespace testing;
using namespace phosphor::SensorReader::test;
namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class HistoryReadTest : public ::testing::Test
{
  protected:
    testing::NiceMock<sdbusplus::SdBusMock> sdbusMock;
    sdbusplus::bus::bus bus;
    fs::path testDir;
    std::unique_ptr<HistoryTestable> history;

    static constexpr auto kSeedKey = "__seed_marker__";

    HistoryReadTest() :
        bus(makeMockBus(sdbusMock)),
        testDir(fs::temp_directory_path() / "sr_read_test")
    {
        writeConfigFile(testDir);
        // Seed a marker so we can wait for the History ctor's asynchronous
        // loader to finish before injecting, then start from empty; otherwise
        // the loader can clobber test-injected data.
        seedHistoryFile(testDir, {{kSeedKey, {{0, 0.0}}}});
        history = std::make_unique<HistoryTestable>(
            bus, "/xyz/openbmc_project/SensorReader/Read", testDir.c_str());
        EXPECT_TRUE(waitForHistoryKey(*history, kSeedKey));
        std::lock_guard<std::mutex> lk(history->historyMutex);
        history->sensorHistory.clear();
    }

    void TearDown() override
    {
        history.reset();
        fs::remove_all(testDir);
    }

    /** Helper: inject a single (timestamp, value) entry for @p sensorName. */
    void injectReading(const std::string& sensorName, uint64_t ts, double val)
    {
        std::lock_guard<std::mutex> lk(history->historyMutex);
        history->sensorHistory[sensorName].emplace_back(ts, val);
    }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_F(HistoryReadTest, ReadUnknownSensorReturnsEmptyMap)
{
    auto result = history->read("nonexistent_sensor");
    EXPECT_TRUE(result.empty());
}

TEST_F(HistoryReadTest, ReadKnownSensorReturnsSingleEntry)
{
    injectReading("cpu_temp", 1000, 55.5);

    auto result = history->read("cpu_temp");

    ASSERT_EQ(result.size(), 1u);
    EXPECT_DOUBLE_EQ(result.at(1000), 55.5);
}

TEST_F(HistoryReadTest, ReadKnownSensorReturnsMultipleEntries)
{
    injectReading("cpu_temp", 1000, 55.0);
    injectReading("cpu_temp", 1060, 56.0);
    injectReading("cpu_temp", 1120, 57.0);

    auto result = history->read("cpu_temp");

    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result.at(1000), 55.0);
    EXPECT_DOUBLE_EQ(result.at(1060), 56.0);
    EXPECT_DOUBLE_EQ(result.at(1120), 57.0);
}

TEST_F(HistoryReadTest, ReadOnlyReturnsRequestedSensor)
{
    injectReading("cpu_temp", 2000, 45.0);
    injectReading("fan_speed", 2000, 1200.0);

    auto result = history->read("cpu_temp");

    ASSERT_EQ(result.size(), 1u);
    EXPECT_DOUBLE_EQ(result.at(2000), 45.0);
}

TEST_F(HistoryReadTest, ReadOnEmptyHistoryReturnsEmptyMap)
{
    // History starts empty; confirm read returns empty for any sensor
    EXPECT_TRUE(history->read("cpu_temp").empty());
    EXPECT_TRUE(history->read("fan_speed").empty());
}

TEST_F(HistoryReadTest, ReadPreservesTimestampValuePairs)
{
    const uint64_t ts1 = 1700000000ULL;
    const uint64_t ts2 = 1700000060ULL;
    const double   v1  = 72.3;
    const double   v2  = 73.1;

    injectReading("inlet_temp", ts1, v1);
    injectReading("inlet_temp", ts2, v2);

    auto result = history->read("inlet_temp");

    EXPECT_DOUBLE_EQ(result[ts1], v1);
    EXPECT_DOUBLE_EQ(result[ts2], v2);
}

TEST_F(HistoryReadTest, ReadDoesNotModifyHistory)
{
    injectReading("mem_temp", 5000, 42.0);

    history->read("mem_temp");
    history->read("mem_temp");

    std::lock_guard<std::mutex> lk(history->historyMutex);
    EXPECT_EQ(history->sensorHistory["mem_temp"].size(), 1u);
}
