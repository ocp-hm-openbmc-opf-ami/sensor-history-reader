#pragma once

#include "xyz/openbmc_project/SensorReader/History/Read/server.hpp"

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <list>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <string>
#include <thread>
#include <variant>
#include <vector>
#include <xyz/openbmc_project/Common/error.hpp>

using phosphor::logging::elog;
using phosphor::logging::entry;
using phosphor::logging::level;
using phosphor::logging::log;
using sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

namespace phosphor
{
namespace SensorReader
{

/* Need a custom deleter for freeing up sd_event */
struct EventDeleter
{
    void operator()(sd_event* event) const
    {
        event = sd_event_unref(event);
    }
};

using namespace std::literals::chrono_literals;
namespace fs = std::filesystem;
using Json = nlohmann::json;
using Value = std::variant<int, double>;
using Service = std::string;
using SensorPath = std::string;
using Interface = std::string;
using Interfaces = std::vector<Interface>;
using SensorValue = std::pair<uint64_t, double>;
using MapSensorValues = std::map<SensorPath, std::list<SensorValue>>;
using MapperResponseType = std::map<SensorPath, std::map<Service, Interfaces>>;

using EventPtr = std::unique_ptr<sd_event, EventDeleter>;
using ObjectPath = sdbusplus::message::object_path;

using Ifaces = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::SensorReader::History::server::Read>;

using HistoryIntf =
    sdbusplus::xyz::openbmc_project::SensorReader::History::server::Read;

/* default 5s D-Bus timeout. */
constexpr std::chrono::microseconds DBUS_TIMEOUT = 5s;

/** @class History
 *  @brief OpenBMC Sensor History Reader implementation.
 */
class History : public Ifaces
{
  public:
    History() = delete;
    History(const History&) = delete;
    History& operator=(const History&) = delete;
    History(History&&) = delete;
    History& operator=(History&&) = delete;
    virtual ~History();

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Path to attach at.
     */
    History(sdbusplus::bus::bus& bus, const char* objPath,
            const char* readerPath);

    /** @brief Reads the Sensor History values .
     *  @param[in] bus - Sensor name.
     *  @return - last 10 minutes sensor reading
     */
    std::map<uint64_t, double> read(std::string name) override;

    uint64_t interval(uint64_t value) override;

    uint64_t timeFrame(uint64_t value) override;

    using HistoryIntf::interval;
    using HistoryIntf::timeFrame;

  private:
    void writeSensorReaderfile(bool force);

    std::pair<uint64_t, uint64_t> readSensorReaderfile();

    /** @brief read sensor objects. */
    MapperResponseType getSensorObject(sdbusplus::bus::bus& bus);

    /** @brief read sensor values. */
    Value getSensorValue(sdbusplus::bus::bus& bus, const std::string& service,
                         const std::string& objPath,
                         const std::string& interface,
                         const std::string& property,
                         std::chrono::microseconds timeout = DBUS_TIMEOUT);

    /** @brief read sensor history. */
    void readHistory();
	std::vector<std::string> readconfiguredsensorsfile();

    bool threadStart;
	std::vector<std::string> sensors;

    std::thread historyReader;

    fs::path readerConfDir;

  protected:
    /** @brief Sensor History Readings. */
    MapSensorValues sensorHistory;
};
} // namespace SensorReader
} // namespace phosphor
