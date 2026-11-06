#include "countdown-font.h"

#include "app-settings.h"
#include "font-weight.h"

#include <QStringList>

#include <algorithm>
#include <iterator>

namespace {

const auto fontKey = QStringLiteral("appearance/countdown-font");
const auto weightKey = QStringLiteral("appearance/countdown-weight");

// Bundled variable fonts (SIL OFL, see src/Resources/fonts/*-OFL.txt).
// Each lists both family spellings: some Windows font APIs report the
// default instance's name (e.g. "Oxanium ExtraLight"), others just "Oxanium".
struct BundledFont
{
    const char *id = nullptr;
    QStringList families;
    int minWeight = 0;
    int maxWeight = 0;
};

const BundledFont bundledFonts[] = {
    {"oxanium", {QStringLiteral("Oxanium"), QStringLiteral("Oxanium ExtraLight")}, 200, 800},
    {"jura", {QStringLiteral("Jura"), QStringLiteral("Jura Light")}, 300, 700},
    {"doto", {QStringLiteral("Doto"), QStringLiteral("Doto Black")}, 100, 900},
};

const BundledFont *findFont(const QString &id)
{
    const auto *it
        = std::ranges::find_if(bundledFonts, [&](const BundledFont &f) { return id == QLatin1String(f.id); });
    return it != std::end(bundledFonts) ? &*it : nullptr;
}

const BundledFont &defaultFont()
{
    const BundledFont *font = findFont(QString::fromLatin1(defaultCountdownFontId));
    Q_ASSERT(font);
    return *font;
}

} // namespace

QFont makeCountdownFont(const QString &fontId, int weight)
{
    const BundledFont *chosen = findFont(fontId.trimmed().toLower());
    if (!chosen) {
        chosen = &defaultFont();
    }
    const int clamped = std::clamp(weight, chosen->minWeight, chosen->maxWeight);

    QFont font;
    font.setFamilies(chosen->families);
    font.setPointSize(18);
    font.setWeight(QFont::Weight(clamped));
    // Pin the exact weight on the variable font's axis (named instances only
    // cover multiples of 100; this also makes e.g. 350 work).
    font.setVariableAxis(QFont::Tag("wght"), static_cast<float>(clamped));
    font.setLetterSpacing(QFont::PercentageSpacing, 125);
    return font;
}

QFont countdownFontFromSettings(const AppSettings &settings)
{
    const QString fontId
        = settings.value(fontKey, QString::fromLatin1(defaultCountdownFontId)).toString().trimmed().toLower();
    const BundledFont *font = findFont(fontId);
    if (!font) {
        qWarning("tTime.ini: unknown countdown-font '%s', using '%s' (choices: oxanium, jura, doto)",
                 qPrintable(fontId), defaultCountdownFontId);
        font = &defaultFont();
    }

    const QString weightText = settings.value(weightKey, defaultCountdownWeight).toString();
    int weight = defaultCountdownWeight;
    if (const auto parsed = parseFontWeight(weightText)) {
        weight = *parsed;
    } else {
        qWarning("tTime.ini: invalid countdown-weight '%s', using %d", qPrintable(weightText), defaultCountdownWeight);
    }
    if (weight < font->minWeight || weight > font->maxWeight) {
        qWarning("tTime.ini: countdown-weight %d is outside %s's range %d-%d, using the nearest", weight, font->id,
                 font->minWeight, font->maxWeight);
    }

    return makeCountdownFont(QString::fromLatin1(font->id), weight);
}
