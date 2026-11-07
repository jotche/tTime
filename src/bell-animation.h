#pragma once

#include <QImage>
#include <QPointF>

class QPainter;

// Animates the alarm icon like a ringing bell: the colored background stays
// still while the white bell swings from the top of its handle and the clapper
// follows a little later.
//
// The icon is split into layers once: white artwork (the bell and the clapper
// dot below it) is cut out with soft edges, and the hole it leaves in the
// background is filled from the surrounding pixels.
class BellAnimation
{
public:
    explicit BellAnimation(const QImage &icon);

    // False if the icon has no recognisable white bell (callers should then
    // fall back to animating the whole icon).
    bool isValid() const { return m_valid; }

    // Replaces the background layer (e.g. with a recolored copy); must have the
    // same size as the icon.
    void setBackground(const QImage &background);

    // One frame: bell rotated by `bellAngle`, clapper by `clapperAngle` (degrees,
    // around the top of the bell's handle).
    QImage frame(double bellAngle, double clapperAngle) const;

    // The layers, for tests and debugging.
    const QImage &background() const { return m_background; }
    const QImage &bell() const { return m_bell; }
    const QImage &clapper() const { return m_clapper; }
    QPointF pivot() const { return m_pivot; }

private:
    void drawRotated(QPainter &painter, const QImage &layer, double angle) const;

    QImage m_background;
    QImage m_bell;
    QImage m_clapper;
    QPointF m_pivot;
    bool m_valid;
};
