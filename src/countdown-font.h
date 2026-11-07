#pragma once

#include <QFont>
#include <QString>

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
//   time-up-message-size=13     (the message in the "Time's up" window, in the message font)
//
// The defaults are also shipped as tTime.ini next to tTime.exe (src/tTime.ini);
// the user's %APPDATA%\tTime\tTime.ini overrides them.
//
// Qt 4 cannot pick a weight from a variable font, so every weight is bundled as a
// static font file of its own and a weight is rounded to the nearest hundred.
// Values outside a font's range are clamped (Oxanium 200-800, Jura 300-700,
// Outfit 100-900, Sansation 300-400; Doto is bundled as Black, 900, only).

const char *const defaultCountdownFontId = "oxanium";
const int defaultCountdownWeight = 400;
const char *const defaultMessageFontId = "outfit";
const int defaultMessageWeight = 300;
const int defaultCountdownPointSize = 18;
const int defaultMessagePointSize = 13;
const int defaultTimeUpMessagePointSize = 13;
const int minFontPointSize = 6;
const int maxFontPointSize = 96;

// The countdown font configured in `settings` (appearance/countdown-font,
// appearance/countdown-weight and appearance/countdown-size), with warnings and
// defaults for invalid values.
QFont countdownFontFromSettings(const AppSettings &settings);

// The message font configured in `settings` (appearance/message-font,
// appearance/message-weight and appearance/message-size), with warnings and
// defaults for invalid values.
QFont messageFontFromSettings(const AppSettings &settings);

// The point size of the message in the "Time's up" window
// (appearance/time-up-message-size).
int timeUpMessagePointSizeFromSettings(const AppSettings &settings);

// Parses a point size setting: a whole number in minFontPointSize..maxFontPointSize.
// Returns false otherwise; `size` may be 0.
bool parsePointSize(const QString &text, int *size = 0);

// The weight (100-900) of a bundled font that is used for a requested weight:
// rounded to the nearest hundred and clamped to the font's range. Unknown ids use
// the default countdown font.
int bundledFontWeight(const QString &fontId, int weight);

// The family name the bundled font registers for a weight (from bundledFontWeight()),
// e.g. "Oxanium Light" for 300 or "Oxanium" for 400 and 700 (the bold style).
QString bundledFontFamily(const QString &fontId, int weight);

// Builds the countdown font for a font id, weight and point size (see
// bundledFontWeight()). Unknown ids use the default countdown font.
QFont makeCountdownFont(const QString &fontId, int weight, int pointSize = defaultCountdownPointSize);

// Same for the message font (unknown ids use the default message font).
QFont makeMessageFont(const QString &fontId, int weight, int pointSize = defaultMessagePointSize);

// Registers the fonts compiled into tTime (call once, after the QApplication exists).
void loadBundledFonts();
