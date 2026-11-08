//! Animates the alarm icon like a ringing bell (port of bell-animation.cpp):
//! the colored background stays still while the white bell swings from the top
//! of its handle and the clapper follows a little later.
//!
//! The icon is split into layers once: white artwork (the bell and the clapper
//! dot below it) is cut out with soft edges, and the hole it leaves in the
//! background is filled from the surrounding pixels.

use image::{Rgba, RgbaImage};

use crate::image_layers::{
    fill_unknown, is_white_artwork, premultiplied, source_over, unpremultiplied, unpremultiply, white_matte,
};

#[derive(Debug, Clone)]
pub struct BellAnimation {
    background: RgbaImage, // straight alpha
    bell: RgbaImage,       // straight alpha: white with varying opacity
    clapper: RgbaImage,
    pivot: (f64, f64),
    valid: bool,
}

struct Grid {
    width: i32,
    height: i32,
    cells: Vec<i32>, // per pixel: 0 = outside, -1 = unlabelled foreground, otherwise a label
}

impl Grid {
    fn index(&self, x: i32, y: i32) -> usize {
        (y as usize) * (self.width as usize) + x as usize
    }
    fn at(&self, x: i32, y: i32) -> i32 {
        self.cells[self.index(x, y)]
    }
    fn set(&mut self, x: i32, y: i32, v: i32) {
        let i = self.index(x, y);
        self.cells[i] = v;
    }
    fn inside(&self, x: i32, y: i32) -> bool {
        x >= 0 && y >= 0 && x < self.width && y < self.height
    }
}

/// Labels 4-connected regions of -1 cells as 1, 2, ...; returns their pixel counts.
fn label_components(grid: &mut Grid) -> Vec<usize> {
    let mut sizes = Vec::new();
    let mut stack = Vec::new();
    let mut next = 0;
    for y in 0..grid.height {
        for x in 0..grid.width {
            if grid.at(x, y) != -1 {
                continue;
            }
            next += 1;
            let mut count = 0;
            stack.push((x, y));
            grid.set(x, y, next);
            while let Some((cx, cy)) = stack.pop() {
                count += 1;
                for (dx, dy) in [(1, 0), (-1, 0), (0, 1), (0, -1)] {
                    let (nx, ny) = (cx + dx, cy + dy);
                    if grid.inside(nx, ny) && grid.at(nx, ny) == -1 {
                        grid.set(nx, ny, next);
                        stack.push((nx, ny));
                    }
                }
            }
            sizes.push(count);
        }
    }
    sizes
}

impl BellAnimation {
    pub fn new(icon: &RgbaImage) -> Self {
        let mut result = Self {
            background: RgbaImage::new(0, 0),
            bell: RgbaImage::new(0, 0),
            clapper: RgbaImage::new(0, 0),
            pivot: (0.0, 0.0),
            valid: false,
        };
        let source = icon;
        let w = source.width() as i32;
        let h = source.height() as i32;
        if w < 8 || h < 8 {
            return result;
        }

        // 1. White artwork, grown by a small margin so the soft (antialiased) rim is included.
        let margin = (w / 64).max(1);
        let mut region = Grid { width: w, height: h, cells: vec![0; (w * h) as usize] };
        for y in 0..h {
            for x in 0..w {
                if !is_white_artwork(*source.get_pixel(x as u32, y as u32)) {
                    continue;
                }
                for dy in -margin..=margin {
                    for dx in -margin..=margin {
                        if region.inside(x + dx, y + dy) {
                            region.set(x + dx, y + dy, -1);
                        }
                    }
                }
            }
        }

        // 2. Bell = largest white region, clapper = second largest (if any).
        let sizes = label_components(&mut region);
        if sizes.is_empty() {
            return result;
        }
        let size_of = |label: i32| sizes[(label - 1) as usize];
        let mut order: Vec<i32> = (1..=sizes.len() as i32).collect();
        order.sort_by_key(|&label| std::cmp::Reverse(size_of(label))); // largest first
        let bell_label = order[0];
        let clapper_label = order.get(1).copied().unwrap_or(0);
        if size_of(bell_label) < ((w * h) / 100) as usize {
            return result; // too small to be the bell
        }
        // Only the bell and the clapper are cut out; any other white detail stays
        // part of the (static) background.
        for cell in &mut region.cells {
            if *cell != bell_label && *cell != clapper_label {
                *cell = 0;
            }
        }

        // 3. Background: fill the cut-out region from its surroundings, ring by ring.
        let mut background = source.clone();
        let mut known: Vec<bool> = region.cells.iter().map(|&c| c == 0).collect();
        fill_unknown(&mut background, &mut known);

        // 4. Bell and clapper layers: pure white, opacity = how much whiter than the
        //    reconstructed background the original pixel is (keeps the soft edges).
        let mut bell = RgbaImage::new(w as u32, h as u32);
        let mut clapper = bell.clone();
        let mut top = h;
        let mut sum_x = 0.0_f64;
        let mut count_x = 0;
        for y in 0..h {
            for x in 0..w {
                let label = region.at(x, y);
                if label == 0 {
                    continue;
                }
                let (ux, uy) = (x as u32, y as u32);
                let white = white_matte(*source.get_pixel(ux, uy), *background.get_pixel(ux, uy));
                let alpha = white[3];
                if label == bell_label {
                    bell.put_pixel(ux, uy, unpremultiply(white));
                    if alpha > 128 {
                        top = top.min(y);
                        sum_x += f64::from(x);
                        count_x += 1;
                    }
                } else if label == clapper_label {
                    clapper.put_pixel(ux, uy, unpremultiply(white));
                }
            }
        }
        result.background = background;
        result.bell = bell;
        result.clapper = clapper;
        if count_x == 0 {
            return result;
        }
        // The bell hangs from the top of its handle, horizontally at its centre.
        result.pivot = (sum_x / f64::from(count_x), f64::from(top));
        result.valid = true;
        result
    }

    /// False if the icon has no recognisable white bell (callers should then
    /// fall back to animating the whole icon).
    pub fn is_valid(&self) -> bool {
        self.valid
    }

    /// Replaces the background layer (e.g. with a recolored copy); must have the
    /// same size as the icon.
    pub fn set_background(&mut self, background: &RgbaImage) {
        if self.valid && background.dimensions() == self.background.dimensions() {
            self.background = background.clone();
        }
    }

    /// One frame: bell rotated by `bell_angle`, clapper by `clapper_angle`
    /// (degrees, clockwise, around the top of the bell's handle). `None` if the
    /// animation is not valid.
    pub fn frame(&self, bell_angle: f64, clapper_angle: f64) -> Option<RgbaImage> {
        if !self.valid {
            return None;
        }
        let mut result = premultiplied(&self.background);
        self.draw_rotated(&mut result, &self.clapper, clapper_angle);
        self.draw_rotated(&mut result, &self.bell, bell_angle);
        Some(unpremultiplied(&result))
    }

    /// Composites `layer` (straight alpha) rotated by `angle` degrees around the
    /// pivot onto the premultiplied `target`, with bilinear sampling.
    fn draw_rotated(&self, target: &mut RgbaImage, layer: &RgbaImage, angle: f64) {
        let layer = premultiplied(layer);
        if angle == 0.0 {
            // Identity transform: a plain blit, like QPainter.
            for (dst, src) in target.pixels_mut().zip(layer.pixels()) {
                *dst = source_over(*dst, *src);
            }
            return;
        }
        let (w, h) = (layer.width() as i64, layer.height() as i64);
        let fetch = |x: i64, y: i64| -> [f64; 4] {
            if x < 0 || y < 0 || x >= w || y >= h {
                [0.0; 4]
            } else {
                layer.get_pixel(x as u32, y as u32).0.map(f64::from)
            }
        };
        let (sin, cos) = angle.to_radians().sin_cos();
        let (px, py) = self.pivot;
        for y in 0..target.height() {
            for x in 0..target.width() {
                // Inverse-map the destination pixel centre into the layer.
                let dx = f64::from(x) + 0.5 - px;
                let dy = f64::from(y) + 0.5 - py;
                let sx = px + dx * cos + dy * sin - 0.5;
                let sy = py - dx * sin + dy * cos - 0.5;
                let (x0, y0) = (sx.floor(), sy.floor());
                let (fx, fy) = (sx - x0, sy - y0);
                let (x0, y0) = (x0 as i64, y0 as i64);
                if x0 < -1 || y0 < -1 || x0 >= w || y0 >= h {
                    continue;
                }
                let (c00, c10, c01, c11) = (fetch(x0, y0), fetch(x0 + 1, y0), fetch(x0, y0 + 1), fetch(x0 + 1, y0 + 1));
                let mut sample = [0u8; 4];
                for i in 0..4 {
                    let top = c00[i] * (1.0 - fx) + c10[i] * fx;
                    let bottom = c01[i] * (1.0 - fx) + c11[i] * fx;
                    sample[i] = (top * (1.0 - fy) + bottom * fy).round().clamp(0.0, 255.0) as u8;
                }
                if sample[3] == 0 {
                    continue;
                }
                let dst = target.get_pixel_mut(x, y);
                *dst = source_over(*dst, Rgba(sample));
            }
        }
    }

    /// The layers, for tests and debugging (straight alpha).
    pub fn background(&self) -> &RgbaImage {
        &self.background
    }

    pub fn bell(&self) -> &RgbaImage {
        &self.bell
    }

    pub fn clapper(&self) -> &RgbaImage {
        &self.clapper
    }

    pub fn pivot(&self) -> (f64, f64) {
        self.pivot
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::theme::{app_icon_image, theme_by_name, theme_names, themed_app_image, themed_bell_animation};

    struct Difference {
        mean: f64,
        worst: i32,
        where_: (u32, u32),
    }

    impl std::fmt::Display for Difference {
        fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
            write!(f, "mean {}, worst {} at {:?}", self.mean, self.worst, self.where_)
        }
    }

    /// Mean and maximum per-channel difference inside (x, y, w, h).
    fn difference(a: &RgbaImage, b: &RgbaImage, area: (u32, u32, u32, u32)) -> Difference {
        let (ax, ay, aw, ah) = area;
        let (mut sum, mut worst, mut where_, mut count) = (0.0, 0, (0, 0), 0);
        for y in ay..ay + ah {
            for x in ax..ax + aw {
                let (p, q) = (a.get_pixel(x, y).0, b.get_pixel(x, y).0);
                for i in 0..4 {
                    let d = (i32::from(p[i]) - i32::from(q[i])).abs();
                    sum += f64::from(d);
                    if d > worst {
                        worst = d;
                        where_ = (x, y);
                    }
                    count += 1;
                }
            }
        }
        Difference { mean: sum / f64::from(count), worst, where_ }
    }

    #[test]
    fn splits_the_original_icon() {
        let icon = app_icon_image();
        assert_eq!(icon.dimensions(), (128, 128));
        let bell = BellAnimation::new(&icon);
        assert!(bell.is_valid());

        let (px, py) = bell.pivot();
        assert!((px - 63.5).abs() < 4.0, "{px}");
        assert!((py - 20.0).abs() < 3.0, "{py}");

        let d = difference(&bell.frame(0.0, 0.0).unwrap(), &icon, (0, 0, 128, 128));
        assert!(d.mean < 1.0 && d.worst < 48, "{d}");
    }

    #[test]
    fn themed_icon_is_the_resting_frame_and_bell_stays_white() {
        for name in theme_names().into_iter().filter(|&n| n != "blue") {
            let theme = theme_by_name(name).unwrap();
            let bell = themed_bell_animation(&theme);
            assert!(bell.is_valid(), "{name}");
            let icon = themed_app_image(&theme);
            let d = difference(&bell.frame(0.0, 0.0).unwrap(), &icon, (0, 0, 128, 128));
            assert_eq!(d.worst, 0, "{name}: {d}");

            let mut checked = 0;
            for (x, y, p) in bell.bell().enumerate_pixels() {
                if p[3] == 255 {
                    let c = icon.get_pixel(x, y);
                    assert_eq!([c[0], c[1], c[2]], [255, 255, 255], "{name} at ({x},{y})");
                    checked += 1;
                }
            }
            assert!(checked > 1000, "{name}: {checked}");
        }
    }

    #[test]
    fn background_stays_still_while_the_bell_moves() {
        let bell = themed_bell_animation(&theme_by_name("hibiscus-red").unwrap());
        assert!(bell.is_valid());
        let rest = bell.frame(0.0, 0.0).unwrap();
        for swung in [bell.frame(12.0, 16.0).unwrap(), bell.frame(-12.0, -16.0).unwrap()] {
            for area in [(0, 0, 128, 10), (0, 118, 128, 10), (0, 0, 10, 128), (118, 0, 10, 128)] {
                let d = difference(&rest, &swung, area);
                assert_eq!(d.worst, 0, "{d}");
            }
            let moved = difference(&rest, &swung, (20, 15, 88, 95));
            assert!(moved.mean > 5.0 && moved.worst > 100, "{moved}");
        }
    }

    #[test]
    fn rejects_an_icon_without_a_bell() {
        let plain = RgbaImage::from_pixel(64, 64, Rgba([30, 90, 160, 255]));
        assert!(!BellAnimation::new(&plain).is_valid());
        assert!(BellAnimation::new(&plain).frame(10.0, 10.0).is_none());
    }
}
