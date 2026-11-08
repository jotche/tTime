//! Window cascading and keeping windows on screen (port of window-placement.cpp).

/// A rectangle with QRect semantics: `right() == x + width - 1`,
/// `bottom() == y + height - 1`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct Rect {
    pub x: i32,
    pub y: i32,
    pub width: i32,
    pub height: i32,
}

impl Rect {
    pub const fn new(x: i32, y: i32, width: i32, height: i32) -> Self {
        Self { x, y, width, height }
    }

    pub const fn left(&self) -> i32 {
        self.x
    }

    pub const fn top(&self) -> i32 {
        self.y
    }

    pub const fn right(&self) -> i32 {
        self.x + self.width - 1
    }

    pub const fn bottom(&self) -> i32 {
        self.y + self.height - 1
    }

    pub const fn is_empty(&self) -> bool {
        self.width <= 0 || self.height <= 0
    }

    /// The overlap of two (non-empty) rectangles; empty if they do not overlap.
    pub fn intersected(&self, other: &Rect) -> Rect {
        let left = self.left().max(other.left());
        let top = self.top().max(other.top());
        let right = self.right().min(other.right());
        let bottom = self.bottom().min(other.bottom());
        if self.is_empty() || other.is_empty() || left > right || top > bottom {
            return Rect::default();
        }
        Rect::new(left, top, right - left + 1, bottom - top + 1)
    }

    /// True if `other` lies completely inside this rectangle.
    pub fn contains_rect(&self, other: &Rect) -> bool {
        !self.is_empty()
            && !other.is_empty()
            && other.left() >= self.left()
            && other.right() <= self.right()
            && other.top() >= self.top()
            && other.bottom() <= self.bottom()
    }
}

/// One axis of the cascade. `first`/`last` are the inclusive edges of the visible area.
fn place_on_axis(base: i32, length: i32, first: i32, last: i32, offset: i32) -> i32 {
    if offset == 0 {
        return base;
    }
    let forward = base + offset;
    if forward + length - 1 <= last {
        return forward;
    }
    let backward = base - offset;
    if backward >= first {
        return backward;
    }
    // Window wider/taller than the free room in both directions: keep it visible.
    forward.clamp(first, first.max(last - length + 1))
}

/// Position for the `slot`-th window of a cascade that starts at `base`
/// (slot 0 == base). Each slot moves `step` pixels right and down (the C++
/// default is 32). If moving right/down would push the window out of
/// `available`, that axis moves left/up instead; if neither fits, the window is
/// clamped inside `available`.
pub fn cascade_position(
    base: (i32, i32),
    window_size: (i32, i32),
    available: &Rect,
    slot: i32,
    step: i32,
) -> (i32, i32) {
    let offset = slot.max(0) * step;
    (
        place_on_axis(base.0, window_size.0, available.left(), available.right(), offset),
        place_on_axis(base.1, window_size.1, available.top(), available.bottom(), offset),
    )
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct VisiblePlacement {
    /// Adjusted window geometry, fully inside `area`.
    pub geometry: Rect,
    /// The visible area (screen) the window was fitted into.
    pub area: Rect,
}

/// Squared distance between two rectangles (0 if they touch or overlap).
fn distance_squared(a: &Rect, b: &Rect) -> i64 {
    let dx = i64::from(0.max(a.left() - b.right()).max(b.left() - a.right()));
    let dy = i64::from(0.max(a.top() - b.bottom()).max(b.top() - a.bottom()));
    dx * dx + dy * dy
}

fn overlap_area(a: &Rect, b: &Rect) -> i64 {
    let overlap = a.intersected(b);
    if overlap.is_empty() { 0 } else { i64::from(overlap.width) * i64::from(overlap.height) }
}

/// Makes `window` fully visible on one of `areas` (the usable screen areas, i.e.
/// without taskbars). Picks the area the window overlaps most, or, if it is
/// entirely off-screen, the nearest area. The window is moved by the smallest
/// distance needed (clamped per axis) and shrunk only if it is larger than the
/// area. A window that is already fully visible is returned unchanged.
/// With no areas, the window is returned unchanged (the C++ asserts instead).
pub fn fit_into_visible_area(window: &Rect, areas: &[Rect]) -> VisiblePlacement {
    // Best area: largest overlap first, then smallest distance (the first one on ties).
    let Some(area) = areas.iter().copied().reduce(|best, a| {
        let (overlap_best, overlap_a) = (overlap_area(window, &best), overlap_area(window, &a));
        let better = if overlap_a != overlap_best {
            overlap_a > overlap_best
        } else {
            distance_squared(window, &a) < distance_squared(window, &best)
        };
        if better { a } else { best }
    }) else {
        return VisiblePlacement { geometry: *window, area: *window };
    };

    let width = window.width.min(area.width).max(1);
    let height = window.height.min(area.height).max(1);
    let clamp = |v: i32, lo: i32, hi: i32| v.min(hi).max(lo); // std::clamp, without panicking if hi < lo
    let x = clamp(window.x, area.left(), area.right() - width + 1);
    let y = clamp(window.y, area.top(), area.bottom() - height + 1);
    VisiblePlacement { geometry: Rect::new(x, y, width, height), area }
}

#[cfg(test)]
mod tests {
    use super::*;

    const SCREEN: Rect = Rect::new(0, 0, 1920, 1040); // available area (taskbar excluded)
    const WINDOW: (i32, i32) = (200, 95);

    fn cascade(base: (i32, i32), available: &Rect, slot: i32) -> (i32, i32) {
        cascade_position(base, WINDOW, available, slot, 32)
    }

    #[test]
    fn first_slot_uses_base_position() {
        assert_eq!(cascade((250, 250), &SCREEN, 0), (250, 250));
    }

    #[test]
    fn cascades_down_and_right() {
        assert_eq!(cascade((250, 250), &SCREEN, 1), (282, 282));
        assert_eq!(cascade((250, 250), &SCREEN, 3), (346, 346));
        assert_eq!(cascade_position((250, 250), WINDOW, &SCREEN, 2, 10), (270, 270));
    }

    #[test]
    fn exactly_fitting_at_the_edge_is_allowed() {
        assert_eq!(cascade((1688, 100), &SCREEN, 1).0, 1720);
    }

    #[test]
    fn reverses_only_the_axis_that_would_spill_out() {
        assert_eq!(cascade((1700, 100), &SCREEN, 1), (1668, 132));
        assert_eq!(cascade((100, 930), &SCREEN, 1), (132, 898));
        assert_eq!(cascade((1700, 930), &SCREEN, 2), (1636, 866));
    }

    #[test]
    fn stays_visible_when_neither_direction_fits() {
        let tiny = Rect::new(0, 0, 260, 120);
        let p = cascade((30, 10), &tiny, 1);
        assert!(tiny.contains_rect(&Rect::new(p.0, p.1, WINDOW.0, WINDOW.1)));
    }

    #[test]
    fn respects_screen_origin() {
        let left = Rect::new(-1920, 0, 1920, 1040);
        assert_eq!(cascade((-300, 100), &left, 1), (-268, 132));
        assert_eq!(cascade((-230, 100), &left, 1), (-262, 132));
        assert_eq!(cascade((-1800, 100), &left, 1), (-1768, 132));
    }

    #[test]
    fn visible_window_is_unchanged() {
        let w = Rect::new(250, 250, WINDOW.0, WINDOW.1);
        let placed = fit_into_visible_area(&w, &[SCREEN]);
        assert_eq!(placed.geometry, w);
        assert_eq!(placed.area, SCREEN);
    }

    #[test]
    fn moves_by_shortest_distance() {
        let cases = [
            ((5000, 300), (1720, 300)),
            ((-5000, 300), (0, 300)),
            ((500, 1000), (500, 945)),
            ((500, -40), (500, 0)),
            ((1800, 300), (1720, 300)),
            ((3000, 3000), (1720, 945)),
            ((-300, -300), (0, 0)),
        ];
        for (saved, expected) in cases {
            let placed = fit_into_visible_area(&Rect::new(saved.0, saved.1, WINDOW.0, WINDOW.1), &[SCREEN]);
            assert_eq!((placed.geometry.x, placed.geometry.y), expected, "{saved:?}");
            assert_eq!((placed.geometry.width, placed.geometry.height), WINDOW);
            assert!(SCREEN.contains_rect(&placed.geometry));
        }
    }

    #[test]
    fn shrinks_window_larger_than_screen() {
        let small = Rect::new(0, 0, 800, 600);
        let placed = fit_into_visible_area(&Rect::new(100, 100, 1000, 300), &[small]);
        assert_eq!(placed.geometry, Rect::new(0, 100, 800, 300));
    }

    #[test]
    fn picks_screen_with_most_overlap() {
        let left = Rect::new(0, 0, 1920, 1040);
        let right = Rect::new(1920, 0, 1920, 1040);
        let placed = fit_into_visible_area(&Rect::new(1900, 100, 200, 95), &[left, right]);
        assert_eq!(placed.area, right);
        assert_eq!((placed.geometry.x, placed.geometry.y), (1920, 100));
    }

    #[test]
    fn picks_nearest_screen_when_off_screen() {
        let primary = Rect::new(0, 0, 1920, 1040);
        let secondary = Rect::new(1920, 200, 1280, 984);
        let placed = fit_into_visible_area(&Rect::new(4000, 500, 200, 95), &[primary, secondary]);
        assert_eq!(placed.area, secondary);
        assert_eq!((placed.geometry.x, placed.geometry.y), (3000, 500));
        let placed = fit_into_visible_area(&Rect::new(2500, 50, 200, 95), &[primary, secondary]);
        assert_eq!(placed.area, secondary);
        assert_eq!((placed.geometry.x, placed.geometry.y), (2500, 200));
        let placed = fit_into_visible_area(&Rect::new(-3000, 500, 200, 95), &[primary, secondary]);
        assert_eq!(placed.area, primary);
        assert_eq!((placed.geometry.x, placed.geometry.y), (0, 500));
    }

    #[test]
    fn rect_semantics() {
        let r = Rect::new(10, 20, 30, 40);
        assert_eq!((r.right(), r.bottom()), (39, 59));
        assert_eq!(r.intersected(&Rect::new(39, 59, 5, 5)), Rect::new(39, 59, 1, 1));
        assert!(r.intersected(&Rect::new(40, 20, 5, 5)).is_empty());
    }
}
