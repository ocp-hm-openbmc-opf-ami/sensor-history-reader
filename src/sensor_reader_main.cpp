#include "sensor_reader.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdeventplus/event.hpp>

#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>

constexpr char DEFAULT_OBJPATH[] = "/xyz/openbmc_project/SensorReader";
constexpr char DEFAULT_BUSNAME[] = "xyz.openbmc_project.SensorReader";
static constexpr auto READER_DIR = "/etc/sensor-reader";
/*coverity [root_function : FALSE] */
int main(int /*argc*/, char** /*argv*/)
{
    try
    {
        auto bus = sdbusplus::bus::new_default();

        // Need sd_event to watch for OCC device errors
        sd_event* event = nullptr;
        auto r = sd_event_default(&event);
        if (r < 0)
        {
            log<level::ERR>("Error creating a default sd_event handler");
            return r;
        }

        phosphor::SensorReader::EventPtr eventPtr{event};
        event = nullptr;

        // Attach the bus to sd_event to service user requests
        bus.attach_event(eventPtr.get(), SD_EVENT_PRIORITY_NORMAL);

        // Add sdbusplus Object Manager for the 'root' path of the Sensor Reader
        // manager.
        sdbusplus::server::manager::manager objManager(bus, DEFAULT_OBJPATH);
        bus.request_name(DEFAULT_BUSNAME);

        auto Sensor_History_Obj =
            std::string(DEFAULT_OBJPATH) + std::string("/History");
        auto service = std::make_shared<phosphor::SensorReader::History>(
            bus, Sensor_History_Obj.c_str(), READER_DIR);

        sd_event_loop(eventPtr.get());
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("D-Bus error",
                        phosphor::logging::entry("NAME=%s", e.name()),
                        phosphor::logging::entry("MSG=%s", e.description()));
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Unhandled std::exception",
                        phosphor::logging::entry("MSG=%s", e.what()));
    }
    return EXIT_FAILURE;
}
