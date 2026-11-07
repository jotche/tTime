#pragma once

#include <QChar>
#include <QList>
#include <QString>

// Horizontal layout for text whose digits must not move when they change
// (a countdown). Every ASCII digit gets a slot as wide as the widest digit
// and is centred in it; other characters keep their natural width.
struct DigitLayout
{
    DigitLayout()
        : width(0)
    {
    }

    QList<qreal> glyphX; // left edge to draw each character at
    qreal width; // total width, without spacing after the last character
};

// A character's natural width (without letter spacing).
class CharAdvance
{
public:
    virtual ~CharAdvance() {}
    virtual qreal operator()(QChar c) const = 0;
};

// Letter spacing is applied after each character like QFont does it:
// percentage of the (slot) width (`spacingFactor`, 1.0 = none) plus a fixed
// amount in pixels (`absoluteSpacing`).
DigitLayout layoutFixedDigits(const QString &text, const CharAdvance &advance, qreal spacingFactor = 1.0,
                              qreal absoluteSpacing = 0.0);

// Same, with a plain function for the character widths.
DigitLayout layoutFixedDigits(const QString &text, qreal (*advance)(QChar), qreal spacingFactor = 1.0,
                              qreal absoluteSpacing = 0.0);
