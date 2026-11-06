#include "alarm-manager.h"
#include "app-config.h"
#include "app-settings.h"

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("tTime"));
    QApplication::setOrganizationName(QStringLiteral("tTime"));
    QApplication::setApplicationVersion(QStringLiteral(TTIME_VERSION));
    // Alarms may live only in the tray; the manager decides when to quit.
    QApplication::setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Countdown alarms in the system tray."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("alarm"),
                                 QStringLiteral("Duration and optional message, e.g. \"5m green tea\" "
                                                "or \"1h 30m meeting\". Opens an input window if omitted."),
                                 QStringLiteral("[alarm...]"));
    parser.process(app);

    loadBundledFonts();

    // Settings are read once: %APPDATA%\tTime\tTime.ini, then <exe dir>\tTime.ini.
    AppSettings settings = AppSettings::forApplication();
    const AppConfig config = AppConfig::fromSettings(settings);
    // Default icon for Qt's own windows and dialogs. Each tTime window also sets
    // it explicitly (Windows shows a window's own icon on the taskbar).
    QApplication::setWindowIcon(themedAppIcon(config.theme));

    AlarmManager manager(config, settings);
    QObject::connect(&manager, &AlarmManager::idle, &app, &QCoreApplication::quit);
    manager.start(parser.positionalArguments().join(QLatin1Char(' ')));
    return QApplication::exec();
}
