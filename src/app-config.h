#pragma once

#include "countdown-font.h"
#include "theme.h"

#include <QFont>
#include <QString>

class AppSettings;

// The look and sound of tTime, read once at startup and passed to the windows.
// (Window geometry is not part of it: it changes while the app runs and is
// saved through AppSettings.)
struct AppConfig
{
    AppConfig()
        : timeUpMessagePointSize(defaultTimeUpMessagePointSize)
        , animations(true)
    {
    }

    Theme theme;
    QFont countdownFont;
    QFont messageFont; // the alarm message below the countdown
    int timeUpMessagePointSize; // the alarm message in the "Time's up" window
    QString alarmSound; // path of a WAV file; empty = silent
    bool animations; // swaying tree and ringing tray icon in the "Time's up" window

    // From tTime.ini (see AppSettings), with the built-in defaults where a key is missing.
    static AppConfig fromSettings(const AppSettings &settings);
    // The built-in defaults only.
    static AppConfig defaults();
};

// Files tTime cannot use from its compiled-in resources (":/...") are shipped in a
// Resources folder next to the executable; this is the path of one of them, e.g.
// externalResourcePath("sounds/submarine1.wav") -> <exe dir>/Resources/sounds/submarine1.wav.
// (The alarm sound is one: QSound plays files only.)
QString externalResourcePath(const QString &relativePath);
