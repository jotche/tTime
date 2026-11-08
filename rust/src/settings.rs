//! tTime's settings (port of app-settings.cpp without Qt), read from two INI
//! files:
//!
//!   1. the user's file      `%APPDATA%\tTime\tTime.ini`   (read/write)
//!   2. the shipped defaults `<exe dir>\tTime.ini`         (read-only)
//!
//! [`AppSettings::value`] returns the first file that has the key, else `None`
//! (callers use the built-in default). [`AppSettings::set_value`] only ever
//! writes the user's file.
//!
//! The INI dialect is the subset of what `QSettings` (IniFormat) reads and
//! writes that tTime needs, so existing users' files keep working:
//! `[section]` headers, `key=value` lines, lines starting with `;` are comments
//! (`#` is *not*: `#key=value` is the key `#key`, which QSettings writes back
//! as `%23key`), keys are percent-encoded (`%XX`, `%UXXXX`) with `\` standing
//! for `/` inside a key, keys without a section live in `[General]`, values are
//! trimmed and surrounding double quotes are stripped.

use std::collections::HashMap;
use std::fs;
use std::io;
use std::path::{Path, PathBuf};

/// Fixed on purpose, so the user's settings stay where they are.
const SETTINGS_DIR: &str = "tTime";
const SETTINGS_FILE: &str = "tTime.ini";
const GENERAL: &str = "General";

// ---------------------------------------------------------------------------
// Key and value escaping (QSettings compatible)

/// Decodes a key or section name as QSettings writes it: `%XX` and `%UXXXX`
/// escapes, `\` for `/`.
fn decode_key(raw: &str) -> String {
    let chars: Vec<char> = raw.chars().collect();
    let mut out = String::new();
    let mut i = 0;
    while i < chars.len() {
        let c = chars[i];
        if c == '\\' {
            out.push('/');
            i += 1;
        } else if c == '%' {
            let hex = |from: usize, len: usize| -> Option<u32> {
                let s: String = chars.get(from..from + len)?.iter().collect();
                u32::from_str_radix(&s, 16).ok()
            };
            if chars.get(i + 1) == Some(&'U')
                && let Some(code) = hex(i + 2, 4)
            {
                out.push(char::from_u32(code).unwrap_or('\u{fffd}'));
                i += 6;
            } else if let Some(code) = hex(i + 1, 2) {
                out.push(char::from_u32(code).unwrap_or('\u{fffd}'));
                i += 3;
            } else {
                out.push(c);
                i += 1;
            }
        } else {
            out.push(c);
            i += 1;
        }
    }
    out
}

/// Encodes a key or section name like QSettings: `[A-Za-z0-9_.-]` stay,
/// `/` becomes `\`, anything else `%XX` (or `%UXXXX` beyond Latin-1).
fn encode_key(key: &str) -> String {
    let mut out = String::new();
    for c in key.chars() {
        if c == '/' {
            out.push('\\');
        } else if c.is_ascii_alphanumeric() || matches!(c, '-' | '_' | '.') {
            out.push(c);
        } else if (c as u32) <= 0xff {
            out.push_str(&format!("%{:02X}", c as u32));
        } else {
            // Characters outside the BMP are written as their UTF-16 units.
            let mut units = [0u16; 2];
            for unit in c.encode_utf16(&mut units) {
                out.push_str(&format!("%U{:04X}", unit));
            }
        }
    }
    out
}

fn decode_value(raw: &str) -> String {
    let value = raw.trim();
    if value.len() >= 2 && value.starts_with('"') && value.ends_with('"') {
        let inner = &value[1..value.len() - 1];
        let mut out = String::with_capacity(inner.len());
        let mut chars = inner.chars();
        while let Some(c) = chars.next() {
            if c == '\\' {
                match chars.next() {
                    Some('n') => out.push('\n'),
                    Some('t') => out.push('\t'),
                    Some('r') => out.push('\r'),
                    Some(other) => out.push(other),
                    None => out.push('\\'),
                }
            } else {
                out.push(c);
            }
        }
        return out;
    }
    value.to_owned()
}

fn encode_value(value: &str) -> String {
    let needs_quotes =
        value != value.trim() || value.starts_with('"') || value.contains([';', ',', '"', '\n', '\r', '\t']);
    if !needs_quotes {
        return value.to_owned();
    }
    let mut out = String::from("\"");
    for c in value.chars() {
        match c {
            '\\' => out.push_str("\\\\"),
            '"' => out.push_str("\\\""),
            '\n' => out.push_str("\\n"),
            '\r' => out.push_str("\\r"),
            '\t' => out.push_str("\\t"),
            _ => out.push(c),
        }
    }
    out.push('"');
    out
}

/// Splits "section/key" into (section, key); keys without a slash live in [General].
fn split_key(key: &str) -> (&str, &str) {
    let key = key.trim_matches('/');
    key.split_once('/').unwrap_or((GENERAL, key))
}

// ---------------------------------------------------------------------------
// One INI file

#[derive(Debug, Clone, PartialEq)]
enum Line {
    Section { raw: String, name: String },
    Entry { raw: String, key: String, value: String },
    Other(String), // comments, blank lines, garbage
}

#[derive(Debug, Clone, Default)]
struct IniFile {
    lines: Vec<Line>,
    crlf: bool,
}

impl IniFile {
    fn parse(text: &str) -> Self {
        let text = text.strip_prefix('\u{feff}').unwrap_or(text);
        let crlf = if text.is_empty() { cfg!(windows) } else { text.contains("\r\n") };
        let lines = text
            .lines()
            .map(|raw| {
                let trimmed = raw.trim();
                if trimmed.starts_with(';') || trimmed.is_empty() {
                    Line::Other(raw.to_owned())
                } else if trimmed.starts_with('[') && trimmed.ends_with(']') {
                    Line::Section { raw: raw.to_owned(), name: decode_key(trimmed[1..trimmed.len() - 1].trim()) }
                } else if let Some((key, value)) = trimmed.split_once('=') {
                    Line::Entry { raw: raw.to_owned(), key: decode_key(key.trim()), value: decode_value(value) }
                } else {
                    Line::Other(raw.to_owned())
                }
            })
            .collect();
        Self { lines, crlf }
    }

    fn read(path: &Path) -> Self {
        match fs::read(path) {
            Ok(bytes) => Self::parse(&String::from_utf8_lossy(&bytes)),
            Err(_) => Self::parse(""),
        }
    }

    /// All entries as ("section/key" or "key", value); later duplicates win.
    fn entries(&self) -> HashMap<String, String> {
        let mut map = HashMap::new();
        let mut section = GENERAL.to_owned();
        for line in &self.lines {
            match line {
                Line::Section { name, .. } => section = name.clone(),
                Line::Entry { key, value, .. } => {
                    let full =
                        if section == GENERAL || section.is_empty() { key.clone() } else { format!("{section}/{key}") };
                    map.insert(full, value.clone());
                }
                Line::Other(_) => {}
            }
        }
        map
    }

    fn set(&mut self, full_key: &str, value: &str) {
        let (section, key) = split_key(full_key);
        let is_section = |name: &str| name == section || (section == GENERAL && name.is_empty());
        let new_line = format!("{}={}", encode_key(key), encode_value(value));
        let entry = Line::Entry { raw: new_line, key: key.to_owned(), value: value.to_owned() };

        let mut current_matches = section == GENERAL; // keys before any header are [General]
        let mut existing = None;
        let mut insert_after = None;
        for (i, line) in self.lines.iter().enumerate() {
            match line {
                Line::Section { name, .. } => {
                    current_matches = is_section(name);
                    if current_matches {
                        insert_after = Some(i);
                    }
                }
                Line::Entry { key: k, .. } if current_matches => {
                    insert_after = Some(i);
                    if k == key {
                        existing = Some(i);
                    }
                }
                _ => {}
            }
        }
        if let Some(i) = existing {
            self.lines[i] = entry;
        } else if let Some(i) = insert_after {
            self.lines.insert(i + 1, entry);
        } else if section == GENERAL {
            self.lines.insert(0, entry);
        } else {
            if self.lines.last().is_some_and(|l| !matches!(l, Line::Other(s) if s.trim().is_empty())) {
                self.lines.push(Line::Other(String::new()));
            }
            self.lines.push(Line::Section { raw: format!("[{}]", encode_key(section)), name: section.to_owned() });
            self.lines.push(entry);
        }
    }

    fn to_text(&self) -> String {
        let eol = if self.crlf { "\r\n" } else { "\n" };
        let mut text = String::new();
        for line in &self.lines {
            text.push_str(match line {
                Line::Section { raw, .. } | Line::Entry { raw, .. } | Line::Other(raw) => raw,
            });
            text.push_str(eol);
        }
        text
    }
}

// ---------------------------------------------------------------------------

/// The user's settings file over the shipped defaults. Both files are read once,
/// at construction.
#[derive(Debug, Clone)]
pub struct AppSettings {
    user_file: PathBuf,
    user: IniFile,
    user_values: HashMap<String, String>,
    defaults_values: HashMap<String, String>,
}

impl AppSettings {
    pub fn new(user_file: PathBuf, defaults_file: PathBuf) -> Self {
        let user = IniFile::read(&user_file);
        let user_values = user.entries();
        let defaults_values = IniFile::read(&defaults_file).entries();
        Self { user_file, user, user_values, defaults_values }
    }

    /// The settings of the running application.
    pub fn for_application() -> Self {
        Self::new(Self::user_file_path(), Self::defaults_file_path())
    }

    /// `%APPDATA%\tTime\tTime.ini` on Windows; `$XDG_CONFIG_HOME/tTime/tTime.ini`
    /// (default `~/.config`) elsewhere, like QSettings' IniFormat UserScope.
    pub fn user_file_path() -> PathBuf {
        let env_dir = |name: &str| std::env::var_os(name).filter(|v| !v.is_empty()).map(PathBuf::from);
        let base = if cfg!(windows) {
            env_dir("APPDATA")
                .unwrap_or_else(|| env_dir("USERPROFILE").unwrap_or_default().join("AppData").join("Roaming"))
        } else {
            env_dir("XDG_CONFIG_HOME").unwrap_or_else(|| env_dir("HOME").unwrap_or_default().join(".config"))
        };
        base.join(SETTINGS_DIR).join(SETTINGS_FILE)
    }

    /// `<exe dir>/tTime.ini`; on macOS `tTime.app/Contents/Resources/tTime.ini`.
    pub fn defaults_file_path() -> PathBuf {
        let exe_dir =
            std::env::current_exe().ok().and_then(|exe| exe.parent().map(Path::to_path_buf)).unwrap_or_default();
        if cfg!(target_os = "macos") {
            exe_dir.join("..").join("Resources").join(SETTINGS_FILE)
        } else {
            exe_dir.join(SETTINGS_FILE)
        }
    }

    /// The value of "section/key" (or "key" for [General]): the user's file
    /// first, then the shipped defaults.
    pub fn value(&self, key: &str) -> Option<String> {
        let key = normalize_key(key);
        self.user_values.get(&key).or_else(|| self.defaults_values.get(&key)).cloned()
    }

    /// Writes `key` to the user's file right away (creating its directory),
    /// keeping its other sections, keys and comments.
    pub fn set_value(&mut self, key: &str, value: &str) {
        let key = normalize_key(key);
        self.user.set(&key, value);
        self.user_values.insert(key, value.to_owned());
        if let Err(err) = self.write_user_file() {
            log::warn!("Could not write {}: {err}", self.user_file.display());
        }
    }

    fn write_user_file(&self) -> io::Result<()> {
        if let Some(dir) = self.user_file.parent() {
            fs::create_dir_all(dir)?;
        }
        fs::write(&self.user_file, self.user.to_text())
    }
}

/// All keys of an INI text ("section/key"), for tests.
#[cfg(test)]
pub(crate) fn ini_keys(text: &str) -> Vec<String> {
    IniFile::parse(text).entries().into_keys().collect()
}

fn normalize_key(key: &str) -> String {
    let (section, key) = split_key(key);
    if section == GENERAL { key.to_owned() } else { format!("{section}/{key}") }
}

/// Parses an on/off setting: on, off, true, false, yes, no, 1, 0 (any case).
pub fn parse_switch(text: &str) -> Option<bool> {
    match text.trim().to_lowercase().as_str() {
        "on" | "true" | "yes" | "1" => Some(true),
        "off" | "false" | "no" | "0" => Some(false),
        _ => None,
    }
}

fn parse_qt_pair(text: &str, prefix: &str) -> Option<(i32, i32)> {
    let inner = text.trim().strip_prefix(prefix)?.strip_suffix(')')?;
    let mut parts = inner.split_whitespace();
    let a = parts.next()?.parse().ok()?;
    let b = parts.next()?.parse().ok()?;
    parts.next().is_none().then_some((a, b))
}

/// Parses QSettings' QSize notation, "@Size(200 130)" -> (200, 130).
pub fn parse_qt_size(text: &str) -> Option<(i32, i32)> {
    parse_qt_pair(text, "@Size(")
}

/// Parses QSettings' QPoint notation, "@Point(250 250)" -> (250, 250).
pub fn parse_qt_point(text: &str) -> Option<(i32, i32)> {
    parse_qt_pair(text, "@Point(")
}

pub fn format_qt_size(size: (i32, i32)) -> String {
    format!("@Size({} {})", size.0, size.1)
}

pub fn format_qt_point(point: (i32, i32)) -> String {
    format!("@Point({} {})", point.0, point.1)
}

#[cfg(test)]
pub(crate) mod tests {
    use super::*;
    use std::sync::atomic::{AtomicU32, Ordering};

    /// A fresh, empty temporary directory per call.
    pub(crate) fn temp_dir() -> PathBuf {
        static COUNTER: AtomicU32 = AtomicU32::new(0);
        let dir = std::env::temp_dir().join(format!(
            "ttime-test-{}-{}",
            std::process::id(),
            COUNTER.fetch_add(1, Ordering::Relaxed)
        ));
        let _ = fs::remove_dir_all(&dir);
        fs::create_dir_all(&dir).unwrap();
        dir
    }

    fn write(path: &Path, key: &str, value: &str) {
        let mut ini = IniFile::read(path);
        ini.set(key, value);
        fs::write(path, ini.to_text()).unwrap();
    }

    #[test]
    fn user_overrides_defaults_overrides_built_in() {
        let dir = temp_dir();
        let (user, defaults) = (dir.join("user.ini"), dir.join("defaults.ini"));
        write(&defaults, "appearance/theme", "sage");
        write(&defaults, "appearance/countdown-weight", "300");
        write(&user, "appearance/countdown-weight", "500");

        let settings = AppSettings::new(user, defaults);
        assert_eq!(settings.value("appearance/countdown-weight").as_deref(), Some("500"));
        assert_eq!(settings.value("appearance/theme").as_deref(), Some("sage"));
        assert_eq!(settings.value("appearance/countdown-font"), None);
    }

    #[test]
    fn missing_files_use_the_fallback() {
        let dir = temp_dir();
        let settings = AppSettings::new(dir.join("user.ini"), dir.join("defaults.ini"));
        assert_eq!(settings.value("appearance/theme"), None);
    }

    #[test]
    fn writes_go_to_the_user_file_only() {
        let dir = temp_dir();
        let (user, defaults) = (dir.join("sub").join("user.ini"), dir.join("defaults.ini"));
        write(&defaults, "progress-dialog/pos", &format_qt_point((250, 250)));
        {
            let mut settings = AppSettings::new(user.clone(), defaults.clone());
            settings.set_value("progress-dialog/pos", &format_qt_point((10, 20)));
            assert_eq!(settings.value("progress-dialog/pos").as_deref().and_then(parse_qt_point), Some((10, 20)));
        }
        let reread = AppSettings::new(user, dir.join("none.ini"));
        assert_eq!(reread.value("progress-dialog/pos").as_deref().and_then(parse_qt_point), Some((10, 20)));
        let shipped = AppSettings::new(dir.join("none.ini"), defaults);
        assert_eq!(shipped.value("progress-dialog/pos").as_deref().and_then(parse_qt_point), Some((250, 250)));
    }

    #[test]
    fn set_value_preserves_the_rest_of_the_file() {
        let dir = temp_dir();
        let user = dir.join("user.ini");
        let original = "; my comment\r\n[appearance]\r\ntheme=sage\r\n%23countdown-font=jura\r\n\r\n[progress-dialog]\r\nsize=@Size(300 130)\r\n";
        fs::write(&user, original).unwrap();
        let mut settings = AppSettings::new(user.clone(), dir.join("none.ini"));
        settings.set_value("progress-dialog/pos", "@Point(1 2)");
        settings.set_value("appearance/theme", "jade");
        settings.set_value("appearance/animations", "off");
        settings.set_value("top", "level");
        settings.set_value("new-section/key", "a; b");
        let text = fs::read_to_string(&user).unwrap();
        assert_eq!(
            text,
            "top=level\r\n; my comment\r\n[appearance]\r\ntheme=jade\r\n%23countdown-font=jura\r\nanimations=off\r\n\r\n\
             [progress-dialog]\r\nsize=@Size(300 130)\r\npos=@Point(1 2)\r\n\r\n[new-section]\r\nkey=\"a; b\"\r\n"
        );
        let reread = AppSettings::new(user, dir.join("none.ini"));
        assert_eq!(reread.value("new-section/key").as_deref(), Some("a; b"));
        assert_eq!(reread.value("top").as_deref(), Some("level"));
        assert_eq!(reread.value("General/top").as_deref(), Some("level"));
        assert_eq!(reread.value("appearance/#countdown-font").as_deref(), Some("jura"));
    }

    #[test]
    fn hash_is_not_a_comment() {
        // From a real tTime.ini: QSettings treats "#countdown-font" as a key name
        // and writes it back percent-encoded.
        let dir = temp_dir();
        let user = dir.join("user.ini");
        fs::write(&user, "[appearance]\n#countdown-font=oxanium\ntheme=sage\n").unwrap();
        let mut settings = AppSettings::new(user.clone(), dir.join("none.ini"));
        assert_eq!(settings.value("appearance/countdown-font"), None);
        assert_eq!(settings.value("appearance/#countdown-font").as_deref(), Some("oxanium"));

        settings.set_value("appearance/theme", "blue");
        let text = fs::read_to_string(&user).unwrap();
        assert_eq!(text, "[appearance]\n#countdown-font=oxanium\ntheme=blue\n");

        // The encoded spelling QSettings writes reads back as the same key.
        fs::write(&user, "[appearance]\n%23countdown-font=oxanium\n").unwrap();
        let settings = AppSettings::new(user, dir.join("none.ini"));
        assert_eq!(settings.value("appearance/countdown-font"), None);
        assert_eq!(settings.value("appearance/#countdown-font").as_deref(), Some("oxanium"));

        // New keys with such characters are written encoded.
        let mut ini = IniFile::default();
        ini.set("appearance/#countdown-font", "x");
        assert_eq!(ini.to_text(), "[appearance]\n%23countdown-font=x\n");
    }

    #[test]
    fn key_escaping_round_trips() {
        for key in ["countdown-font", "#a b", "x/y", "ü€", "a.b_c"] {
            assert_eq!(decode_key(&encode_key(key)), key);
        }
        assert_eq!(encode_key("#a b"), "%23a%20b");
        assert_eq!(encode_key("x/y"), "x\\y");
        assert_eq!(encode_key("€"), "%U20AC");
    }

    #[test]
    fn values_are_trimmed_and_unquoted() {
        let ini = IniFile::parse("[a]\n k1 =  v1  \nk2=\"  spaced \\\"q\\\" \"\nk3=x=y\n");
        let e = ini.entries();
        assert_eq!(e["a/k1"], "v1");
        assert_eq!(e["a/k2"], "  spaced \"q\" ");
        assert_eq!(e["a/k3"], "x=y");
    }

    #[test]
    fn parses_switches() {
        for on in ["on", "ON", " true ", "yes", "1"] {
            assert_eq!(parse_switch(on), Some(true), "{on}");
        }
        for off in ["off", "False", "no", "0"] {
            assert_eq!(parse_switch(off), Some(false), "{off}");
        }
        assert_eq!(parse_switch("maybe"), None);
        assert_eq!(parse_switch(""), None);
    }

    #[test]
    fn qt_variants() {
        assert_eq!(parse_qt_size("@Size(200 130)"), Some((200, 130)));
        assert_eq!(parse_qt_point("@Point(-250 250)"), Some((-250, 250)));
        assert_eq!(parse_qt_point("@Size(1 2)"), None);
        assert_eq!(parse_qt_point("@Point(1)"), None);
        assert_eq!(format_qt_size((200, 130)), "@Size(200 130)");
        assert_eq!(format_qt_point((-1, 2)), "@Point(-1 2)");
    }

    #[test]
    fn paths() {
        let user = AppSettings::user_file_path();
        assert!(user.ends_with(Path::new("tTime").join("tTime.ini")), "{}", user.display());
        let exe_dir = std::env::current_exe().unwrap().parent().unwrap().to_path_buf();
        let expected = if cfg!(target_os = "macos") {
            exe_dir.join("..").join("Resources").join("tTime.ini")
        } else {
            exe_dir.join("tTime.ini")
        };
        assert_eq!(AppSettings::defaults_file_path(), expected);
    }
}
