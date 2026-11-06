#pragma once

#include <QFont>
#include <QString>

#include <optional>

class AppSettings;

// Fonts of the countdown time and of the message below it, configurable in tTime.ini:
//
//   [appearance]
//   countdown-font=oxanium      (oxanium, jura, doto, outfit or sansation)
//   countdown-weight=400        (100-900, or a name such as light, regular, semibold)
//   message-font=outfit         (same choices)
//   message-weight=300
//   countdown-size=18           (point sizes, 6-96)
//   message-size=13
//   time-up-message-size=20     (the message in the "Time's up" window, in the message font)
//
// The defaults are also shipped as tTime.ini next to tTime.exe (src/tTime.ini);
// the user's %APPDATA%\tTime\tTime.ini overrides them.
// Oxanium, Jura, Doto and Outfit are variable fonts, so any weight in a font's
// range works; values outside it are clamped (Oxanium 200-800, Jura 300-700,
// Doto 100-900, Outfit 100-900). Sansation is bundled as Light and Regular (300-400).

inline constexpr const char *defaultCountdownFontId = "oxanium";
inline constexpr int defaultCountdownWeight = 400;
inline constexpr const char *defaultMessageFontId = "outfit";
inline constexpr int defaultMessageWeight = 300;
inline constexpr int defaultCountdownPointSize = 18;
inline constexpr int defaultMessagePointSize = 13;
inline constexpr int defaultTimeUpMessagePointSize = 20;
inline constexpr int minFontPointSize = 6;
inline constexpr int maxFontPointSize = 96;

// The countdown font configured in `settings` (appearance/countdown-font,
// appearance/countdown-weight and appearance/countdown-size), with warnings and
// defaults for invalid values.
[[nodiscard]] QFont countdownFontFromSettings(const AppSettings &settings);

// The message font configured in `settings` (appearance/message-font,
// appearance/message-weight and appearance/message-size), with warnings and
// defaults for invalid values.
[[nodiscard]] QFont messageFontFromSettings(const AppSettings &settings);

// The point size of the message in the "Time's up" window
// (appearance/time-up-message-size).
[[nodiscard]] int timeUpMessagePointSizeFromSettings(const AppSettings &settings);

// Parses a point size setting: a whole number in minFontPointSize..maxFontPointSize.
[[nodiscard]] std::optional<int> parsePointSize(const QString &text);

// Builds the countdown font for a font id, weight and point size, clamping the
// weight to what the font supports. Unknown ids use the default countdown font.
[[nodiscard]] QFont makeCountdownFont(const QString &fontId, int weight, int pointSize = defaultCountdownPointSize);

// Same for the message font (unknown ids use the default message font).
[[nodiscard]] QFont makeMessageFont(const QString &fontId, int weight, int pointSize = defaultMessagePointSize);
