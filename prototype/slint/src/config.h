#pragma once

#include "theme.h"

#include <optional>
#include <string>

class AppSettings;

// A font of one of tTime's texts: a bundled font family, a weight and a size.
struct FontChoice
{
    std::string id; // oxanium, jura, doto, outfit, sansation
    std::string family; // as registered from the font file
    int weight = 400; // clamped to the font's range
    int pointSize = 13;
};

// Fonts (tTime.ini, [appearance]):
//   countdown-font=oxanium      (oxanium, jura, doto, outfit or sansation)
//   countdown-weight=400        (100-900, or a name such as light, regular, semibold)
//   message-font=outfit         (same choices)
//   message-weight=300
//   countdown-size=18           (point sizes, 6-96)
//   message-size=13
//   time-up-message-size=13
inline constexpr const char *defaultCountdownFontId = "oxanium";
inline constexpr int defaultCountdownWeight = 400;
inline constexpr const char *defaultMessageFontId = "outfit";
inline constexpr int defaultMessageWeight = 300;
inline constexpr int defaultCountdownPointSize = 18;
inline constexpr int defaultMessagePointSize = 13;
inline constexpr int defaultTimeUpMessagePointSize = 13;
inline constexpr int minFontPointSize = 6;
inline constexpr int maxFontPointSize = 96;

// The look of tTime, read once at startup and passed to the windows.
struct AppConfig
{
    Theme theme = defaultTheme();
    FontChoice countdownFont;
    FontChoice messageFont;
    int timeUpMessagePointSize = defaultTimeUpMessagePointSize;
    bool animations = true; // swaying tree and ringing tray icon in the "Time's up" window

    // From tTime.ini (see AppSettings); invalid values log a warning and use the default.
    [[nodiscard]] static AppConfig fromSettings(const AppSettings &settings);
    [[nodiscard]] static AppConfig defaults();
};

// A font weight: a number (1-1000) or a name ("light", "semibold", "semi-bold", ...).
[[nodiscard]] std::optional<int> parseFontWeight(const std::string &text);
// A point size: a whole number in minFontPointSize..maxFontPointSize.
[[nodiscard]] std::optional<int> parsePointSize(const std::string &text);
// The font for an id and weight (unknown ids use `defaultId`; the weight is clamped).
[[nodiscard]] FontChoice makeFont(const std::string &id, int weight, int pointSize, const char *defaultId);
// Point size to Slint's logical pixels (Qt uses 96 dpi on Windows/Linux and 72 on macOS).
[[nodiscard]] float pointsToPixels(int pointSize);
