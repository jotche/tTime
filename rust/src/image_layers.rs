//! Helpers for splitting white artwork out of tTime's pictures into layers
//! (port of image-layers.cpp; used by BellAnimation, TreeSway uses the
//! white-artwork test), plus Qt-compatible premultiplication.
//!
//! Pixels are `image::Rgba<u8>` with straight (non-premultiplied) alpha unless
//! a function says otherwise.

use image::{Rgba, RgbaImage};

/// "Whiteness" of a pixel: its darkest channel (white is high in every channel,
/// a colored background is low in at least one).
pub fn whiteness(color: Rgba<u8>) -> i32 {
    let [r, g, b, _] = color.0;
    i32::from(r.min(g).min(b))
}

/// Clearly white artwork: opaque, bright and nearly grey. (Opacity matters: the
/// transparent corners of a rounded icon often carry white color values.)
pub fn is_white_artwork(color: Rgba<u8>) -> bool {
    let [r, g, b, a] = color.0;
    let max = i32::from(r.max(g).max(b));
    a >= 128 && whiteness(color) >= 200 && max - whiteness(color) <= 40
}

/// Fills every pixel of `image` whose entry in `known` (indexed y * width + x)
/// is false from its known neighbours, ring by ring, and marks it known. Used to
/// rebuild the background behind artwork that has been cut out.
pub fn fill_unknown(image: &mut RgbaImage, known: &mut [bool]) {
    let w = image.width() as i32;
    let h = image.height() as i32;
    let index = |x: i32, y: i32| (y as usize) * (w as usize) + x as usize;

    loop {
        let mut filled: Vec<(u32, u32, Rgba<u8>)> = Vec::new();
        for y in 0..h {
            for x in 0..w {
                if known[index(x, y)] {
                    continue;
                }
                let (mut r, mut g, mut b, mut a, mut n) = (0u32, 0u32, 0u32, 0u32, 0u32);
                for dy in -1..=1 {
                    for dx in -1..=1 {
                        let (nx, ny) = (x + dx, y + dy);
                        if nx >= 0 && ny >= 0 && nx < w && ny < h && known[index(nx, ny)] {
                            let c = image.get_pixel(nx as u32, ny as u32).0;
                            r += u32::from(c[0]);
                            g += u32::from(c[1]);
                            b += u32::from(c[2]);
                            a += u32::from(c[3]);
                            n += 1;
                        }
                    }
                }
                if n > 0 {
                    let avg = |v: u32| (v / n) as u8;
                    filled.push((x as u32, y as u32, Rgba([avg(r), avg(g), avg(b), avg(a)])));
                }
            }
        }
        if filled.is_empty() {
            break;
        }
        for (x, y, color) in filled {
            image.put_pixel(x, y, color);
            known[index(x as i32, y as i32)] = true;
        }
    }
}

/// The artwork pixel as pure white (premultiplied, so all four channels equal
/// the alpha), with an opacity of how much whiter `original` is than the
/// rebuilt `background`; keeps soft edges.
pub fn white_matte(original: Rgba<u8>, background: Rgba<u8>) -> Rgba<u8> {
    let bg = whiteness(background);
    let fg = whiteness(original);
    let alpha = if bg >= 255 { 0 } else { ((fg - bg) * 255 / (255 - bg)).clamp(0, 255) };
    premultiply(Rgba([255, 255, 255, (alpha * i32::from(original[3]) / 255) as u8]))
}

/// Qt's BYTE_MUL for one channel: `x * a / 255`, rounded.
#[inline]
pub(crate) fn byte_mul(x: u32, a: u32) -> u32 {
    let t = x * a;
    (t + (t >> 8) + 0x80) >> 8
}

/// `qPremultiply`.
pub fn premultiply(c: Rgba<u8>) -> Rgba<u8> {
    let [r, g, b, a] = c.0;
    let a32 = u32::from(a);
    let m = |v: u8| byte_mul(u32::from(v), a32) as u8;
    Rgba([m(r), m(g), m(b), a])
}

/// `qUnpremultiply`.
pub fn unpremultiply(c: Rgba<u8>) -> Rgba<u8> {
    let [r, g, b, a] = c.0;
    match a {
        255 => c,
        0 => Rgba([0, 0, 0, 0]),
        _ => {
            let inv = 0x00ff_00ff / u32::from(a);
            let u = |v: u8| ((u32::from(v) * inv + 0x8000) >> 16).min(255) as u8;
            Rgba([u(r), u(g), u(b), a])
        }
    }
}

/// Source-over of premultiplied `src` onto premultiplied `dst` (Qt's comp_func_SourceOver).
pub fn source_over(dst: Rgba<u8>, src: Rgba<u8>) -> Rgba<u8> {
    let sa = src[3];
    if sa == 255 {
        return src;
    }
    if src.0 == [0, 0, 0, 0] {
        return dst;
    }
    let inv = 255 - u32::from(sa);
    let ch = |i: usize| (u32::from(src[i]) + byte_mul(u32::from(dst[i]), inv)).min(255) as u8;
    Rgba([ch(0), ch(1), ch(2), ch(3)])
}

/// The whole image premultiplied (`convertToFormat(Format_ARGB32_Premultiplied)`).
pub fn premultiplied(image: &RgbaImage) -> RgbaImage {
    let mut out = image.clone();
    out.pixels_mut().for_each(|p| *p = premultiply(*p));
    out
}

/// The whole image unpremultiplied (`convertToFormat(Format_ARGB32)`).
pub fn unpremultiplied(image: &RgbaImage) -> RgbaImage {
    let mut out = image.clone();
    out.pixels_mut().for_each(|p| *p = unpremultiply(*p));
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn white_artwork() {
        assert!(is_white_artwork(Rgba([255, 255, 255, 255])));
        assert!(is_white_artwork(Rgba([210, 220, 240, 128])));
        assert!(!is_white_artwork(Rgba([255, 255, 255, 127]))); // transparent corner
        assert!(!is_white_artwork(Rgba([190, 255, 255, 255]))); // not bright enough
        assert!(!is_white_artwork(Rgba([200, 255, 255, 255]))); // too colorful
        assert_eq!(whiteness(Rgba([9, 3, 200, 0])), 3);
    }

    #[test]
    fn premultiplication_round_trips() {
        for a in 0..=255u8 {
            for v in [0u8, 1, 77, 128, 200, 255] {
                let p = premultiply(Rgba([v, v, v, a]));
                assert_eq!(premultiply(unpremultiply(p)), p);
            }
        }
        assert_eq!(premultiply(Rgba([255, 255, 255, 128])), Rgba([128, 128, 128, 128]));
    }

    #[test]
    fn white_matte_opacity() {
        let bg = Rgba([40, 120, 200, 255]);
        assert_eq!(white_matte(Rgba([255, 255, 255, 255]), bg), Rgba([255, 255, 255, 255]));
        assert_eq!(white_matte(bg, bg), Rgba([0, 0, 0, 0]));
        assert_eq!(white_matte(Rgba([255, 255, 255, 255]), Rgba([255, 255, 255, 255])), Rgba([0, 0, 0, 0]));
    }

    #[test]
    fn fills_unknown_from_neighbours() {
        let mut image = RgbaImage::from_pixel(5, 1, Rgba([10, 20, 30, 255]));
        image.put_pixel(2, 0, Rgba([255, 255, 255, 255]));
        image.put_pixel(4, 0, Rgba([50, 60, 70, 255]));
        let mut known = vec![true, true, false, false, true];
        fill_unknown(&mut image, &mut known);
        assert!(known.iter().all(|&k| k));
        assert_eq!(image.get_pixel(2, 0), &Rgba([10, 20, 30, 255]));
        assert_eq!(image.get_pixel(3, 0), &Rgba([50, 60, 70, 255]));
    }
}
