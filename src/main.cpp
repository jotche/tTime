#include "alarm-manager.h"
#include "app-config.h"
#include "app-settings.h"

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // Qt 6.8+ renders text with DirectWrite by default, which roughly doubles the
    // private memory of this small tray app (about 17 MB instead of 9 MB). FreeType
    // keeps the variable font axes working. QT_QPA_PLATFORM or -platform still win.
    if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "windows:fontengine=freetype");
    }
#endif

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
