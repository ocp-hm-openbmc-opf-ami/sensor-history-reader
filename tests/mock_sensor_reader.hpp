#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sdbusplus/test/sdbus_mock.hpp>

#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/utility.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sensor_reader.hpp>
#include <string>
#include <thread>

namespace phosphor::SensorReader::test
{

namespace fs = std::filesystem;

/** On-disk name of the serialized history file (matches production). */
inline constexpr auto kHistoryFileName = "sensorHistoryData.bin";

/**
 * @brief Creates a mock sdbusplus bus backed by SdBusMock.
 *
 * SdBusMock's constructor already sets default ON_CALL handlers for
 * sd_bus_add_object_vtable / add_object_manager / add_match so object
 * construction succeeds without a real D-Bus daemon.
 */
inline sdbusplus::bus::bus makeMockBus(sdbusplus::SdBusMock& mock)
{
    return sdbusplus::get_mocked_new(&mock);
}

/**
 * @brief Write a minimal sensor-reader JSON config to @p dir.
 *
 * Creates dir if it does not exist, then writes sensorreader.json with
 * the supplied interval (seconds) and timeFrame (minutes).
 */
inline void writeConfigFile(const fs::path& dir, uint64_t interval = 60,
                             uint64_t timeFrame = 10)
{
    fs::create_directories(dir);
    std::ofstream f(dir / "sensorreader.json");
    f << "{\n\"Interval\": " << interval << ",\n\"TimeFrame\": " << timeFrame
      << "\n}\n";
}

/**
 * @brief Expose protected sensorHistory / historyMutex for white-box tests.
 *
 * No additional methods – all logic under test lives in the production
 * History class.  Tests inject data via the protected members and then
 * exercise the public API.
 */
class HistoryTestable : public phosphor::SensorReader::History
{
  public:
    HistoryTestable(sdbusplus::bus::bus& bus, const char* objPath,
                    const char* readerPath)
        : phosphor::SensorReader::History(bus, objPath, readerPath)
    {}

    using phosphor::SensorReader::History::sensorHistory;
    using phosphor::SensorReader::History::historyMutex;
};

/**
 * @brief Seed the on-disk history file so the History ctor's asynchronous
 *        loader has a detectable, non-empty map to load.
 *
 * The production loader runs on a background thread and overwrites
 * sensorHistory once at startup; seeding lets tests wait for that load to
 * finish (see waitForHistoryKey) before injecting data, removing the race.
 * Uses the same text archive format as the production writer.
 */
inline void seedHistoryFile(const fs::path& dir, const MapSensorValues& data)
{
    fs::create_directories(dir);
    std::ofstream f((dir / kHistoryFileName).c_str());
    boost::archive::text_oarchive oa(f);
    oa << data;
}

/**
 * @brief Poll until the async loader has populated @p key into sensorHistory,
 *        or @p timeout elapses. Returns true if the key appeared.
 */
inline bool waitForHistoryKey(
    HistoryTestable& h, const std::string& key,
    std::chrono::milliseconds timeout = std::chrono::seconds(10))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        {
            std::lock_guard<std::mutex> lk(h.historyMutex);
            if (h.sensorHistory.count(key) > 0)
            {
                return true;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return false;
}

} // namespace phosphor::SensorReader::test
