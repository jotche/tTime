#pragma once

#include <QImage>
#include <QPointF>

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
    [[nodiscard]] bool isValid() const noexcept { return m_valid; }

    // Replaces the background layer (e.g. with a recolored copy); must have the
    // same size as the icon.
    void setBackground(const QImage &background);

    // One frame: bell rotated by `bellAngle`, clapper by `clapperAngle` (degrees,
    // around the top of the bell's handle).
    [[nodiscard]] QImage frame(double bellAngle, double clapperAngle) const;

    // The layers, for tests and debugging.
    [[nodiscard]] const QImage &background() const noexcept { return m_background; }
    [[nodiscard]] const QImage &bell() const noexcept { return m_bell; }
    [[nodiscard]] const QImage &clapper() const noexcept { return m_clapper; }
    [[nodiscard]] QPointF pivot() const noexcept { return m_pivot; }

private:
    QImage m_background;
    QImage m_bell;
    QImage m_clapper;
    QPointF m_pivot;
    bool m_valid = false;
};
