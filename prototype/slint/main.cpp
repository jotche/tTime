// tTime with a Slint user interface: countdown alarms in the system tray.
//
//   ttime-slint.exe                  opens the "new alarm" input
//   ttime-slint.exe 5m green tea
//   ttime-slint.exe --help

#include "alarm-manager.h"
#include "config.h"
#include "platform.h"
#include "settings.h"

#include <slint.h>

#include <cstdlib>
#include <string>
#include <vector>

#ifndef TTIME_VERSION
#    define TTIME_VERSION "0.0.0"
#endif

namespace {

const char *const helpText = "Usage: tTime [options] [alarm...]\n"
                             "Countdown alarms in the system tray.\n"
                             "\n"
                             "Options:\n"
                             "  -?, -h, --help  Displays help on commandline options.\n"
                             "  -v, --version   Displays version information.\n"
                             "\n"
                             "Arguments:\n"
                             "  [alarm...]      Duration and optional message, e.g. \"5m green tea\" or\n"
                             "                  \"1h 30m meeting\". Opens an input window if omitted.";

} // namespace

int main(int argc, char *argv[])
{
    // Slint's software renderer: no GPU context, the smallest memory use. SLINT_BACKEND
    // still chooses another one (e.g. winit-skia).
    if (!std::getenv("SLINT_BACKEND")) {
#ifdef _WIN32
        _putenv_s("SLINT_BACKEND", "winit-software");
#else
        setenv("SLINT_BACKEND", "winit-software", 0);
#endif
    }

    std::string spec;
    for (const std::string &argument : platform::arguments(argc, argv)) {
        if (argument == "-h" || argument == "--help" || argument == "-?" || argument == "/?") {
            platform::showMessage("tTime", helpText);
            return 0;
        }
        if (argument == "-v" || argument == "--version") {
            platform::showMessage("tTime", "tTime " TTIME_VERSION);
            return 0;
        }
        if (argument.size() > 1 && argument[0] == '-' && !(argument[1] >= '0' && argument[1] <= '9')) {
            platform::showMessage("tTime", "Unknown option '" + argument.substr(argument.find_first_not_of('-')) + "'.",
                                  true);
            return 1;
        }
        spec += (spec.empty() ? "" : " ") + argument;
    }

    // Settings are read once: %APPDATA%\tTime\tTime.ini, then <exe dir>\tTime.ini.
    AppSettings settings = AppSettings::forApplication();
    const AppConfig config = AppConfig::fromSettings(settings);

    AlarmManager manager(config, settings);
    manager.onIdle = [] { slint::quit_event_loop(); };
    manager.start(spec);
    // Alarms may live only in the tray; the manager decides when to quit.
    slint::run_event_loop(slint::EventLoopMode::RunUntilQuit);
    return 0;
}
