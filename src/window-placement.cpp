#include "window-placement.h"

#include <algorithm>

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
qint64 distanceSquared(const QRect &a, const QRect &b)
{
    const qint64 dx = std::max({0, a.left() - b.right(), b.left() - a.right()});
    const qint64 dy = std::max({0, a.top() - b.bottom(), b.top() - a.bottom()});
    return dx * dx + dy * dy;
}

qint64 overlapArea(const QRect &a, const QRect &b)
{
    const QRect overlap = a.intersected(b);
    return overlap.isEmpty() ? 0 : static_cast<qint64>(overlap.width()) * overlap.height();
}

} // namespace

VisiblePlacement fitIntoVisibleArea(const QRect &window, const QList<QRect> &availableAreas)
{
    Q_ASSERT(!availableAreas.isEmpty());

    // Best area: largest overlap first, then smallest distance.
    const QRect area = *std::ranges::min_element(availableAreas, [&](const QRect &a, const QRect &b) {
        const qint64 overlapA = overlapArea(window, a);
        const qint64 overlapB = overlapArea(window, b);
        if (overlapA != overlapB) {
            return overlapA > overlapB;
        }
        return distanceSquared(window, a) < distanceSquared(window, b);
    });

    const QSize size = window.size().boundedTo(area.size()).expandedTo(QSize(1, 1));
    const int x = std::clamp(window.x(), area.left(), area.right() - size.width() + 1);
    const int y = std::clamp(window.y(), area.top(), area.bottom() - size.height() + 1);
    return {QRect(QPoint(x, y), size), area};
}

QPoint cascadePosition(QPoint base, QSize windowSize, const QRect &available, int slot, int step)
{
    const int offset = std::max(slot, 0) * step;
    return {placeOnAxis(base.x(), windowSize.width(), available.left(), available.right(), offset),
            placeOnAxis(base.y(), windowSize.height(), available.top(), available.bottom(), offset)};
}
