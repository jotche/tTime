//! Themes of the tTime windows and icon (port of theme.cpp, without the panel
//! painting and the tool-button style sheet, which belong to the GUI).
//!
//! ```ini
//! [appearance]
//! theme=blue   ; see theme_names()
//! ```

use std::collections::HashMap;
use std::sync::{LazyLock, Mutex};

use image::{Rgba, RgbaImage};

use crate::bell_animation::BellAnimation;
use crate::settings::AppSettings;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Rgb(pub u8, pub u8, pub u8);

#[derive(Debug, Clone, PartialEq)]
pub struct Theme {
    pub name: &'static str,
    /// Window edges.
    pub dark: Rgb,
    /// Inner band.
    pub mid: Rgb,
    /// Centre.
    pub light: Rgb,
    /// Hovered / pressed / checked button background.
    pub highlight: Rgb,
    /// ... and its border.
    pub highlight_border: Rgb,
}

pub const DEFAULT_THEME_NAME: &str = "blue";
const THEME_KEY: &str = "appearance/theme";

/// The bundled alarm icon (src/Resources/icons/alarm.ico, 128x128).
pub const ALARM_ICO: &[u8] = include_bytes!("../../src/Resources/icons/alarm.ico");

const fn theme(name: &'static str, colors: [(u8, u8, u8); 5]) -> Theme {
    const fn c(rgb: (u8, u8, u8)) -> Rgb {
        Rgb(rgb.0, rgb.1, rgb.2)
    }
    Theme {
        name,
        dark: c(colors[0]),
        mid: c(colors[1]),
        light: c(colors[2]),
        highlight: c(colors[3]),
        highlight_border: c(colors[4]),
    }
}

// All themes, default first, with the accepted short name. Names are matched
// ignoring case, "-", "_" and spaces.
const THEMES: &[(Theme, &str)] = &[
    // The original tTime blue.
    (theme("blue", [(3, 66, 124), (69, 134, 186), (164, 212, 238), (141, 178, 206), (186, 209, 225)]), "blue"),
    // Sage green with the same light/dark structure as the blue.
    (theme("sage", [(44, 78, 56), (112, 146, 114), (196, 216, 188), (150, 176, 142), (200, 216, 192)]), "sagegreen"),
    // Prussian blue: deeper and greyer than the default blue.
    (
        theme("prussian-blue", [(0, 40, 68), (24, 78, 112), (140, 170, 190), (110, 145, 170), (170, 195, 212)]),
        "prussian",
    ),
    // Hibiscus red: a deep pink-red.
    (
        theme("hibiscus-red", [(112, 18, 48), (182, 49, 96), (238, 172, 190), (205, 120, 145), (240, 190, 205)]),
        "hibiscus",
    ),
    // Jade green (#00A86B): a bright, slightly bluish green.
    (theme("jade-green", [(0, 72, 46), (24, 150, 104), (160, 226, 200), (110, 190, 160), (180, 230, 210)]), "jade"),
    // Forest green (#228B22): a classic, fairly saturated green.
    (theme("forest-green", [(16, 64, 16), (40, 128, 40), (170, 215, 165), (120, 175, 115), (190, 222, 185)]), "forest"),
    // Pine green (#01796F): a deep teal green.
    (theme("pine-green", [(0, 58, 54), (16, 118, 108), (150, 208, 200), (105, 165, 158), (175, 215, 210)]), "pine"),
    // Hunter green (#355E3B): a dark, muted green.
    (theme("hunter-green", [(30, 56, 34), (64, 104, 70), (176, 200, 172), (130, 160, 128), (192, 212, 190)]), "hunter"),
];

fn normalized_theme_key(name: &str) -> String {
    name.trim().to_lowercase().chars().filter(|c| !matches!(c, '-' | ' ' | '_')).collect()
}

/// The default theme ("blue").
pub fn default_theme() -> Theme {
    THEMES[0].0.clone()
}

/// The available themes, default first.
pub fn theme_names() -> Vec<&'static str> {
    THEMES.iter().map(|(t, _)| t.name).collect()
}

/// Looks up a theme by name (case-insensitive, ignoring "-", "_" and spaces).
/// Short forms are accepted too: "sage-green", "prussian", "hibiscus", "jade",
/// "forest", "pine", "hunter".
pub fn theme_by_name(name: &str) -> Option<Theme> {
    let key = normalized_theme_key(name);
    THEMES
        .iter()
        .find(|(t, short)| key == normalized_theme_key(t.name) || key == normalized_theme_key(short))
        .map(|(t, _)| t.clone())
}

/// The theme configured in `settings` (appearance/theme); unknown names log a
/// warning and use the default.
pub fn theme_from_settings(settings: &AppSettings) -> Theme {
    let name = settings.value(THEME_KEY).unwrap_or_else(|| DEFAULT_THEME_NAME.to_owned());
    if let Some(theme) = theme_by_name(&name) {
        return theme;
    }
    log::warn!(
        "tTime.ini: unknown theme '{}', using '{}' (choices: {})",
        name,
        DEFAULT_THEME_NAME,
        theme_names().join(", ")
    );
    default_theme()
}

// ---------------------------------------------------------------------------
// QColor-compatible HSL (Qt 6: float math on 16-bit channels)

const USHRT_MAX: f32 = 65535.0;

fn q_round(d: f32) -> i32 {
    if d >= 0.0 { (d + 0.5) as i32 } else { (d - 0.5) as i32 }
}

fn qt_div_257(x: i32) -> i32 {
    (x - (x >> 8) + 0x80) >> 8
}

/// A color in QColor's HSL representation: hue in hundredths of a degree
/// (`None` = achromatic), saturation and lightness as 16-bit values.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct QtHsl {
    hue: Option<i32>,
    saturation: i32,
    lightness: i32,
}

impl QtHsl {
    /// `QColor(r, g, b).toHsl()`.
    pub fn from_rgb(r: u8, g: u8, b: u8) -> Self {
        let r = f32::from(r) * 257.0 / USHRT_MAX;
        let g = f32::from(g) * 257.0 / USHRT_MAX;
        let b = f32::from(b) * 257.0 / USHRT_MAX;
        let max = r.max(g).max(b);
        let min = r.min(g).min(b);
        let delta = max - min;
        let delta2 = max + min;
        let lightness = 0.5 * delta2;
        let lightness16 = q_round(lightness * USHRT_MAX);
        if delta.abs() <= 0.00001 {
            // achromatic case, hue is undefined
            return Self { hue: None, saturation: 0, lightness: lightness16 };
        }
        let saturation = if lightness < 0.5 { delta / delta2 } else { delta / (2.0 - delta2) };
        let mut hue = if r == max {
            (g - b) / delta
        } else if g == max {
            2.0 + (b - r) / delta
        } else {
            4.0 + (r - g) / delta
        };
        hue *= 60.0;
        if hue < 0.0 {
            hue += 360.0;
        }
        Self { hue: Some(q_round(hue * 100.0)), saturation: q_round(saturation * USHRT_MAX), lightness: lightness16 }
    }

    /// `hslHue()`: 0-359, or -1 for achromatic colors.
    pub fn hue(&self) -> i32 {
        self.hue.map_or(-1, |h| h / 100)
    }

    /// `hslSaturation()`: 0-255.
    pub fn saturation(&self) -> i32 {
        qt_div_257(self.saturation)
    }

    /// `lightness()`: 0-255.
    pub fn lightness(&self) -> i32 {
        qt_div_257(self.lightness)
    }
}

/// QColor's `hslHue()` of an RGB color (0-359, -1 for achromatic).
pub fn hsl_hue(c: Rgb) -> i32 {
    QtHsl::from_rgb(c.0, c.1, c.2).hue()
}

/// QColor's `hslSaturation()` (0-255).
pub fn hsl_saturation(c: Rgb) -> i32 {
    QtHsl::from_rgb(c.0, c.1, c.2).saturation()
}

/// QColor's `lightness()` (0-255).
pub fn lightness(c: Rgb) -> i32 {
    QtHsl::from_rgb(c.0, c.1, c.2).lightness()
}

/// `QColor::fromHsl(h, s, l, a).rgba()`. Out-of-range input gives an invalid
/// QColor in Qt, whose rgba() is transparent black; the same here.
pub fn from_hsl(h: i32, s: i32, l: i32, a: i32) -> Rgba<u8> {
    if (!(0..360).contains(&h) && h != -1)
        || !(0..=255).contains(&s)
        || !(0..=255).contains(&l)
        || !(0..=255).contains(&a)
    {
        return Rgba([0, 0, 0, 0]);
    }
    let hue16 = if h == -1 { None } else { Some((h % 360) * 100) };
    let s16 = s * 0x101;
    let l16 = l * 0x101;
    let channels: [i32; 3] = match hue16 {
        _ if s16 == 0 => [l16; 3],
        None => [l16; 3],
        Some(_) if l16 == 0 => [0; 3],
        Some(hue) => {
            let h = if hue == 36000 { 0.0 } else { hue as f32 / 36000.0 };
            let s = s16 as f32 / USHRT_MAX;
            let l = l16 as f32 / USHRT_MAX;
            let temp2 = if l < 0.5 { l * (1.0 + s) } else { l + s - (l * s) };
            let temp1 = (2.0 * l) - temp2;
            let mut temp3 = [h + (1.0 / 3.0), h, h - (1.0 / 3.0)];
            let mut out = [0i32; 3];
            for i in 0..3 {
                if temp3[i] < 0.0 {
                    temp3[i] += 1.0;
                } else if temp3[i] > 1.0 {
                    temp3[i] -= 1.0;
                }
                let sixtemp3 = temp3[i] * 6.0;
                out[i] = if sixtemp3 < 1.0 {
                    q_round((temp1 + (temp2 - temp1) * sixtemp3) * USHRT_MAX)
                } else if temp3[i] * 2.0 < 1.0 {
                    q_round(temp2 * USHRT_MAX)
                } else if temp3[i] * 3.0 < 2.0 {
                    q_round((temp1 + (temp2 - temp1) * (2.0 / 3.0 - temp3[i]) * 6.0) * USHRT_MAX)
                } else {
                    q_round(temp1 * USHRT_MAX)
                };
                if out[i] == 1 {
                    out[i] = 0;
                }
            }
            out
        }
    };
    let c = |v: i32| qt_div_257(v).clamp(0, 255) as u8;
    Rgba([c(channels[0]), c(channels[1]), c(channels[2]), c(a * 0x101)])
}

// ---------------------------------------------------------------------------
// Recoloring

/// Recolors an artwork made for the blue theme: dark, semi-transparent pixels
/// (tinted fills) take the theme's hue and saturation but keep their lightness
/// and opacity; light pixels (white artwork) are untouched. Returns `image`
/// unchanged for the blue theme.
pub fn recolor_for_theme(image: &RgbaImage, theme: &Theme) -> RgbaImage {
    let mut result = image.clone();
    if theme.name == "blue" {
        return result;
    }
    let hue = hsl_hue(theme.dark);
    let saturation = hsl_saturation(theme.dark);
    for p in result.pixels_mut() {
        let [r, g, b, a] = p.0;
        let l = QtHsl::from_rgb(r, g, b).lightness();
        if a == 0 || a >= 200 || l >= 128 {
            continue; // transparent, solid artwork or light (white) strokes
        }
        *p = from_hsl(hue, saturation, l, i32::from(a));
    }
    result
}

/// Recolors the app icon (drawn in the default blue): colored pixels take the
/// theme's hue, with saturation and lightness scaled like the theme's mid color
/// relative to the blue one; white/grey artwork is untouched. Returns `image`
/// unchanged for the blue theme.
pub fn recolor_icon_for_theme(image: &RgbaImage, theme: &Theme) -> RgbaImage {
    let mut result = image.clone();
    if theme.name == "blue" {
        return result;
    }
    // Map the blue reference (the default theme's mid color) onto the theme's
    // mid color: same hue, saturation and lightness scaled proportionally.
    let reference = default_theme().mid;
    let hue = hsl_hue(theme.mid);
    let saturation_scale = f64::from(hsl_saturation(theme.mid)) / f64::from(hsl_saturation(reference).max(1));
    let lightness_scale = f64::from(lightness(theme.mid)) / f64::from(lightness(reference).max(1));
    for p in result.pixels_mut() {
        let [r, g, b, a] = p.0;
        let hsl = QtHsl::from_rgb(r, g, b);
        if a == 0 || hsl.saturation() < 40 {
            continue; // transparent, or white/grey artwork (the bell)
        }
        let s = ((f64::from(hsl.saturation()) * saturation_scale).round() as i32).clamp(0, 255);
        let l = ((f64::from(hsl.lightness()) * lightness_scale).round() as i32).clamp(0, 255);
        *p = from_hsl(hue, s, l, i32::from(a));
    }
    result
}

static APP_ICON: LazyLock<RgbaImage> = LazyLock::new(|| {
    image::load_from_memory_with_format(ALARM_ICO, image::ImageFormat::Ico)
        .expect("bundled alarm icon decodes")
        .to_rgba8()
});

/// The decoded alarm icon (128x128, blue theme).
pub fn app_icon_image() -> RgbaImage {
    APP_ICON.clone()
}

/// The alarm icon split into background, bell and clapper, with the background
/// recolored for this theme and the bell kept pure white.
pub fn themed_bell_animation(theme: &Theme) -> BellAnimation {
    let mut animation = BellAnimation::new(&APP_ICON);
    if animation.is_valid() {
        let background = recolor_icon_for_theme(animation.background(), theme);
        animation.set_background(&background);
    }
    animation
}

/// The alarm icon in this theme (window, taskbar and tray icon). Cached per theme.
pub fn themed_app_image(theme: &Theme) -> RgbaImage {
    static CACHE: LazyLock<Mutex<HashMap<&'static str, RgbaImage>>> = LazyLock::new(Default::default);
    if let Some(image) = CACHE.lock().unwrap_or_else(|e| e.into_inner()).get(theme.name) {
        return image.clone();
    }
    let themed = if theme.name == "blue" {
        app_icon_image()
    } else {
        // Recolor only the background and put the white bell back on top, so the
        // bell keeps clean white edges in every theme. Fall back to recoloring the
        // whole icon if it has no separable bell.
        themed_bell_animation(theme).frame(0.0, 0.0).unwrap_or_else(|| recolor_icon_for_theme(&APP_ICON, theme))
    };
    CACHE.lock().unwrap_or_else(|e| e.into_inner()).insert(theme.name, themed.clone());
    themed
}

#[cfg(test)]
mod tests {
    use super::*;

    fn hsl_of(p: Rgba<u8>) -> QtHsl {
        QtHsl::from_rgb(p[0], p[1], p[2])
    }

    #[test]
    fn blue_is_the_original_palette() {
        let blue = theme_by_name("blue").unwrap();
        assert_eq!(blue.dark, Rgb(3, 66, 124));
        assert_eq!(blue.mid, Rgb(69, 134, 186));
        assert_eq!(blue.light, Rgb(164, 212, 238));
        assert_eq!(blue.highlight, Rgb(141, 178, 206));
        assert_eq!(blue.highlight_border, Rgb(186, 209, 225));
        assert_eq!(default_theme(), blue);
    }

    #[test]
    fn names_are_forgiving() {
        for name in ["sage", "Sage", " sage ", "sage-green", "Sage Green", "sage_green"] {
            assert_eq!(theme_by_name(name).map(|t| t.name), Some("sage"), "{name}");
        }
        assert!(theme_by_name("purple").is_none());
        assert!(theme_by_name("").is_none());
    }

    #[test]
    fn every_listed_theme_exists() {
        assert_eq!(theme_names()[0], DEFAULT_THEME_NAME);
        for name in theme_names() {
            assert!(theme_by_name(name).is_some(), "{name}");
        }
    }

    #[test]
    fn themes_keep_white_text_readable() {
        for name in theme_names() {
            let Rgb(r, g, b) = theme_by_name(name).unwrap().dark;
            let luminance =
                0.2126 * f64::from(r) / 255.0 + 0.7152 * f64::from(g) / 255.0 + 0.0722 * f64::from(b) / 255.0;
            assert!(luminance < 0.35, "{name}");
        }
    }

    #[test]
    fn recolour_leaves_blue_and_white_alone() {
        let mut image = RgbaImage::new(3, 1);
        image.put_pixel(0, 0, Rgba([5, 14, 52, 89])); // translucent navy fill
        image.put_pixel(1, 0, Rgba([255, 255, 255, 255])); // white artwork
        image.put_pixel(2, 0, Rgba([230, 230, 240, 120])); // light, antialiased edge

        assert_eq!(recolor_for_theme(&image, &theme_by_name("blue").unwrap()), image);

        let sage = recolor_for_theme(&image, &theme_by_name("sage").unwrap());
        let fill = *sage.get_pixel(0, 0);
        assert_eq!(fill[3], 89);
        assert!((hsl_of(fill).lightness() - lightness(Rgb(5, 14, 52))).abs() <= 1);
        assert!(fill[1] > fill[2]);
        assert_eq!(sage.get_pixel(1, 0), image.get_pixel(1, 0));
        assert_eq!(sage.get_pixel(2, 0), image.get_pixel(2, 0));
    }

    #[test]
    fn new_themes_and_aliases() {
        assert_eq!(theme_by_name("prussian-blue").unwrap().name, "prussian-blue");
        assert_eq!(theme_by_name("Prussian Blue").unwrap().name, "prussian-blue");
        assert_eq!(theme_by_name("prussian").unwrap().name, "prussian-blue");
        assert_eq!(theme_by_name("hibiscus-red").unwrap().name, "hibiscus-red");
        assert_eq!(theme_by_name("hibiscus").unwrap().name, "hibiscus-red");
        assert_eq!(
            theme_names(),
            [
                "blue",
                "sage",
                "prussian-blue",
                "hibiscus-red",
                "jade-green",
                "forest-green",
                "pine-green",
                "hunter-green"
            ]
        );
        for (alias, name) in [
            ("jade", "jade-green"),
            ("Forest Green", "forest-green"),
            ("pine_green", "pine-green"),
            ("HUNTER", "hunter-green"),
        ] {
            assert_eq!(theme_by_name(alias).unwrap().name, name);
        }
    }

    #[test]
    fn icon_recolor() {
        let cases = [
            ("sage", 100, 150),
            ("prussian-blue", 190, 215),
            ("hibiscus-red", 320, 359),
            ("jade-green", 145, 170),
            ("forest-green", 110, 130),
            ("pine-green", 165, 185),
            ("hunter-green", 115, 140),
        ];
        let mut icon = RgbaImage::new(3, 1);
        icon.put_pixel(0, 0, Rgba([40, 120, 200, 255])); // the icon's blue body
        icon.put_pixel(1, 0, Rgba([255, 255, 255, 255])); // white bell
        icon.put_pixel(2, 0, Rgba([0, 0, 0, 0])); // transparent
        assert_eq!(recolor_icon_for_theme(&icon, &theme_by_name("blue").unwrap()), icon);
        for (theme, min_hue, max_hue) in cases {
            let recolored = recolor_icon_for_theme(&icon, &theme_by_name(theme).unwrap());
            let body = *recolored.get_pixel(0, 0);
            let hue = hsl_of(body).hue();
            assert!((min_hue..=max_hue).contains(&hue), "{theme}: {hue}");
            assert_eq!(body[3], 255);
            assert_eq!(recolored.get_pixel(1, 0), icon.get_pixel(1, 0));
            assert_eq!(recolored.get_pixel(2, 0), icon.get_pixel(2, 0));
        }
    }

    #[test]
    fn prussian_icon_is_darker_than_blue() {
        let icon = RgbaImage::from_pixel(1, 1, Rgba([40, 120, 200, 255]));
        let prussian = *recolor_icon_for_theme(&icon, &theme_by_name("prussian-blue").unwrap()).get_pixel(0, 0);
        assert!(hsl_of(prussian).lightness() < lightness(Rgb(40, 120, 200)));
    }

    #[test]
    fn qt_hsl_values() {
        // Known QColor results.
        assert_eq!(hsl_hue(Rgb(255, 0, 0)), 0);
        assert_eq!(hsl_hue(Rgb(0, 255, 0)), 120);
        assert_eq!(hsl_hue(Rgb(0, 0, 255)), 240);
        assert_eq!(hsl_hue(Rgb(128, 128, 128)), -1);
        assert_eq!(hsl_saturation(Rgb(255, 0, 0)), 255);
        assert_eq!(lightness(Rgb(255, 0, 0)), 128);
        assert_eq!(lightness(Rgb(255, 255, 255)), 255);
        // Qt quirk: lightness 128 is slightly above 0.5, so pure red comes back as #ff0101.
        assert_eq!(from_hsl(0, 255, 128, 255), Rgba([255, 1, 1, 255]));
        assert_eq!(from_hsl(120, 255, 64, 255), Rgba([0, 128, 0, 255]));
        assert_eq!(from_hsl(-1, 0, 200, 7), Rgba([200, 200, 200, 7]));
        assert_eq!(from_hsl(360, 0, 0, 0), Rgba([0, 0, 0, 0])); // invalid
    }

    #[test]
    fn hsl_round_trip_is_stable() {
        // Recoloring a pixel to its own hue/saturation/lightness stays close. (Like
        // QColor, hslHue() truncates to whole degrees, so saturated colors can
        // move by a few levels in one channel.)
        for r in (0..=255u16).step_by(15) {
            for g in (0..=255u16).step_by(17) {
                for b in (0..=255u16).step_by(13) {
                    let (r, g, b) = (r as u8, g as u8, b as u8);
                    let hsl = QtHsl::from_rgb(r, g, b);
                    let back = from_hsl(hsl.hue(), hsl.saturation(), hsl.lightness(), 255);
                    for (x, y) in [(r, back[0]), (g, back[1]), (b, back[2])] {
                        assert!((i32::from(x) - i32::from(y)).abs() <= 5, "({r},{g},{b}) -> {back:?}");
                    }
                }
            }
        }
    }

    #[test]
    fn theme_from_settings_falls_back() {
        let dir = crate::settings::tests::temp_dir();
        let user = dir.join("user.ini");
        std::fs::write(&user, "[appearance]\ntheme=Jade\n").unwrap();
        let settings = AppSettings::new(user.clone(), dir.join("none.ini"));
        assert_eq!(theme_from_settings(&settings).name, "jade-green");
        std::fs::write(&user, "[appearance]\ntheme=purple\n").unwrap();
        let settings = AppSettings::new(user, dir.join("none.ini"));
        assert_eq!(theme_from_settings(&settings), default_theme());
        let settings = AppSettings::new(dir.join("x.ini"), dir.join("none.ini"));
        assert_eq!(theme_from_settings(&settings), default_theme());
    }

    #[test]
    fn themed_app_image_is_cached_and_blue_is_the_icon() {
        assert_eq!(themed_app_image(&default_theme()), app_icon_image());
        let sage = theme_by_name("sage").unwrap();
        assert_eq!(themed_app_image(&sage), themed_app_image(&sage));
        assert_ne!(themed_app_image(&sage), app_icon_image());
    }
}
