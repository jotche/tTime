#pragma once

#include <vector>

struct Point
{
    int x = 0;
    int y = 0;
    friend bool operator==(const Point &, const Point &) = default;
};

struct Size
{
    int width = 0;
    int height = 0;
    friend bool operator==(const Size &, const Size &) = default;
};

// A rectangle; right() and bottom() are the last pixel inside (like QRect).
struct Rect
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    [[nodiscard]] int left() const noexcept { return x; }
    [[nodiscard]] int top() const noexcept { return y; }
    [[nodiscard]] int right() const noexcept { return x + width - 1; }
    [[nodiscard]] int bottom() const noexcept { return y + height - 1; }
    [[nodiscard]] Point topLeft() const noexcept { return {x, y}; }
    [[nodiscard]] Size size() const noexcept { return {width, height}; }
    [[nodiscard]] bool isValid() const noexcept { return width > 0 && height > 0; }
    friend bool operator==(const Rect &, const Rect &) = default;
};

// Position for the `slot`-th window of a cascade that starts at `base`
// (slot 0 == base). Each slot moves `step` pixels right and down. If moving
// right/down would push the window out of `available`, that axis moves
// left/up instead; if neither fits, the window is clamped inside `available`.
[[nodiscard]] Point cascadePosition(Point base, Size windowSize, const Rect &available, int slot, int step = 32);

struct VisiblePlacement
{
    Rect geometry; // adjusted window geometry, fully inside `area`
    Rect area; // the visible area (screen) the window was fitted into
};

// Makes `window` fully visible on one of `availableAreas`: picks the area it overlaps
// most (or the nearest), moves it by the smallest distance and shrinks it only if it
// is larger than the area. `availableAreas` must not be empty.
[[nodiscard]] VisiblePlacement fitIntoVisibleArea(const Rect &window, const std::vector<Rect> &availableAreas);
