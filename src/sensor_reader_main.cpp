#include "sensor_reader.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdeventplus/event.hpp>

#include <csignal>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>

constexpr char DEFAULT_OBJPATH[] = "/xyz/openbmc_project/SensorReader";
constexpr char DEFAULT_BUSNAME[] = "xyz.openbmc_project.SensorReader";
static constexpr auto READER_DIR = "/etc/sensor-reader";

// Global service pointer for signal handler
static std::shared_ptr<phosphor::SensorReader::History> g_historyService;

// Signal handler to save history on shutdown
void signalHandler(int signum)
{
    log<phosphor::logging::level::INFO>(
        "Shutdown/StopProcess signal received",
        phosphor::logging::entry("SIGNAL=%d", signum));
    if (g_historyService)
    {
        // Save history to file before exit
        // We need to access the sensorHistory member, which is protected
        // This will be done via a public method we'll add
        g_historyService->saveHistoryOnShutdown();
    }
    std::exit(0);
}
/*coverity [root_function : FALSE] */
int main(int /*argc*/, char** /*argv*/)
{
    try
    {
        // Register signal handlers for graceful shutdown
        std::signal(SIGTERM, signalHandler);
        std::signal(SIGINT, signalHandler);

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
        g_historyService = std::make_shared<phosphor::SensorReader::History>(
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
