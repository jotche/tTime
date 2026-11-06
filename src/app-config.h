#pragma once

#include "theme.h"

#include <QFont>
#include <QUrl>

class AppSettings;

// The look and sound of tTime, read once at startup and passed to the windows.
// (Window geometry is not part of it: it changes while the app runs and is
// saved through AppSettings.)
struct AppConfig
{
    Theme theme;
    QFont countdownFont;
    QFont messageFont; // the alarm message below the countdown
    int timeUpMessagePointSize = 20; // the alarm message in the "Time's up" window
    QUrl alarmSound; // empty = silent
    bool animations = true; // swaying tree and ringing tray icon in the "Time's up" window

    // From tTime.ini (see AppSettings), with the built-in defaults where a key is missing.
    [[nodiscard]] static AppConfig fromSettings(const AppSettings &settings);
    // The built-in defaults only.
    [[nodiscard]] static AppConfig defaults();
};

// Registers the fonts compiled into tTime (call once, after the QGuiApplication exists).
void loadBundledFonts();
