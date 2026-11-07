#include "digit-layout.h"

namespace {

bool isAsciiDigit(QChar c)
{
    return c >= QLatin1Char('0') && c <= QLatin1Char('9');
}

class FunctionAdvance : public CharAdvance
{
public:
    explicit FunctionAdvance(qreal (*function)(QChar))
        : m_function(function)
    {
    }
    qreal operator()(QChar c) const { return m_function(c); }

private:
    qreal (*m_function)(QChar);
};

} // namespace

DigitLayout layoutFixedDigits(const QString &text, const CharAdvance &advance, qreal spacingFactor,
                              qreal absoluteSpacing)
{
    qreal digitSlot = 0;
    for (char d = '0'; d <= '9'; ++d) {
        digitSlot = qMax(digitSlot, advance(QLatin1Char(d)));
    }

    DigitLayout layout;
    layout.glyphX.reserve(text.size());

    qreal x = 0;
    qreal lastSpacing = 0;
    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);
        const qreal natural = advance(c);
        const qreal slot = isAsciiDigit(c) ? digitSlot : natural;
        layout.glyphX.append(x + (slot - natural) / 2);

        lastSpacing = slot * (spacingFactor - 1.0) + absoluteSpacing;
        x += slot + lastSpacing;
    }
    layout.width = text.isEmpty() ? 0 : x - lastSpacing;
    return layout;
}

DigitLayout layoutFixedDigits(const QString &text, qreal (*advance)(QChar), qreal spacingFactor,
                              qreal absoluteSpacing)
{
    return layoutFixedDigits(text, FunctionAdvance(advance), spacingFactor, absoluteSpacing);
}
