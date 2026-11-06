#include "app-config.h"

#include "app-settings.h"
#include "countdown-font.h"

#include <QFontDatabase>

namespace {

const auto bundledAlarmSound = QStringLiteral("qrc:/sounds/submarine");
const auto animationsKey = QStringLiteral("appearance/animations");

bool animationsFromSettings(const AppSettings &settings)
{
    const QString text = settings.value(animationsKey, QStringLiteral("on")).toString();
    if (const auto on = AppSettings::parseSwitch(text)) {
        return *on;
    }
    qWarning("tTime.ini: invalid animations '%s', using 'on' (choices: on, off)", qPrintable(text));
    return true;
}

} // namespace

AppConfig AppConfig::fromSettings(const AppSettings &settings)
{
    return {themeFromSettings(settings),       countdownFontFromSettings(settings),
            messageFontFromSettings(settings), timeUpMessagePointSizeFromSettings(settings),
            QUrl(bundledAlarmSound),           animationsFromSettings(settings)};
}

AppConfig AppConfig::defaults()
{
    return {defaultTheme(),
            makeCountdownFont(QString::fromLatin1(defaultCountdownFontId), defaultCountdownWeight),
            makeMessageFont(QString::fromLatin1(defaultMessageFontId), defaultMessageWeight),
            defaultTimeUpMessagePointSize,
            QUrl(bundledAlarmSound),
            true};
}

void loadBundledFonts()
{
    for (const auto *font : {":/fonts/sansation_regular", ":/fonts/sansation_light", ":/fonts/oxanium", ":/fonts/jura",
                             ":/fonts/doto", ":/fonts/outfit"}) {
        if (QFontDatabase::addApplicationFont(QString::fromLatin1(font)) < 0) {
            qWarning("Could not load bundled font %s", font);
        }
    }
}
