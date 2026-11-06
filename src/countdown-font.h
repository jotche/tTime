#pragma once

#include <QFont>
#include <QString>

class AppSettings;

// Font of the countdown time, configurable in tTime.ini:
//
//   [appearance]
//   countdown-font=oxanium      (oxanium, jura or doto)
//   countdown-weight=400        (100-900, or a name such as light, regular, semibold)
//
// The defaults are also shipped as tTime.ini next to tTime.exe (src/tTime.ini);
// the user's %APPDATA%\tTime\tTime.ini overrides them.
// All bundled fonts are variable fonts, so any weight in a font's range works;
// values outside it are clamped (Oxanium 200-800, Jura 300-700, Doto 100-900).

inline constexpr const char *defaultCountdownFontId = "oxanium";
inline constexpr int defaultCountdownWeight = 400;

// The countdown font configured in `settings` (appearance/countdown-font and
// appearance/countdown-weight), with warnings and defaults for invalid values.
[[nodiscard]] QFont countdownFontFromSettings(const AppSettings &settings);

// Builds the countdown font for a font id and weight, clamping the weight to
// what the font supports. Unknown ids use the default font. Used by tests.
[[nodiscard]] QFont makeCountdownFont(const QString &fontId, int weight);
