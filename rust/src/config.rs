//! Fonts and the overall look-and-sound configuration (port of
//! countdown-font.cpp and app-config.cpp, without Qt font objects).
//!
//! ```ini
//! [appearance]
//! countdown-font=oxanium      ; oxanium, jura, doto, outfit or sansation
//! countdown-weight=400        ; 100-900, or a name such as light, regular, semibold
//! message-font=outfit         ; same choices
//! message-weight=300
//! countdown-size=18           ; point sizes, 6-96
//! message-size=13
//! time-up-message-size=13     ; the message in the "Time's up" window, in the message font
//! animations=on
//! ```
//!
//! Oxanium, Jura, Doto and Outfit are variable fonts, so any weight in a font's
//! range works; values outside it are clamped (Oxanium 200-800, Jura 300-700,
//! Doto 100-900, Outfit 100-900). Sansation is bundled as Light and Regular (300-400).

use crate::font_weight::parse_font_weight;
use crate::settings::{AppSettings, parse_switch};
use crate::theme::{Theme, default_theme, theme_from_settings};

pub const DEFAULT_COUNTDOWN_FONT_ID: &str = "oxanium";
pub const DEFAULT_COUNTDOWN_WEIGHT: i32 = 400;
pub const DEFAULT_MESSAGE_FONT_ID: &str = "outfit";
pub const DEFAULT_MESSAGE_WEIGHT: i32 = 300;
pub const DEFAULT_COUNTDOWN_POINT_SIZE: i32 = 18;
pub const DEFAULT_MESSAGE_POINT_SIZE: i32 = 13;
pub const DEFAULT_TIME_UP_MESSAGE_POINT_SIZE: i32 = 13;
pub const MIN_FONT_POINT_SIZE: i32 = 6;
pub const MAX_FONT_POINT_SIZE: i32 = 96;
/// Letter spacing of the countdown (QFont::PercentageSpacing); 100 = none.
pub const COUNTDOWN_LETTER_SPACING_PERCENT: f32 = 125.0;

/// The font files compiled into tTime (SIL OFL, see src/Resources/fonts/*-OFL.txt),
/// as (file name, contents). The GUI registers them at startup.
pub const FONT_FILES: &[(&str, &[u8])] = &[
    ("Sansation-Regular.ttf", include_bytes!("../../src/Resources/fonts/Sansation-Regular.ttf")),
    ("Sansation-Light.ttf", include_bytes!("../../src/Resources/fonts/Sansation-Light.ttf")),
    ("Oxanium-Variable.ttf", include_bytes!("../../src/Resources/fonts/Oxanium-Variable.ttf")),
    ("Jura-Variable.ttf", include_bytes!("../../src/Resources/fonts/Jura-Variable.ttf")),
    ("Doto-Variable.ttf", include_bytes!("../../src/Resources/fonts/Doto-Variable.ttf")),
    ("Outfit-Variable.ttf", include_bytes!("../../src/Resources/fonts/Outfit-Variable.ttf")),
];

/// A bundled font. Each lists both family spellings: some Windows font APIs
/// report the default instance's name (e.g. "Oxanium ExtraLight"), others just "Oxanium".
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct BundledFont {
    pub id: &'static str,
    pub families: &'static [&'static str],
    pub min_weight: i32,
    pub max_weight: i32,
}

pub const BUNDLED_FONTS: &[BundledFont] = &[
    BundledFont { id: "oxanium", families: &["Oxanium", "Oxanium ExtraLight"], min_weight: 200, max_weight: 800 },
    BundledFont { id: "jura", families: &["Jura", "Jura Light"], min_weight: 300, max_weight: 700 },
    BundledFont { id: "doto", families: &["Doto", "Doto Black"], min_weight: 100, max_weight: 900 },
    BundledFont { id: "outfit", families: &["Outfit", "Outfit Thin"], min_weight: 100, max_weight: 900 },
    // Two static files, not a variable font (see families_for()).
    BundledFont { id: "sansation", families: &["Sansation"], min_weight: 300, max_weight: 400 },
];

const FONT_CHOICES: &str = "oxanium, jura, doto, outfit, sansation";

/// A resolved font choice, ready for the GUI.
#[derive(Debug, Clone, PartialEq)]
pub struct FontSpec {
    pub font_id: &'static str,
    /// The family to ask for first.
    pub family: &'static str,
    /// Further family names to try, in order.
    pub fallback_families: Vec<&'static str>,
    /// Clamped to the font's range; also the value for the variable "wght" axis.
    pub weight: i32,
    pub point_size: i32,
    /// Letter spacing in percent of the character width (100 = none).
    pub letter_spacing_percent: f32,
}

fn find_font(id: &str) -> Option<&'static BundledFont> {
    BUNDLED_FONTS.iter().find(|f| f.id == id)
}

fn font_or_default(id: &str, default_id: &str) -> &'static BundledFont {
    find_font(&id.trim().to_lowercase()).or_else(|| find_font(default_id)).expect("default font is bundled")
}

fn families_for(font: &BundledFont, weight: i32) -> &'static [&'static str] {
    // Sansation Light registers under its own family name on some platforms.
    if font.id == "sansation" && weight < 400 {
        return &["Sansation Light", "Sansation"];
    }
    font.families
}

fn make_font(chosen: &'static BundledFont, weight: i32, point_size: i32, letter_spacing_percent: f32) -> FontSpec {
    let clamped = weight.clamp(chosen.min_weight, chosen.max_weight);
    let families = families_for(chosen, clamped);
    FontSpec {
        font_id: chosen.id,
        family: families[0],
        fallback_families: families[1..].to_vec(),
        weight: clamped,
        point_size,
        letter_spacing_percent,
    }
}

/// Parses a point size setting: a whole number in MIN_FONT_POINT_SIZE..=MAX_FONT_POINT_SIZE.
pub fn parse_point_size(text: &str) -> Option<i32> {
    let size: i32 = text.trim().parse().ok()?;
    (MIN_FONT_POINT_SIZE..=MAX_FONT_POINT_SIZE).contains(&size).then_some(size)
}

/// The countdown font for a font id, weight and point size, clamping the weight
/// to what the font supports. Unknown ids use the default countdown font.
pub fn make_countdown_font(font_id: &str, weight: i32, point_size: i32) -> FontSpec {
    make_font(font_or_default(font_id, DEFAULT_COUNTDOWN_FONT_ID), weight, point_size, COUNTDOWN_LETTER_SPACING_PERCENT)
}

/// Same for the message font (unknown ids use the default message font).
pub fn make_message_font(font_id: &str, weight: i32, point_size: i32) -> FontSpec {
    make_font(font_or_default(font_id, DEFAULT_MESSAGE_FONT_ID), weight, point_size, 100.0)
}

struct FontSetting {
    font_key: &'static str,
    weight_key: &'static str,
    size_key: &'static str,
    default_id: &'static str,
    default_weight: i32,
    default_point_size: i32,
}

/// The part of "section/key" after the slash.
fn key_name(key: &str) -> &str {
    key.split_once('/').map_or("", |(_, name)| name)
}

fn read_point_size(settings: &AppSettings, key: &str, default_point_size: i32) -> i32 {
    let text = settings.value(key).unwrap_or_else(|| default_point_size.to_string());
    if let Some(size) = parse_point_size(&text) {
        return size;
    }
    log::warn!(
        "tTime.ini: invalid {} '{}', using {} (a whole number of points, {}-{})",
        key_name(key),
        text,
        default_point_size,
        MIN_FONT_POINT_SIZE,
        MAX_FONT_POINT_SIZE
    );
    default_point_size
}

/// Reads a font id, weight and size from `settings`, warning about invalid values.
fn read_font_setting(settings: &AppSettings, setting: &FontSetting) -> (&'static BundledFont, i32, i32) {
    let font_id =
        settings.value(setting.font_key).unwrap_or_else(|| setting.default_id.to_owned()).trim().to_lowercase();
    let font = find_font(&font_id).unwrap_or_else(|| {
        log::warn!(
            "tTime.ini: unknown {} '{}', using '{}' (choices: {})",
            key_name(setting.font_key),
            font_id,
            setting.default_id,
            FONT_CHOICES
        );
        font_or_default("", setting.default_id)
    });

    let weight_name = key_name(setting.weight_key);
    let weight_text = settings.value(setting.weight_key).unwrap_or_else(|| setting.default_weight.to_string());
    let weight = parse_font_weight(&weight_text).unwrap_or_else(|| {
        log::warn!("tTime.ini: invalid {} '{}', using {}", weight_name, weight_text, setting.default_weight);
        setting.default_weight
    });
    if weight < font.min_weight || weight > font.max_weight {
        log::warn!(
            "tTime.ini: {} {} is outside {}'s range {}-{}, using the nearest",
            weight_name,
            weight,
            font.id,
            font.min_weight,
            font.max_weight
        );
    }
    (font, weight, read_point_size(settings, setting.size_key, setting.default_point_size))
}

/// The countdown font configured in `settings` (appearance/countdown-font,
/// -weight and -size), with warnings and defaults for invalid values.
pub fn countdown_font_from_settings(settings: &AppSettings) -> FontSpec {
    let (font, weight, size) = read_font_setting(
        settings,
        &FontSetting {
            font_key: "appearance/countdown-font",
            weight_key: "appearance/countdown-weight",
            size_key: "appearance/countdown-size",
            default_id: DEFAULT_COUNTDOWN_FONT_ID,
            default_weight: DEFAULT_COUNTDOWN_WEIGHT,
            default_point_size: DEFAULT_COUNTDOWN_POINT_SIZE,
        },
    );
    make_countdown_font(font.id, weight, size)
}

/// The message font configured in `settings` (appearance/message-font,
/// -weight and -size), with warnings and defaults for invalid values.
pub fn message_font_from_settings(settings: &AppSettings) -> FontSpec {
    let (font, weight, size) = read_font_setting(
        settings,
        &FontSetting {
            font_key: "appearance/message-font",
            weight_key: "appearance/message-weight",
            size_key: "appearance/message-size",
            default_id: DEFAULT_MESSAGE_FONT_ID,
            default_weight: DEFAULT_MESSAGE_WEIGHT,
            default_point_size: DEFAULT_MESSAGE_POINT_SIZE,
        },
    );
    make_message_font(font.id, weight, size)
}

/// The point size of the message in the "Time's up" window (appearance/time-up-message-size).
pub fn time_up_message_point_size_from_settings(settings: &AppSettings) -> i32 {
    read_point_size(settings, "appearance/time-up-message-size", DEFAULT_TIME_UP_MESSAGE_POINT_SIZE)
}

fn animations_from_settings(settings: &AppSettings) -> bool {
    let text = settings.value("appearance/animations").unwrap_or_else(|| "on".to_owned());
    parse_switch(&text).unwrap_or_else(|| {
        log::warn!("tTime.ini: invalid animations '{}', using 'on' (choices: on, off)", text);
        true
    })
}

/// The look and sound of tTime, read once at startup and passed to the windows.
/// (Window geometry is not part of it: it changes while the app runs and is
/// saved through AppSettings.)
#[derive(Debug, Clone, PartialEq)]
pub struct AppConfig {
    pub theme: Theme,
    pub countdown_font: FontSpec,
    /// The alarm message below the countdown.
    pub message_font: FontSpec,
    /// The alarm message in the "Time's up" window (in the message font).
    pub time_up_message_point_size: i32,
    /// Play the bundled alarm sound (`wav_info::ALARM_WAV`); false = silent.
    pub alarm_sound: bool,
    /// Swaying tree and ringing tray icon in the "Time's up" window.
    pub animations: bool,
}

impl AppConfig {
    /// From tTime.ini (see AppSettings), with the built-in defaults where a key is missing.
    pub fn from_settings(settings: &AppSettings) -> Self {
        Self {
            theme: theme_from_settings(settings),
            countdown_font: countdown_font_from_settings(settings),
            message_font: message_font_from_settings(settings),
            time_up_message_point_size: time_up_message_point_size_from_settings(settings),
            alarm_sound: true,
            animations: animations_from_settings(settings),
        }
    }

    /// The built-in defaults only.
    pub fn defaults() -> Self {
        Self {
            theme: default_theme(),
            countdown_font: make_countdown_font(
                DEFAULT_COUNTDOWN_FONT_ID,
                DEFAULT_COUNTDOWN_WEIGHT,
                DEFAULT_COUNTDOWN_POINT_SIZE,
            ),
            message_font: make_message_font(
                DEFAULT_MESSAGE_FONT_ID,
                DEFAULT_MESSAGE_WEIGHT,
                DEFAULT_MESSAGE_POINT_SIZE,
            ),
            time_up_message_point_size: DEFAULT_TIME_UP_MESSAGE_POINT_SIZE,
            alarm_sound: true,
            animations: true,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::settings::{parse_qt_point, parse_qt_size, tests::temp_dir};
    use crate::theme::DEFAULT_THEME_NAME;
    use std::path::PathBuf;

    const SHIPPED_INI: &str = concat!(env!("CARGO_MANIFEST_DIR"), "/../src/tTime.ini");

    fn shipped() -> AppSettings {
        AppSettings::new(temp_dir().join("none.ini"), PathBuf::from(SHIPPED_INI))
    }

    fn with_user(text: &str) -> AppSettings {
        let dir = temp_dir();
        std::fs::write(dir.join("user.ini"), text).unwrap();
        AppSettings::new(dir.join("user.ini"), dir.join("none.ini"))
    }

    #[test]
    fn shipped_ini_matches_built_in_defaults() {
        let ini = shipped();
        let v = |key: &str| ini.value(key).unwrap_or_else(|| panic!("{key} missing"));
        assert_eq!(v("appearance/countdown-font"), DEFAULT_COUNTDOWN_FONT_ID);
        assert_eq!(parse_font_weight(&v("appearance/countdown-weight")), Some(DEFAULT_COUNTDOWN_WEIGHT));
        assert_eq!(v("appearance/message-font"), DEFAULT_MESSAGE_FONT_ID);
        assert_eq!(parse_font_weight(&v("appearance/message-weight")), Some(DEFAULT_MESSAGE_WEIGHT));
        assert_eq!(parse_point_size(&v("appearance/countdown-size")), Some(DEFAULT_COUNTDOWN_POINT_SIZE));
        assert_eq!(parse_point_size(&v("appearance/message-size")), Some(DEFAULT_MESSAGE_POINT_SIZE));
        assert_eq!(parse_point_size(&v("appearance/time-up-message-size")), Some(DEFAULT_TIME_UP_MESSAGE_POINT_SIZE));
        assert_eq!(v("appearance/theme"), DEFAULT_THEME_NAME);
        assert_eq!(parse_switch(&v("appearance/animations")), Some(true));
        assert_eq!(AppConfig::from_settings(&ini), AppConfig::defaults());
    }

    #[test]
    fn window_defaults_parse() {
        let ini = shipped();
        assert_eq!(ini.value("progress-dialog/size").as_deref().and_then(parse_qt_size), Some((200, 130)));
        assert_eq!(ini.value("progress-dialog/pos").as_deref().and_then(parse_qt_point), Some((250, 250)));
    }

    #[test]
    fn only_known_keys() {
        let text = std::fs::read_to_string(SHIPPED_INI).unwrap();
        let known = [
            "appearance/countdown-font",
            "appearance/countdown-weight",
            "appearance/message-font",
            "appearance/message-weight",
            "appearance/countdown-size",
            "appearance/message-size",
            "appearance/time-up-message-size",
            "appearance/theme",
            "appearance/animations",
            "progress-dialog/size",
            "progress-dialog/pos",
        ];
        let keys = crate::settings::ini_keys(&text);
        assert_eq!(keys.len(), known.len());
        for key in keys {
            assert!(known.contains(&key.as_str()), "unexpected key: {key}");
        }
    }

    #[test]
    fn missing_settings_are_the_defaults() {
        let dir = temp_dir();
        let settings = AppSettings::new(dir.join("a.ini"), dir.join("b.ini"));
        assert_eq!(AppConfig::from_settings(&settings), AppConfig::defaults());
    }

    #[test]
    fn default_fonts() {
        let config = AppConfig::defaults();
        assert_eq!(
            config.countdown_font,
            FontSpec {
                font_id: "oxanium",
                family: "Oxanium",
                fallback_families: vec!["Oxanium ExtraLight"],
                weight: 400,
                point_size: 18,
                letter_spacing_percent: 125.0,
            }
        );
        assert_eq!(config.message_font.family, "Outfit");
        assert_eq!(config.message_font.weight, 300);
        assert_eq!(config.message_font.point_size, 13);
        assert_eq!(config.message_font.letter_spacing_percent, 100.0);
        assert!(config.alarm_sound && config.animations);
    }

    #[test]
    fn weights_are_clamped_and_sansation_light_has_its_own_family() {
        assert_eq!(make_countdown_font("oxanium", 900, 18).weight, 800);
        assert_eq!(make_countdown_font("jura", 100, 18).weight, 300);
        let light = make_message_font("sansation", 300, 13);
        assert_eq!((light.family, light.fallback_families.clone()), ("Sansation Light", vec!["Sansation"]));
        let regular = make_message_font("Sansation", 700, 13);
        assert_eq!((regular.family, regular.weight), ("Sansation", 400));
        assert!(regular.fallback_families.is_empty());
        assert_eq!(make_countdown_font("nope", 400, 18).font_id, "oxanium");
        assert_eq!(make_message_font("nope", 400, 13).font_id, "outfit");
    }

    #[test]
    fn invalid_settings_fall_back() {
        let settings = with_user(
            "[appearance]\ncountdown-font=Comic\ncountdown-weight=chunky\ncountdown-size=200\n\
             message-font= JURA \nmessage-weight=semibold\nmessage-size=20\ntime-up-message-size=x\n\
             theme=sage\nanimations=maybe\n",
        );
        let config = AppConfig::from_settings(&settings);
        assert_eq!(config.countdown_font, make_countdown_font("oxanium", 400, 18));
        assert_eq!(config.message_font, make_message_font("jura", 600, 20));
        assert_eq!(config.time_up_message_point_size, 13);
        assert_eq!(config.theme.name, "sage");
        assert!(config.animations);
        let off = with_user("[appearance]\nanimations=Off\n");
        assert!(!AppConfig::from_settings(&off).animations);
    }

    #[test]
    fn parses_point_sizes() {
        assert_eq!(parse_point_size(" 18 "), Some(18));
        assert_eq!(parse_point_size("6"), Some(6));
        assert_eq!(parse_point_size("96"), Some(96));
        assert_eq!(parse_point_size("5"), None);
        assert_eq!(parse_point_size("97"), None);
        assert_eq!(parse_point_size("12pt"), None);
        assert_eq!(parse_point_size(""), None);
    }

    #[test]
    fn bundled_fonts_are_embedded() {
        assert_eq!(FONT_FILES.len(), 6);
        for (name, bytes) in FONT_FILES {
            assert!(bytes.len() > 1000, "{name}");
        }
    }
}
