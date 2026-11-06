#pragma once

#include <QImage>

// Animates the tree picture of the "Time's up" window as if moved by a light
// breeze: the frame and the ground stay still, the tree bends (its base stays
// planted, the crown moves most) and the leaves shimmer.
//
// The picture is warped rather than cut apart: inside the frame and above the
// ground, every pixel is taken from a slightly shifted position. The background
// fill there is an even, translucent color, so shifting it is invisible and only
// the tree visibly moves; with no wind the picture is reproduced exactly.
class TreeSway
{
public:
    // Tuning of the breeze used by the "Time's up" window.
    static constexpr double breezePeriod = 6.0; // seconds for one full sway cycle
    static constexpr double breezeSway = 4.0; // pixels the crown top moves each way
    static constexpr double leafFlutter = 0.8; // pixels of leaf shimmer in the crown
    static constexpr double leafFlutterPeriod = 1.2; // seconds (fits 5 times into breezePeriod)

    explicit TreeSway(const QImage &picture);

    // False if no frame, ground line or tree was found (callers then show the
    // picture without animation).
    [[nodiscard]] bool isValid() const noexcept { return m_valid; }

    // One frame. `sway`: sideways movement of the crown top in pixels (negative =
    // left); every row moves by sway * s^2, where s goes from 0 at the ground line
    // to 1 at the crown top. `flutter` (pixels) and `flutterPhase` (radians): a
    // small, smooth ripple in the crown only.
    [[nodiscard]] QImage frame(double sway, double flutterPhase = 0, double flutter = 0) const;

    // Strength of the breeze at `seconds`, in [-1, 1]: a slow sway with a lighter,
    // faster ripple on top. Repeats every breezePeriod seconds.
    [[nodiscard]] static double breeze(double seconds);

    // For tests and debugging.
    [[nodiscard]] int groundLine() const noexcept { return m_groundLine; }
    [[nodiscard]] int crownTop() const noexcept { return m_crownTop; }
    [[nodiscard]] bool insideFrame(int x, int y) const;

private:
    QImage m_picture; // as given (unchanged pixels are copied from here)
    QImage m_premultiplied; // for sampling moved pixels
    int m_groundLine = 0; // first row of the ground (does not move)
    int m_crownTop = 0; // highest row of the tree
    int m_band = 0; // width of the frame band (left, top, right)
    int m_corner = 0; // size of the rounded top corners treated as frame
    bool m_valid = false;
};
