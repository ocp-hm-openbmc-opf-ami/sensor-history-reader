#include "sensor_reader.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/bus.hpp>

namespace phosphor
{
namespace SensorReader
{
static constexpr auto READER_FILE = "sensorreader.json";
static constexpr auto PROP_INTF = "org.freedesktop.DBus.Properties";
static constexpr auto METHOD_GET = "Get";
static constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
static constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
static constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";
static constexpr const char* SensorInterface =
    "xyz.openbmc_project.Sensor.Value";
static const std::string property = "Value";
static const auto seconds_minute = 60; // seconds for a minute

History::History(sdbusplus::bus::bus& bus, const char* objPath,
                 const char* readerPath) :
    Ifaces(bus, objPath)
{
    fs::path confDir(readerPath);
    readerConfDir = confDir;
    writeSensorReaderfile(false);
    std::pair<uint64_t, uint64_t> value = readSensorReaderfile();
    HistoryIntf::interval(value.first);
    HistoryIntf::timeFrame(value.second);
    threadStart = true;
    historyReader = std::thread(&History::readHistory, this);
    emit_object_added();
}

History::~History()
{
    threadStart = false;

    if (historyReader.joinable())
    {
        historyReader.join();
    }
}

void History::writeSensorReaderfile(bool force)
{
    uint64_t interval = 60;// Default interval
    uint64_t timeFrame = 10;//Default Time Frame
    fs::path filePath = readerConfDir;
    filePath /= READER_FILE;

    if (!fs::exists(readerConfDir))
    {
        if (!fs::create_directories(readerConfDir))
        {
            log<level::ERR>("Unable to create the sensor reader directory",
                            entry("DIR=%s", readerConfDir.c_str()));
        }
    }

    if (force)
    {
        interval = HistoryIntf::interval();
        timeFrame = HistoryIntf::timeFrame();
    }

    if (!fs::is_regular_file(filePath.string()) || force)
    {
        std::fstream jsonFileStream;
        std::string readerjason =
            std::string("{\n\"Interval\": ") + std::to_string(interval) +
            std::string(",\n\"TimeFrame\": ") + std::to_string(timeFrame) +
            std::string("\n}");

        jsonFileStream.open(filePath.string(), std::ios::out);
        if (!jsonFileStream)
        {
            log<level::ERR>("Unable to open JSON file");
            return;
        }

        jsonFileStream << readerjason;
        jsonFileStream.close();
    }
}

std::pair<uint64_t, uint64_t> History::readSensorReaderfile()
{
    uint64_t interval = 60;// Default interval
    uint64_t timeFrame = 10;//Default Time Frame
    Json jsonData = nullptr;
    fs::path filePath = readerConfDir;
    filePath /= READER_FILE;

    std::ifstream jsonFile(filePath.string());
    if (!jsonFile.good())
    {
        log<level::ERR>("Sensor Reader JSON file not found");
    }

    try
    {
        jsonData = Json::parse(jsonFile, nullptr, false);
        interval = jsonData.at("Interval").get<uint64_t>();
        timeFrame = jsonData.at("TimeFrame").get<uint64_t>();
    }
    catch (Json::exception& e)
    {
        log<level::ERR>("Parse/Data type error", entry("MSG: %s", e.what()));
    }

    jsonFile.close();
    auto value = std::make_pair(interval, timeFrame);

    return value;
}

std::map<uint64_t, double> History::read(std::string name)
{
    std::map<uint64_t, double> historyValue;

    auto it = this->sensorHistory.find(name);
    if (it != this->sensorHistory.end())
    {
        for (SensorValue const& data : it->second)
        {
            auto value = std::make_pair(data.first, data.second);
            historyValue.insert(value);
        }
    }

    return historyValue;
}

uint64_t History::interval(uint64_t value)
{
    if (value == HistoryIntf::interval())
    {
        return value;
    }
    HistoryIntf::interval(value);
    writeSensorReaderfile(true);

    return value;
}

uint64_t History::timeFrame(uint64_t value)
{
    if (value == HistoryIntf::timeFrame())
    {
        return value;
    }
    HistoryIntf::timeFrame(value);
    writeSensorReaderfile(true);

    return value;
}

MapperResponseType History::getSensorObject(sdbusplus::bus::bus& bus)
{
    auto depth = 0;
    MapperResponseType mapperResponse;

    try
    {
        auto mapperCall = bus.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                              MAPPER_INTERFACE, "GetSubTree");
        mapperCall.append("/");
        mapperCall.append(depth);
        mapperCall.append(std::vector<Interface>({SensorInterface}));

        auto mapperResponseMsg = bus.call(mapperCall);
        if (mapperResponseMsg.is_method_error())
        {
            log<level::ERR>("Mapper GetSubTree failed",
                            entry("INTERFACE=%s", SensorInterface));
        }

        mapperResponseMsg.read(mapperResponse);
        if (mapperResponse.empty())
        {
            log<level::ERR>("Invalid mapper response",
                            entry("INTERFACE=%s", SensorInterface));
        }
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("error in getSensorObject", entry("name=%s", e.name()),
                        entry("what=%s", e.what()));
    }

    return mapperResponse;
}

Value History::getSensorValue(sdbusplus::bus::bus& bus,
                              const std::string& service,
                              const std::string& objPath,
                              const std::string& interface,
                              const std::string& property,
                              std::chrono::microseconds timeout)
{
    Value value = {0.0};

    try
    {
        auto method = bus.new_method_call(service.c_str(), objPath.c_str(),
                                          PROP_INTF, METHOD_GET);
        method.append(interface, property);
        auto reply = bus.call(method, timeout.count());

        if (reply.is_method_error())
        {
            log<level::ERR>("Failed to getSensorValue",
                            entry("PROPERTY=%s", property.c_str()),
                            entry("PATH=%s", objPath.c_str()),
                            entry("INTERFACE=%s", interface.c_str()));
        }
        reply.read(value);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("error in getSensorValue", entry("name=%s", e.name()),
                        entry("what=%s", e.what()),
                        entry("PROPERTY=%s", property.c_str()),
                        entry("PATH=%s", objPath.c_str()),
                        entry("INTERFACE=%s", interface.c_str()));
    }

    return value;
}

void History::readHistory()
{
    while (threadStart)
    {
        boost::asio::io_context io;
        auto conn = std::make_shared<sdbusplus::asio::connection>(io);
        auto sensorObjects = getSensorObject(*conn);
        auto now = std::chrono::system_clock::now();
        auto timeStamp = std::chrono::duration_cast<std::chrono::seconds>(
                             now.time_since_epoch())
                             .count();

        for (auto it = sensorObjects.begin(); it != sensorObjects.end(); it++)
        {
            auto data = getSensorValue(*conn, it->second.begin()->first,
                                       it->first, SensorInterface, property);
            double value = std::get<double>(data);

            if (std::isnan(value))
                value = 0.0;

            std::size_t pos = (it->first).rfind('/');
            auto sensorName = (it->first).substr(pos + 1);

            if (this->sensorHistory[sensorName].size() >=
                ((HistoryIntf::timeFrame() * seconds_minute) /
                 HistoryIntf::interval()))
            {
                for(int i = this->sensorHistory[sensorName].size() -
                ((HistoryIntf::timeFrame() * seconds_minute) /
                 HistoryIntf::interval()); i >= 0; i-- )
                this->sensorHistory[sensorName].pop_front();
            }
            
	    auto sensorValue = std::make_pair(timeStamp, value);
            this->sensorHistory[sensorName].push_back(sensorValue);
        }

        if (!threadStart)
            break;
        std::this_thread::sleep_for(
            std::chrono::seconds(HistoryIntf::interval()));
    }
}

} // namespace SensorReader
} // namespace phosphor
