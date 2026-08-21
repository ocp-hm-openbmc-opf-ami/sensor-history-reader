/**
 * Unit tests for History data persistence (wrtieHistoryDataToFile /
 * readHistoryDataFromFile / saveHistoryOnShutdown).
 *
 * Tests verify:
 *  - Sensor history injected into sensorHistory is written to the binary
 *    file and fully recovered on the next construction.
 *  - Multiple sensors are persisted and retrieved correctly.
 *  - A missing history file results in an empty sensorHistory on startup.
 *  - saveHistoryOnShutdown() persists in-memory history on demand.
 *  - An empty sensorHistory produces no binary file (nothing to save).
 */

#include "mock_sensor_reader.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <string>

using namespace testing;
using namespace phosphor::SensorReader::test;
namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class HistoryPersistenceTest : public ::testing::Test
{
  protected:
    testing::NiceMock<sdbusplus::SdBusMock> sdbusMock;
    sdbusplus::bus::bus bus;
    fs::path testDir;

    static constexpr auto kObjPath = "/xyz/openbmc_project/SensorReader/Persist";
    static constexpr auto kSeedKey = "__seed_marker__";

    HistoryPersistenceTest() :
        bus(makeMockBus(sdbusMock)),
        testDir(fs::temp_directory_path() / "sr_persist_test")
    {
        writeConfigFile(testDir);
    }

    void TearDown() override
    {
        fs::remove_all(testDir);
    }

    std::unique_ptr<HistoryTestable> makeHistory(const char* objPath = kObjPath)
    {
        return std::make_unique<HistoryTestable>(bus, objPath, testDir.c_str());
    }

    /**
     * Construct a History and block until its asynchronous loader has run,
     * then clear the loaded marker so the caller starts from a known-empty
     * sensorHistory that the loader can no longer clobber.
     */
    std::unique_ptr<HistoryTestable> makeEmptyReadyHistory(
        const char* objPath = kObjPath)
    {
        seedHistoryFile(testDir, {{kSeedKey, {{0, 0.0}}}});
        auto h = makeHistory(objPath);
        EXPECT_TRUE(waitForHistoryKey(*h, kSeedKey));
        {
            std::lock_guard<std::mutex> lk(h->historyMutex);
            h->sensorHistory.clear();
        }
        return h;
    }

    /** Inject @p count readings into @p sensorName, starting at ts @p base. */
    void injectReadings(HistoryTestable& h, const std::string& sensorName,
                        uint64_t base, double startValue, int count)
    {
        std::lock_guard<std::mutex> lk(h.historyMutex);
        for (int i = 0; i < count; ++i)
            h.sensorHistory[sensorName].emplace_back(base + i * 60ULL,
                                                     startValue + i);
    }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_F(HistoryPersistenceTest, NoHistoryFileResultsInEmptyHistory)
{
    // Fresh directory – no sensorHistoryData.bin exists
    auto h = makeHistory();

    std::lock_guard<std::mutex> lk(h->historyMutex);
    EXPECT_TRUE(h->sensorHistory.empty());
}

TEST_F(HistoryPersistenceTest, SaveAndRestoreSingleSensor)
{
    {
        auto h = makeEmptyReadyHistory();
        injectReadings(*h, "cpu_temp", 1000, 50.0, 3);
        h->saveHistoryOnShutdown();
    }

    // Re-open the same directory: history should be loaded from file
    auto h2 = makeHistory("/xyz/openbmc_project/SensorReader/Persist2");
    ASSERT_TRUE(waitForHistoryKey(*h2, "cpu_temp"));

    std::lock_guard<std::mutex> lk(h2->historyMutex);
    ASSERT_TRUE(h2->sensorHistory.count("cpu_temp") > 0);
    EXPECT_EQ(h2->sensorHistory.at("cpu_temp").size(), 3u);

    auto it = h2->sensorHistory.at("cpu_temp").begin();
    EXPECT_EQ(it->first, 1000u);
    EXPECT_DOUBLE_EQ(it->second, 50.0);
}

TEST_F(HistoryPersistenceTest, SaveAndRestoreMultipleSensors)
{
    {
        auto h = makeEmptyReadyHistory();
        injectReadings(*h, "cpu_temp",  2000, 45.0, 2);
        injectReadings(*h, "inlet_temp", 2000, 25.0, 2);
        injectReadings(*h, "fan_speed",  2000, 1200.0, 2);
        h->saveHistoryOnShutdown();
    }

    auto h2 = makeHistory("/xyz/openbmc_project/SensorReader/Persist3");
    ASSERT_TRUE(waitForHistoryKey(*h2, "cpu_temp"));

    std::lock_guard<std::mutex> lk(h2->historyMutex);
    EXPECT_EQ(h2->sensorHistory.count("cpu_temp"),   1u);
    EXPECT_EQ(h2->sensorHistory.count("inlet_temp"), 1u);
    EXPECT_EQ(h2->sensorHistory.count("fan_speed"),  1u);
    EXPECT_EQ(h2->sensorHistory.at("cpu_temp").size(),   2u);
    EXPECT_EQ(h2->sensorHistory.at("inlet_temp").size(), 2u);
    EXPECT_EQ(h2->sensorHistory.at("fan_speed").size(),  2u);
}

TEST_F(HistoryPersistenceTest, SaveHistoryOnShutdownPersistsCorrectValues)
{
    {
        auto h = makeEmptyReadyHistory();
        injectReadings(*h, "mem_temp", 5000, 40.0, 1);
        h->saveHistoryOnShutdown();
    }

    auto h2 = makeHistory("/xyz/openbmc_project/SensorReader/Persist4");
    ASSERT_TRUE(waitForHistoryKey(*h2, "mem_temp"));

    auto result = h2->read("mem_temp");
    ASSERT_EQ(result.size(), 1u);
    EXPECT_DOUBLE_EQ(result.at(5000), 40.0);
}

TEST_F(HistoryPersistenceTest, SaveHistoryOnShutdownDoesNotWriteWhenEmpty)
{
    {
        auto h = makeHistory();
        // sensorHistory is empty – saveHistoryOnShutdown must be a no-op
        h->saveHistoryOnShutdown();
    }

    // No binary file should exist
    EXPECT_FALSE(fs::exists(testDir / "sensorHistoryData.bin"));
}

TEST_F(HistoryPersistenceTest, ReadApiReturnsRestoredValues)
{
    {
        auto h = makeEmptyReadyHistory();
        injectReadings(*h, "outlet_temp", 3000, 30.0, 4);
        h->saveHistoryOnShutdown();
    }

    auto h2 = makeHistory("/xyz/openbmc_project/SensorReader/Persist5");
    ASSERT_TRUE(waitForHistoryKey(*h2, "outlet_temp"));
    auto result = h2->read("outlet_temp");

    ASSERT_EQ(result.size(), 4u);
    EXPECT_DOUBLE_EQ(result.at(3000), 30.0);
    EXPECT_DOUBLE_EQ(result.at(3060), 31.0);
    EXPECT_DOUBLE_EQ(result.at(3120), 32.0);
    EXPECT_DOUBLE_EQ(result.at(3180), 33.0);
}

TEST_F(HistoryPersistenceTest, OverwriteExistingHistoryFile)
{
    // Write once
    {
        auto h = makeEmptyReadyHistory();
        injectReadings(*h, "cpu_temp", 1000, 60.0, 2);
        h->saveHistoryOnShutdown();
    }

    // Write again with different data (simulates next boot cycle)
    {
        auto h = makeHistory("/xyz/openbmc_project/SensorReader/Persist6");
        // Wait for the loader to restore the previous file, then overwrite.
        ASSERT_TRUE(waitForHistoryKey(*h, "cpu_temp"));
        {
            std::lock_guard<std::mutex> lk(h->historyMutex);
            h->sensorHistory.clear();
        }
        injectReadings(*h, "cpu_temp", 9000, 70.0, 1);
        h->saveHistoryOnShutdown();
    }

    auto h2 = makeHistory("/xyz/openbmc_project/SensorReader/Persist7");
    ASSERT_TRUE(waitForHistoryKey(*h2, "cpu_temp"));
    auto result = h2->read("cpu_temp");

    // Only the second write should be visible
    ASSERT_EQ(result.size(), 1u);
    EXPECT_DOUBLE_EQ(result.at(9000), 70.0);
}
