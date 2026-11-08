#pragma once

#include "image.h"

// Animates the alarm icon like a ringing bell: the colored background stays still
// while the white bell swings from the top of its handle and the clapper follows a
// little later.
//
// The icon is split into layers once: white artwork (the bell and the clapper dot
// below it) is cut out with soft edges, and the hole it leaves in the background is
// filled from the surrounding pixels. Images in and out are not premultiplied.
class BellAnimation
{
public:
    explicit BellAnimation(const Image &icon);

    // False if the icon has no recognisable white bell.
    [[nodiscard]] bool isValid() const noexcept { return m_valid; }

    // Replaces the background layer (e.g. with a recolored copy); same size as the icon.
    void setBackground(const Image &background);

    // One frame: bell rotated by `bellAngle`, clapper by `clapperAngle` (degrees,
    // around the top of the bell's handle).
    [[nodiscard]] Image frame(double bellAngle, double clapperAngle) const;

    [[nodiscard]] const Image &background() const noexcept { return m_background; }

private:
    Image m_background;
    Image m_bell; // premultiplied
    Image m_clapper; // premultiplied
    double m_pivotX = 0;
    double m_pivotY = 0;
    bool m_valid = false;
};

// Helpers for splitting white artwork out of tTime's pictures (also used by TreeSway).
namespace imagelayers {

// "Whiteness" of a pixel: its darkest channel.
[[nodiscard]] int whiteness(Rgba color);
// Clearly white artwork: opaque, bright and nearly grey.
[[nodiscard]] bool isWhiteArtwork(Rgba color);

} // namespace imagelayers
