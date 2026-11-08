#include "window-placement.h"

#include <algorithm>
#include <cassert>

namespace {

// One axis of the cascade. `first`/`last` are the inclusive edges of the visible area.
int placeOnAxis(int base, int length, int first, int last, int offset)
{
    if (offset == 0) {
        return base;
    }
    const int forward = base + offset;
    if (forward + length - 1 <= last) {
        return forward;
    }
    const int backward = base - offset;
    if (backward >= first) {
        return backward;
    }
    // Window wider/taller than the free room in both directions: keep it visible.
    return std::clamp(forward, first, std::max(first, last - length + 1));
}

// Squared distance between two rectangles (0 if they touch or overlap).
long long distanceSquared(const Rect &a, const Rect &b)
{
    const long long dx = std::max({0, a.left() - b.right(), b.left() - a.right()});
    const long long dy = std::max({0, a.top() - b.bottom(), b.top() - a.bottom()});
    return dx * dx + dy * dy;
}

long long overlapArea(const Rect &a, const Rect &b)
{
    const int w = std::min(a.right(), b.right()) - std::max(a.left(), b.left()) + 1;
    const int h = std::min(a.bottom(), b.bottom()) - std::max(a.top(), b.top()) + 1;
    return w > 0 && h > 0 ? static_cast<long long>(w) * h : 0;
}

} // namespace

VisiblePlacement fitIntoVisibleArea(const Rect &window, const std::vector<Rect> &availableAreas)
{
    assert(!availableAreas.empty());

    // Best area: largest overlap first, then smallest distance.
    const Rect area = *std::ranges::min_element(availableAreas, [&](const Rect &a, const Rect &b) {
        const long long overlapA = overlapArea(window, a);
        const long long overlapB = overlapArea(window, b);
        if (overlapA != overlapB) {
            return overlapA > overlapB;
        }
        return distanceSquared(window, a) < distanceSquared(window, b);
    });

    const int width = std::max(1, std::min(window.width, area.width));
    const int height = std::max(1, std::min(window.height, area.height));
    const int x = std::clamp(window.x, area.left(), area.right() - width + 1);
    const int y = std::clamp(window.y, area.top(), area.bottom() - height + 1);
    return {{x, y, width, height}, area};
}

Point cascadePosition(Point base, Size windowSize, const Rect &available, int slot, int step)
{
    const int offset = std::max(slot, 0) * step;
    return {placeOnAxis(base.x, windowSize.width, available.left(), available.right(), offset),
            placeOnAxis(base.y, windowSize.height, available.top(), available.bottom(), offset)};
}
