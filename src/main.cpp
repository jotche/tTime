#include "alarm-manager.h"
#include "app-config.h"
#include "app-settings.h"
#include "countdown-font.h"

#include <QApplication>
#include <QMessageBox>
#include <QStringList>

namespace {

QString usage()
{
    return QLatin1String("Usage: tTime [options] [alarm...]\n"
                         "Countdown alarms in the system tray.\n\n"
                         "Options:\n"
                         "  -h, --help     Displays this help.\n"
                         "  -v, --version  Displays version information.\n\n"
                         "Arguments:\n"
                         "  alarm          Duration and optional message, e.g. \"5m green tea\"\n"
                         "                 or \"1h 30m meeting\". Opens an input window if omitted.");
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QLatin1String("tTime"));
    QApplication::setOrganizationName(QLatin1String("tTime"));
    QApplication::setApplicationVersion(QLatin1String(TTIME_VERSION));
    // Alarms may live only in the tray; the manager decides when to quit.
    QApplication::setQuitOnLastWindowClosed(false);

    QStringList alarm = QApplication::arguments();
    alarm.removeFirst(); // the program
    if (alarm.size() == 1) {
        const QString option = alarm.first();
        // tTime has no console on Windows, so these are shown in a message box.
        if (option == QLatin1String("-h") || option == QLatin1String("--help") || option == QLatin1String("-?")) {
            QMessageBox::information(0, QLatin1String("tTime"), usage());
            return 0;
        }
        if (option == QLatin1String("-v") || option == QLatin1String("--version")) {
            QMessageBox::information(0, QLatin1String("tTime"), QLatin1String("tTime " TTIME_VERSION));
            return 0;
        }
    }

    loadBundledFonts();

    // Settings are read once: %APPDATA%\tTime\tTime.ini, then <exe dir>\tTime.ini.
    AppSettings settings(AppSettings::userFilePath(), AppSettings::defaultsFilePath());
    const AppConfig config = AppConfig::fromSettings(settings);
    // Default icon for Qt's own windows and dialogs. Each tTime window also sets
    // it explicitly (Windows shows a window's own icon on the taskbar).
    QApplication::setWindowIcon(themedAppIcon(config.theme));

    AlarmManager manager(config, settings);
    QObject::connect(&manager, SIGNAL(idle()), &app, SLOT(quit()));
    manager.start(alarm.join(QLatin1String(" ")));
    return QApplication::exec();
}
