#pragma once

#include <QList>
#include <QStringView>

#include <functional>

// Horizontal layout for text whose digits must not move when they change
// (a countdown). Every ASCII digit gets a slot as wide as the widest digit
// and is centred in it; other characters keep their natural width.
struct DigitLayout
{
    QList<qreal> glyphX; // left edge to draw each character at
    qreal width = 0; // total width, without spacing after the last character
};

// `advance` returns a character's natural width (without letter spacing).
// Letter spacing is applied after each character like QFont does it:
// percentage of the (slot) width (`spacingFactor`, 1.0 = none) plus a fixed
// amount in pixels (`absoluteSpacing`).
[[nodiscard]] DigitLayout layoutFixedDigits(QStringView text, const std::function<qreal(QChar)> &advance,
                                            qreal spacingFactor = 1.0, qreal absoluteSpacing = 0.0);
