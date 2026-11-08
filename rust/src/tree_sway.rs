//! Animates the tree picture of the "Time's up" window as if moved by a light
//! breeze (port of tree-sway.cpp): the frame and the ground stay still, the tree
//! bends (its base stays planted, the crown moves most) and the leaves shimmer.
//!
//! The picture is warped rather than cut apart: inside the frame and above the
//! ground, every pixel is taken from a slightly shifted position. The background
//! fill there is an even, translucent color, so shifting it is invisible and only
//! the tree visibly moves; with no wind the picture is reproduced exactly.

use std::f64::consts::PI;

use image::{Rgba, RgbaImage};

use crate::image_layers::{is_white_artwork, premultiplied, unpremultiply};

/// The bundled tree picture (src/Resources/images/tree02.png), drawn for the blue theme.
pub const TREE_PNG: &[u8] = include_bytes!("../../src/Resources/images/tree02.png");

/// The decoded tree picture (straight RGBA); recolor it with
/// [`crate::theme::recolor_for_theme`].
pub fn tree_image() -> RgbaImage {
    image::load_from_memory_with_format(TREE_PNG, image::ImageFormat::Png)
        .expect("bundled tree picture decodes")
        .to_rgba8()
}

#[derive(Debug, Clone)]
pub struct TreeSway {
    picture: RgbaImage,       // as given (unchanged pixels are copied from here)
    premultiplied: RgbaImage, // for sampling moved pixels
    ground_line: i32,         // first row of the ground (does not move)
    crown_top: i32,           // highest row of the tree
    band: i32,                // width of the frame band (left, top, right)
    corner: i32,              // size of the rounded top corners treated as frame
    valid: bool,
}

/// 0 below `edge0`, 1 above `edge1`, smooth in between.
fn smoothstep(edge0: f64, edge1: f64, value: f64) -> f64 {
    let t = ((value - edge0) / (edge1 - edge0)).clamp(0.0, 1.0);
    t * t * (3.0 - 2.0 * t)
}

impl TreeSway {
    /// Seconds for one full sway cycle.
    pub const BREEZE_PERIOD: f64 = 6.0;
    /// Pixels the crown top moves each way.
    pub const BREEZE_SWAY: f64 = 4.0;
    /// Pixels of leaf shimmer in the crown.
    pub const LEAF_FLUTTER: f64 = 0.8;
    /// Seconds (fits 5 times into BREEZE_PERIOD).
    pub const LEAF_FLUTTER_PERIOD: f64 = 1.2;

    pub fn new(picture: &RgbaImage) -> Self {
        let mut s = Self {
            picture: picture.clone(),
            premultiplied: premultiplied(picture),
            ground_line: 0,
            crown_top: 0,
            band: 0,
            corner: 0,
            valid: false,
        };
        let w = picture.width() as i32;
        let h = picture.height() as i32;
        if w < 16 || h < 16 {
            return s;
        }
        let white = |x: i32, y: i32| is_white_artwork(*picture.get_pixel(x as u32, y as u32));

        // 1. The frame: a white band along the left, top and right edges. Measure its
        //    thickness on the middle row/column (plus 2 px for its soft inner edge).
        let white_run = |mut x: i32, mut y: i32, dx: i32, dy: i32| {
            let mut run = 0;
            while x >= 0 && y >= 0 && x < w && y < h && white(x, y) {
                run += 1;
                x += dx;
                y += dy;
            }
            run
        };
        s.band = white_run(0, h / 2, 1, 0).max(white_run(w - 1, h / 2, -1, 0)).max(white_run(w / 2, 0, 0, 1)) + 2;
        // The top corners are rounded: their arcs bulge inwards past the straight band.
        s.corner = 3 * s.band;
        if s.band < 3 || 2 * s.corner >= w {
            return s; // no recognisable frame
        }

        // 2. Ground line: the first row below the frame's top band that is mostly white.
        s.ground_line = -1;
        let mut y = s.band;
        while y < h && s.ground_line < 0 {
            let count = (0..w).filter(|&x| white(x, y)).count() as i32;
            if count * 2 >= w {
                s.ground_line = y;
            }
            y += 1;
        }
        if s.ground_line <= s.band {
            return s;
        }

        // 3. Crown top: the highest white artwork inside the frame.
        s.crown_top = s.ground_line;
        'rows: for y in s.band..s.ground_line {
            for x in 0..w {
                if s.inside_frame(x, y) && white(x, y) {
                    s.crown_top = y;
                    break 'rows;
                }
            }
        }
        s.valid = s.crown_top < s.ground_line - 8; // a tree worth animating
        s
    }

    /// False if no frame, ground line or tree was found (callers then show the
    /// picture without animation).
    pub fn is_valid(&self) -> bool {
        self.valid
    }

    pub fn ground_line(&self) -> i32 {
        self.ground_line
    }

    pub fn crown_top(&self) -> i32 {
        self.crown_top
    }

    pub fn inside_frame(&self, x: i32, y: i32) -> bool {
        let w = self.picture.width() as i32;
        let in_top_corner = y < self.corner && (x < self.corner || x >= w - self.corner);
        x >= self.band && x < w - self.band && y >= self.band && !in_top_corner
    }

    /// One frame. `sway`: sideways movement of the crown top in pixels (negative =
    /// left); every row moves by sway * s^2, where s goes from 0 at the ground line
    /// to 1 at the crown top. `flutter` (pixels) and `flutter_phase` (radians): a
    /// small, smooth ripple in the crown only. `None` if not valid.
    pub fn frame(&self, sway: f64, flutter_phase: f64, flutter: f64) -> Option<RgbaImage> {
        if !self.valid {
            return None;
        }
        let w = self.picture.width() as i32;
        let height = f64::from(self.ground_line - self.crown_top);
        let mut result = self.picture.clone();
        let (band, ground) = (self.band, self.ground_line);

        // Samples the picture at (x, y) with bilinear filtering. Positions are kept inside
        // the frame and above the ground, so the frame's white never smears inwards.
        let sample = |x: f64, y: f64| -> Rgba<u8> {
            let x = x.clamp(f64::from(band), f64::from(w - band - 1));
            let y = y.clamp(f64::from(band), f64::from(ground - 1));
            let x0 = x.floor() as i32;
            let y0 = y.floor() as i32;
            let x1 = (x0 + 1).min(w - band - 1);
            let y1 = (y0 + 1).min(ground - 1);
            let fx = x - f64::from(x0);
            let fy = y - f64::from(y0);
            let px = |x: i32, y: i32| self.premultiplied.get_pixel(x as u32, y as u32).0;
            let (c00, c10, c01, c11) = (px(x0, y0), px(x1, y0), px(x0, y1), px(x1, y1));
            let mix = |i: usize| {
                let top = f64::from(c00[i]) * (1.0 - fx) + f64::from(c10[i]) * fx;
                let bottom = f64::from(c01[i]) * (1.0 - fx) + f64::from(c11[i]) * fx;
                (top * (1.0 - fy) + bottom * fy).round() as u8
            };
            unpremultiply(Rgba([mix(0), mix(1), mix(2), mix(3)]))
        };

        // Rows from just above the crown down to the ground move; the empty sky between
        // the frame's top band and the crown stays as it is.
        let first_row = band.max(self.crown_top - 3);
        for y in first_row..ground {
            // 0 at the ground, 1 at the crown top: the base stays put, the crown moves most.
            let s = (f64::from(ground - y) / height).clamp(0.0, 1.0);
            let bend = sway * s * s;
            let leaves = smoothstep(0.45, 0.75, s); // flutter in the crown only
            if bend == 0.0 && leaves * flutter == 0.0 {
                continue; // row unchanged
            }
            for x in 0..w {
                if !self.inside_frame(x, y) {
                    continue; // the frame never moves
                }
                let (xf, yf) = (f64::from(x), f64::from(y));
                let fx = flutter * leaves * (flutter_phase + 0.35 * xf + 0.23 * yf).sin();
                let fy = 0.5 * flutter * leaves * (flutter_phase + 0.29 * xf - 0.31 * yf).cos();
                result.put_pixel(x as u32, y as u32, sample(xf - bend - fx, yf - fy));
            }
        }
        Some(result)
    }

    /// Strength of the breeze at `seconds`, in [-1, 1]: a slow sway with a lighter,
    /// faster ripple on top. Repeats every BREEZE_PERIOD seconds.
    pub fn breeze(seconds: f64) -> f64 {
        // The ripple runs exactly 3 times per period, so the motion loops seamlessly.
        let phase = 2.0 * PI * seconds / Self::BREEZE_PERIOD;
        0.65 * phase.sin() + 0.35 * (3.0 * phase + 1.1).sin()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::theme::{recolor_for_theme, theme_by_name, theme_names};

    fn tree_picture(theme: &str) -> RgbaImage {
        recolor_for_theme(&tree_image(), &theme_by_name(theme).unwrap())
    }

    fn worst_difference(a: &RgbaImage, b: &RgbaImage, area: (u32, u32, u32, u32)) -> i32 {
        let (ax, ay, aw, ah) = area;
        let mut worst = 0;
        for y in ay..ay + ah {
            for x in ax..ax + aw {
                let (p, q) = (a.get_pixel(x, y).0, b.get_pixel(x, y).0);
                for i in 0..4 {
                    worst = worst.max((i32::from(p[i]) - i32::from(q[i])).abs());
                }
            }
        }
        worst
    }

    fn mean_difference(a: &RgbaImage, b: &RgbaImage) -> f64 {
        let sum: f64 = a
            .pixels()
            .zip(b.pixels())
            .map(|(p, q)| (0..4).map(|i| (f64::from(p[i]) - f64::from(q[i])).abs()).sum::<f64>())
            .sum();
        sum / (4.0 * f64::from(a.width()) * f64::from(a.height()))
    }

    /// Horizontal centre of the white tree pixels in rows [top, bottom), inside the frame.
    fn ink_centre(image: &RgbaImage, top: i32, bottom: i32) -> f64 {
        let (mut sum, mut weight) = (0.0, 0.0);
        for y in top..bottom {
            for x in 12..image.width() - 12 {
                let c = image.get_pixel(x, y as u32);
                let white = f64::from(c[0].min(c[1]).min(c[2])) / 255.0;
                if white > 0.8 {
                    sum += f64::from(x) * white;
                    weight += white;
                }
            }
        }
        if weight > 0.0 { sum / weight } else { 0.0 }
    }

    #[test]
    fn splits_the_picture() {
        for theme in theme_names() {
            let picture = tree_picture(theme);
            assert_eq!(picture.dimensions(), (157, 173));
            let sway = TreeSway::new(&picture);
            assert!(sway.is_valid(), "{theme}");
            assert!((112..=125).contains(&sway.ground_line()), "{theme}: {}", sway.ground_line());
            assert!((15..=35).contains(&sway.crown_top()), "{theme}: {}", sway.crown_top());
            assert_eq!(mean_difference(&sway.frame(0.0, 0.0, 0.0).unwrap(), &picture), 0.0);
            assert_eq!(mean_difference(&sway.frame(0.0, 2.0, 0.0).unwrap(), &picture), 0.0);
        }
    }

    #[test]
    fn frame_and_ground_stay_still() {
        let sway = TreeSway::new(&tree_picture("blue"));
        assert!(sway.is_valid());
        let rest = sway.frame(0.0, 0.0, 0.0).unwrap();
        for s in [-TreeSway::BREEZE_SWAY, TreeSway::BREEZE_SWAY] {
            for phase in [0.0, 1.7, 3.9] {
                let moved = sway.frame(s, phase, TreeSway::LEAF_FLUTTER).unwrap();
                for (x, y, p) in rest.enumerate_pixels() {
                    let (x, y) = (x as i32, y as i32);
                    let still = y >= sway.ground_line() || !sway.inside_frame(x, y) || y < sway.crown_top() - 3;
                    if still {
                        assert_eq!(p, moved.get_pixel(x as u32, y as u32), "pixel ({x},{y}) moved");
                    }
                }
            }
        }
    }

    #[test]
    fn crown_moves_base_stays_planted() {
        let sway = TreeSway::new(&tree_picture("blue"));
        let left = sway.frame(-TreeSway::BREEZE_SWAY, 0.0, 0.0).unwrap();
        let right = sway.frame(TreeSway::BREEZE_SWAY, 0.0, 0.0).unwrap();
        let (crown, ground) = (sway.crown_top(), sway.ground_line());
        let crown_shift = ink_centre(&right, crown, crown + 20) - ink_centre(&left, crown, crown + 20);
        let full_swing = 2.0 * TreeSway::BREEZE_SWAY;
        assert!(crown_shift > 0.7 * full_swing && crown_shift < 1.1 * full_swing, "{crown_shift}");
        let base_shift = ink_centre(&right, ground - 12, ground) - ink_centre(&left, ground - 12, ground);
        assert!(base_shift.abs() < 0.6, "{base_shift}");
    }

    #[test]
    fn flutter_moves_only_the_crown() {
        let sway = TreeSway::new(&tree_picture("blue"));
        let calm = sway.frame(0.0, 0.0, 0.0).unwrap();
        let shimmer = sway.frame(0.0, 1.3, TreeSway::LEAF_FLUTTER).unwrap();
        let height = sway.ground_line() - sway.crown_top();
        let trunk_height = height * 2 / 5;
        let trunk = (0, (sway.ground_line() - trunk_height) as u32, 157, trunk_height as u32);
        assert_eq!(worst_difference(&calm, &shimmer, trunk), 0);
        let crown = (0, sway.crown_top() as u32, 157, (height / 3) as u32);
        assert!(worst_difference(&calm, &shimmer, crown) > 20);
    }

    #[test]
    fn breeze_is_gentle_and_loops() {
        let (mut low, mut high) = (0.0_f64, 0.0_f64);
        for i in 0..600 {
            let t = f64::from(i) * 0.01;
            let b = TreeSway::breeze(t);
            low = low.min(b);
            high = high.max(b);
            assert!((TreeSway::breeze(t + TreeSway::BREEZE_PERIOD) - b).abs() < 1e-9);
        }
        assert!(low >= -1.0 && high <= 1.0);
        assert!(low < -0.5 && high > 0.5);
    }

    #[test]
    fn rejects_a_picture_without_a_tree() {
        let plain = RgbaImage::from_pixel(64, 64, Rgba([30, 90, 160, 255]));
        assert!(!TreeSway::new(&plain).is_valid());
        assert!(TreeSway::new(&plain).frame(2.0, 0.0, 0.0).is_none());
    }
}
