#include "window-placement.h"

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
    return qBound(first, forward, qMax(first, last - length + 1));
}

// Squared distance between two rectangles (0 if they touch or overlap).
qint64 distanceSquared(const QRect &a, const QRect &b)
{
    const qint64 dx = qMax(0, qMax(a.left() - b.right(), b.left() - a.right()));
    const qint64 dy = qMax(0, qMax(a.top() - b.bottom(), b.top() - a.bottom()));
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
    QRect area = availableAreas.first();
    for (int i = 1; i < availableAreas.size(); ++i) {
        const QRect &candidate = availableAreas.at(i);
        const qint64 overlapCandidate = overlapArea(window, candidate);
        const qint64 overlapBest = overlapArea(window, area);
        if (overlapCandidate > overlapBest
            || (overlapCandidate == overlapBest
                && distanceSquared(window, candidate) < distanceSquared(window, area))) {
            area = candidate;
        }
    }

    const QSize size = window.size().boundedTo(area.size()).expandedTo(QSize(1, 1));
    const int x = qBound(area.left(), window.x(), area.right() - size.width() + 1);
    const int y = qBound(area.top(), window.y(), area.bottom() - size.height() + 1);

    VisiblePlacement placement;
    placement.geometry = QRect(QPoint(x, y), size);
    placement.area = area;
    return placement;
}

QPoint cascadePosition(const QPoint &base, const QSize &windowSize, const QRect &available, int slot, int step)
{
    const int offset = qMax(slot, 0) * step;
    return QPoint(placeOnAxis(base.x(), windowSize.width(), available.left(), available.right(), offset),
                  placeOnAxis(base.y(), windowSize.height(), available.top(), available.bottom(), offset));
}
