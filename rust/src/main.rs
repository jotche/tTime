//! tTime: countdown alarms ("tea timer") that live in the system tray.
//!
//! `tTime 5m green tea` starts an alarm; without arguments (or with an
//! unparsable one) the "New alarm" input window opens.

#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

mod gui;

use std::borrow::Cow;
use std::cell::RefCell;
use std::rc::Rc;

use gpui::{App, Application};
use ttime::config::{AppConfig, FONT_FILES, FontSpec};
use ttime::settings::AppSettings;

use gui::manager::{self, Shared};

const VERSION: &str = env!("CARGO_PKG_VERSION");

const HELP: &str = "Usage: tTime [options] [alarm...]
Countdown alarms in the system tray.

Options:
  -h, --help     Displays help on commandline options.
  -v, --version  Displays version information.

Arguments:
  alarm          Duration and optional message, e.g. \"5m green tea\" or
                 \"1h 30m meeting\". Opens an input window if omitted.";

fn main() {
    env_logger::Builder::new().filter_level(log::LevelFilter::Warn).parse_default_env().init();

    let mut words = Vec::new();
    for arg in std::env::args().skip(1) {
        match arg.as_str() {
            "-h" | "--help" | "-?" | "/?" => {
                gui::platform::message_box("tTime", HELP);
                return;
            }
            "-v" | "--version" => {
                gui::platform::message_box("tTime", &format!("tTime {VERSION}"));
                return;
            }
            _ => words.push(arg),
        }
    }
    let initial_spec = words.join(" ");

    Application::new().run(move |cx: &mut App| {
        load_bundled_fonts(cx);

        // Settings are read once: the user's tTime.ini, then <exe dir>/tTime.ini.
        let settings = AppSettings::for_application();
        let mut config = AppConfig::from_settings(&settings);
        resolve_family(&mut config.countdown_font, cx);
        resolve_family(&mut config.message_font, cx);

        gui::text_input::bind_keys(cx);
        gui::tray::init(cx);
        let shared = Rc::new(Shared { config, settings: RefCell::new(settings), icons: gui::style::Icons::load() });
        manager::init(shared, cx);
        // Started from a task: effects queued directly inside `run` are not flushed.
        let initial_spec = initial_spec.clone();
        cx.spawn(async move |cx| cx.update(|cx| manager::start(initial_spec, cx))).detach();
    });
}

/// Registers the fonts compiled into tTime.
fn load_bundled_fonts(cx: &App) {
    let fonts = FONT_FILES.iter().map(|(_, bytes)| Cow::Borrowed(*bytes)).collect();
    if let Err(error) = cx.text_system().add_fonts(fonts) {
        log::warn!("Could not load the bundled fonts: {error}");
    }
}

/// Some platforms register a bundled font under another family name (e.g.
/// "Sansation Light"); use the first name the text system knows.
fn resolve_family(spec: &mut FontSpec, cx: &App) {
    let names = cx.text_system().all_font_names();
    let known = |family: &str| names.iter().any(|name| name == family);
    if known(spec.family) {
        return;
    }
    if let Some(family) = spec.fallback_families.iter().copied().find(|family| known(family)) {
        spec.family = family;
    } else {
        log::warn!("Font family '{}' not found", spec.family);
    }
}
