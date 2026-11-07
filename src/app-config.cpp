#include "app-config.h"

#include "app-settings.h"
#include "countdown-font.h"

#include <QCoreApplication>

namespace {

// In the Resources folder next to tTime.exe (see externalResourcePath()).
const char *const bundledAlarmSound = "sounds/submarine1.wav";
const char *const animationsKey = "appearance/animations";

bool animationsFromSettings(const AppSettings &settings)
{
    const QString text = settings.value(QLatin1String(animationsKey), QLatin1String("on")).toString();
    bool on = true;
    if (AppSettings::parseSwitch(text, &on)) {
        return on;
    }
    qWarning("tTime.ini: invalid animations '%s', using 'on' (choices: on, off)", qPrintable(text));
    return true;
}

} // namespace

AppConfig AppConfig::fromSettings(const AppSettings &settings)
{
    AppConfig config;
    config.theme = themeFromSettings(settings);
    config.countdownFont = countdownFontFromSettings(settings);
    config.messageFont = messageFontFromSettings(settings);
    config.timeUpMessagePointSize = timeUpMessagePointSizeFromSettings(settings);
    config.alarmSound = externalResourcePath(QLatin1String(bundledAlarmSound));
    config.animations = animationsFromSettings(settings);
    return config;
}

AppConfig AppConfig::defaults()
{
    AppConfig config;
    config.theme = defaultTheme();
    config.countdownFont = makeCountdownFont(QLatin1String(defaultCountdownFontId), defaultCountdownWeight);
    config.messageFont = makeMessageFont(QLatin1String(defaultMessageFontId), defaultMessageWeight);
    config.timeUpMessagePointSize = defaultTimeUpMessagePointSize;
    config.alarmSound = externalResourcePath(QLatin1String(bundledAlarmSound));
    config.animations = true;
    return config;
}

QString externalResourcePath(const QString &relativePath)
{
    return QCoreApplication::applicationDirPath() + QLatin1String("/Resources/") + relativePath;
}
