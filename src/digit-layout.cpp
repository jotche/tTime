#include "digit-layout.h"

#include <algorithm>

namespace {

constexpr bool isAsciiDigit(QChar c) noexcept
{
    return c >= QLatin1Char('0') && c <= QLatin1Char('9');
}

} // namespace

DigitLayout layoutFixedDigits(QStringView text, const std::function<qreal(QChar)> &advance, qreal spacingFactor,
                              qreal absoluteSpacing)
{
    qreal digitSlot = 0;
    for (char d = '0'; d <= '9'; ++d) {
        digitSlot = std::max(digitSlot, advance(QLatin1Char(d)));
    }

    DigitLayout layout;
    layout.glyphX.reserve(text.size());

    qreal x = 0;
    qreal lastSpacing = 0;
    for (const QChar c : text) {
        const qreal natural = advance(c);
        const qreal slot = isAsciiDigit(c) ? digitSlot : natural;
        layout.glyphX.append(x + (slot - natural) / 2);

        lastSpacing = slot * (spacingFactor - 1.0) + absoluteSpacing;
        x += slot + lastSpacing;
    }
    layout.width = text.isEmpty() ? 0 : x - lastSpacing;
    return layout;
}
