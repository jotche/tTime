#pragma once

#include "image.h"

// Animates the tree picture of the "Time's up" window as if moved by a light breeze:
// the frame and the ground stay still, the tree bends (its base stays planted, the
// crown moves most) and the leaves shimmer. Every pixel inside the frame and above
// the ground is taken from a slightly shifted position; with no wind the picture is
// reproduced exactly. Images in and out are not premultiplied.
class TreeSway
{
public:
    static constexpr double breezePeriod = 6.0; // seconds for one full sway cycle
    static constexpr double breezeSway = 4.0; // pixels the crown top moves each way
    static constexpr double leafFlutter = 0.8; // pixels of leaf shimmer in the crown
    static constexpr double leafFlutterPeriod = 1.2; // seconds (fits 5 times into breezePeriod)

    explicit TreeSway(const Image &picture);

    // False if no frame, ground line or tree was found.
    [[nodiscard]] bool isValid() const noexcept { return m_valid; }

    // One frame: `sway` = sideways movement of the crown top in pixels; `flutter`
    // (pixels) and `flutterPhase` (radians) = a small ripple in the crown only.
    [[nodiscard]] Image frame(double sway, double flutterPhase = 0, double flutter = 0) const;

    // Strength of the breeze at `seconds`, in [-1, 1]. Repeats every breezePeriod seconds.
    [[nodiscard]] static double breeze(double seconds);

    [[nodiscard]] int groundLine() const noexcept { return m_groundLine; }
    [[nodiscard]] int crownTop() const noexcept { return m_crownTop; }

private:
    [[nodiscard]] bool insideFrame(int x, int y) const;

    Image m_picture;
    Image m_premultiplied;
    int m_groundLine = 0; // first row of the ground (does not move)
    int m_crownTop = 0; // highest row of the tree
    int m_band = 0; // width of the frame band (left, top, right)
    int m_corner = 0; // size of the rounded top corners treated as frame
    bool m_valid = false;
};
