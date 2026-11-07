#pragma once

#include <QList>
#include <QPoint>
#include <QRect>
#include <QSize>

// Position for the `slot`-th window of a cascade that starts at `base`
// (slot 0 == base). Each slot moves `step` pixels right and down. If moving
// right/down would push the window out of `available`, that axis moves
// left/up instead; if neither fits, the window is clamped inside `available`.
QPoint cascadePosition(const QPoint &base, const QSize &windowSize, const QRect &available, int slot, int step = 32);

struct VisiblePlacement
{
    QRect geometry; // adjusted window geometry, fully inside `area`
    QRect area; // the visible area (screen) the window was fitted into
};

// Makes `window` fully visible on one of `availableAreas` (the usable screen
// areas, i.e. without taskbars). Picks the area the window overlaps most, or,
// if it is entirely off-screen, the nearest area. The window is moved by the
// smallest distance needed (clamped per axis) and shrunk only if it is larger
// than the area. A window that is already fully visible is returned unchanged.
// `availableAreas` must not be empty.
VisiblePlacement fitIntoVisibleArea(const QRect &window, const QList<QRect> &availableAreas);
