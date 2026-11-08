//! Frameless countdown window with its own tray icon that fills up as time passes.
//!
//! Left-drag moves the window, Ctrl+left-drag changes its width (the height
//! always follows the content), right-click opens the context menu. Alt+F4
//! hides it to the tray. It remembers its size and position in tTime.ini.

use std::rc::Rc;
use std::time::Duration;

use gpui::{
    App, Bounds, Context, Entity, Font, IntoElement, MouseButton, MouseDownEvent, MouseMoveEvent, ParentElement,
    Pixels, Render, SharedString, Styled, Task, TextRun, TitlebarOptions, Window, WindowBackgroundAppearance,
    WindowBounds, WindowHandle, WindowKind, WindowOptions, canvas, div, point, prelude::*, px, rgb, size,
};
use image::RgbaImage;
use tray_icon::TrayIcon;
use tray_icon::menu::Menu;
use ttime::alarm::{Alarm, AlarmState};
use ttime::alarm_spec::format_duration;
use ttime::digit_layout::layout_fixed_digits;
use ttime::settings::{format_qt_point, format_qt_size, parse_qt_point, parse_qt_size};
use ttime::theme::themed_app_image;
use ttime::window_placement::{Rect, cascade_position, fit_into_visible_area};
use ttime::window_slot::WindowSlot;

use super::manager::{self, Shared};
use super::platform;
use super::style::{self, PanelStyle, Tooltip};
use super::tray::{self, TrayEvent};

// Historical key names (the window used to be called the "progress dialog"); kept so
// existing tTime.ini files keep their saved window size and position.
const SIZE_KEY: &str = "progress-dialog/size";
const POS_KEY: &str = "progress-dialog/pos";
/// Used when tTime.ini has no size; the shipped tTime.ini has the same value.
const DEFAULT_SIZE: (i32, i32) = (200, 130);
const DEFAULT_POS: (i32, i32) = (250, 250);
/// The window can be made wider but not narrower than this.
const MINIMUM_WIDTH: f32 = 200.;
/// The window actions sit this far from the top and right edges, ignoring the padding.
const WINDOW_ACTIONS_INSET: f32 = 8.;
// Window padding and spacing (as in the Qt layout).
const PAD_TOP: f32 = 12.;
const PAD_BOTTOM: f32 = 12.;
const PAD_SIDE: f32 = 8.;
const ROW_GAP: f32 = 8.;
const CONTENT_INDENT: f32 = 4.;
const CONTENT_GAP: f32 = 4.;
const BIG_ICON: f32 = 24.;
const SMALL_ICON: f32 = 16.;

pub struct CountdownWindow {
    id: u64,
    alarm: Entity<Alarm>,
    shared: Rc<Shared>,
    hwnd: isize,
    _slot: WindowSlot,
    _ticker: Task<()>,

    message: SharedString,
    duration_secs: u64,
    shown_seconds: u64,
    on_top: bool,
    visible: bool,
    closed: bool,
    frameless: bool, // the native frame is gone; the window may be fitted to its content

    tray_key: String,
    tray: Option<TrayIcon>,
    _tray_menu: Menu, // kept alive with the tray icon
    context_menu: Menu,
    menu_items: Vec<tray_icon::menu::MenuItem>, // Restore items (enabled while hidden)
    menu_icon_items: Vec<tray_icon::menu::IconMenuItem>, // Minimize items (enabled while visible)
    app_image: RgbaImage,
    tray_fraction_left: f64, // last value drawn into the tray icon, -1 = none

    resize_drag: Option<(Pixels, Pixels)>, // (mouse x, window width) when Ctrl+drag started
    placed_pos: (i32, i32),                // where the window was put automatically; not a user choice
    placed_size: Option<(i32, i32)>,       // size after automatic fitting; not a user choice
}

impl CountdownWindow {
    pub fn open(id: u64, alarm: Entity<Alarm>, shared: Rc<Shared>, cx: &mut App) -> anyhow::Result<WindowHandle<Self>> {
        let slot = WindowSlot::acquire();
        let (pos, width, area) = initial_placement(&shared, slot.index());
        let message = alarm.read(cx).message().to_string();
        let title = if message.is_empty() { "tTime".to_string() } else { message };
        let bounds =
            Bounds::new(point(px(pos.0 as f32), px(pos.1 as f32)), size(px(width as f32), px(DEFAULT_SIZE.1 as f32)));
        let options = WindowOptions {
            window_bounds: Some(WindowBounds::Windowed(bounds)),
            titlebar: Some(TitlebarOptions {
                title: Some(title.into()),
                appears_transparent: true,
                traffic_light_position: None,
            }),
            focus: true,
            show: true,
            kind: WindowKind::Normal,
            is_movable: true,
            is_resizable: false,
            is_minimizable: true,
            display_id: display_for(area, cx),
            window_background: WindowBackgroundAppearance::Transparent,
            app_id: Some("tTime".into()),
            window_min_size: None,
            window_decorations: None,
            tabbing_identifier: None,
        };
        let handle =
            cx.open_window(options, |window, cx| cx.new(|cx| Self::new(id, alarm, shared, slot, pos, window, cx)))?;
        Ok(handle)
    }

    fn new(
        id: u64,
        alarm: Entity<Alarm>,
        shared: Rc<Shared>,
        slot: WindowSlot,
        placed_pos: (i32, i32),
        window: &mut Window,
        cx: &mut Context<Self>,
    ) -> Self {
        let hwnd = platform::native_handle(window);
        platform::square_corners(hwnd);
        // Outside of this update: it makes gpui handle resize messages synchronously.
        // The size is fitted to the content only afterwards (a resize queued before would
        // still include the old frame).
        cx.spawn(async move |this, cx| {
            platform::make_frameless(hwnd);
            let _ = this.update(cx, |view, cx| {
                view.frameless = true;
                cx.notify();
            });
        })
        .detach();
        let theme = &shared.config.theme;
        let app_image = themed_app_image(theme);
        let tray_key = tray::new_key("alarm");
        let restart_icon = style::decode_png(style::RESTART_PNG);
        let minimize_icon = style::decode_png(style::MINIMIZE_PNG);
        let quit_icon = style::decode_png(style::QUIT_PNG);

        // Context menu (right click): the window's actions, as in the Qt version.
        let ctx_minimize = tray::icon_item(&tray_key, "minimize", "Mi&nimize", &minimize_icon);
        let ctx_restore = tray::item(&tray_key, "restore", "&Restore", false);
        let context_menu = tray::menu(&[
            &tray::icon_item(&tray_key, "restart", "Restart", &restart_icon),
            &ctx_minimize,
            &ctx_restore,
            &tray::item(&tray_key, "new", "New Alarm", true),
            &tray::icon_item(&tray_key, "quit", "&Quit", &quit_icon),
        ]);
        let tray_minimize = tray::icon_item(&tray_key, "minimize", "Mi&nimize", &minimize_icon);
        let tray_restore = tray::item(&tray_key, "restore", "&Restore", false);
        let tray_menu = tray::menu(&[
            &tray_minimize,
            &tray_restore,
            &tray::separator(),
            &tray::item(&tray_key, "new", "New Alarm", true),
            &tray::icon_item(&tray_key, "quit", "&Quit", &quit_icon),
        ]);

        let tooltip = window_tooltip(alarm.read(cx).message(), alarm.read(cx).remaining_seconds());
        let tray = tray::create_tray_icon(&tray_key, tray::tray_icon_image(&app_image), &tooltip, &tray_menu);
        platform::set_window_icon(hwnd, &app_image);

        let handle = window.window_handle();
        let weak = cx.entity().downgrade();
        tray::register(cx, &tray_key, move |event, cx| {
            let weak = weak.clone();
            let _ = handle.update(cx, |_, window, cx| {
                if let Some(view) = weak.upgrade() {
                    view.update(cx, |view, cx| view.on_tray_event(event, window, cx));
                }
            });
        });

        // Alt+F4 & co. hide to the tray instead of silently killing the alarm.
        let weak = cx.entity().downgrade();
        window.on_window_should_close(cx, move |window, cx| {
            if let Some(view) = weak.upgrade() {
                view.update(cx, |view, cx| view.hide(window, cx));
            }
            false
        });

        // The alarm runs 4 ticks a second for accuracy; the view only redraws when
        // the displayed second changes.
        let ticker = cx.spawn(async move |this, cx| {
            loop {
                cx.background_executor().timer(Duration::from_millis(250)).await;
                if this.update(cx, |view, cx| view.tick(cx)).is_err() {
                    break;
                }
            }
        });

        let message: SharedString = alarm.read(cx).message().to_string().into();
        let duration_secs = alarm.read(cx).duration().as_secs();
        let mut view = Self {
            id,
            message,
            duration_secs,
            shown_seconds: alarm.read(cx).remaining_seconds(),
            alarm,
            shared,
            hwnd,
            _slot: slot,
            _ticker: ticker,
            on_top: false,
            visible: true,
            closed: false,
            frameless: !cfg!(windows),
            tray_key,
            tray,
            _tray_menu: tray_menu,
            context_menu,
            menu_items: vec![ctx_restore, tray_restore],
            menu_icon_items: vec![ctx_minimize, tray_minimize],
            app_image,
            tray_fraction_left: -1.,
            resize_drag: None,
            placed_pos,
            placed_size: None,
        };
        view.update_tray_icon();
        view
    }

    fn tick(&mut self, cx: &mut Context<Self>) {
        let finished = self.alarm.update(cx, |alarm, _| alarm.tick());
        let seconds = self.alarm.read(cx).remaining_seconds();
        if seconds != self.shown_seconds {
            self.shown_seconds = seconds;
            self.time_left_changed(cx);
        }
        if finished {
            manager::alarm_finished(self.id, self.alarm.clone(), cx);
        }
    }

    fn time_left_changed(&mut self, cx: &mut Context<Self>) {
        self.update_tray_icon();
        if let Some(tray) = &self.tray {
            let alarm = self.alarm.read(cx);
            let _ = tray.set_tooltip(Some(window_tooltip(alarm.message(), self.shown_seconds)));
        }
        cx.notify();
    }

    /// Redraws the tray (and taskbar) icon: the elapsed part filled from the bottom.
    fn update_tray_icon(&mut self) {
        let total = self.alarm_duration_secs();
        let fraction_left = if total > 0 { self.shown_seconds as f64 / total as f64 } else { 0. };
        // Only redraw on noticeable change (or on reset back to full).
        let last = self.tray_fraction_left;
        if last >= 0. && fraction_left < last && last - fraction_left < 0.05 {
            return;
        }
        self.tray_fraction_left = fraction_left;
        let image = progress_icon(&self.app_image, fraction_left);
        if let Some(tray) = &self.tray {
            let _ = tray.set_icon(Some(tray::tray_icon_image(&image)));
        }
        platform::set_window_icon(self.hwnd, &image);
    }

    fn alarm_duration_secs(&self) -> u64 {
        self.duration_secs
    }

    fn on_tray_event(&mut self, event: TrayEvent, window: &mut Window, cx: &mut Context<Self>) {
        match event {
            TrayEvent::DoubleClick => {
                if self.visible && platform::is_visible(self.hwnd) {
                    self.hide(window, cx);
                } else {
                    self.restore(window, cx);
                }
            }
            TrayEvent::Click => {}
            TrayEvent::Menu(action) => self.run_action(&action, window, cx),
        }
    }

    fn run_action(&mut self, action: &str, window: &mut Window, cx: &mut Context<Self>) {
        match action {
            "restart" => {
                self.alarm.update(cx, |alarm, _| alarm.restart());
                self.state_changed(cx);
            }
            "minimize" => self.hide(window, cx),
            "restore" => self.restore(window, cx),
            "new" => manager::show_new_alarm_window(None, cx),
            "quit" => manager::remove_alarm(self.id, cx),
            _ => {}
        }
    }

    fn state_changed(&mut self, cx: &mut Context<Self>) {
        let seconds = self.alarm.read(cx).remaining_seconds();
        self.shown_seconds = seconds;
        self.time_left_changed(cx);
    }

    fn toggle_play_pause(&mut self, cx: &mut Context<Self>) {
        self.alarm.update(
            cx,
            |alarm, _| {
                if alarm.state() == AlarmState::Running { alarm.pause() } else { alarm.start() }
            },
        );
        self.state_changed(cx);
    }

    fn set_visible_state(&mut self, visible: bool) {
        self.visible = visible;
        for item in &self.menu_items {
            item.set_enabled(!visible);
        }
        for item in &self.menu_icon_items {
            item.set_enabled(visible);
        }
    }

    fn hide(&mut self, window: &mut Window, _cx: &mut Context<Self>) {
        if platform::native_handle(window) != 0 && cfg!(windows) {
            platform::hide(self.hwnd);
        } else {
            window.minimize_window();
        }
        self.set_visible_state(false);
    }

    fn restore(&mut self, window: &mut Window, _cx: &mut Context<Self>) {
        platform::show_and_activate(self.hwnd);
        window.activate_window();
        self.set_visible_state(true);
    }

    fn set_stay_on_top(&mut self, on_top: bool, cx: &mut Context<Self>) {
        self.on_top = on_top;
        platform::set_topmost(self.hwnd, on_top);
        cx.notify();
    }

    /// Called by the manager just before the window goes away (quit, or the
    /// "Time's up" window replaces it): saves the geometry, removes the tray icon.
    pub fn close(&mut self, window: &mut Window, cx: &mut Context<Self>) {
        if self.closed {
            return;
        }
        self.closed = true;
        self.save_settings(window);
        self.tray = None;
        tray::unregister(cx, &self.tray_key);
        platform::forget_window(self.hwnd);
    }

    /// Only remembers geometry the user chose: automatic adjustments (fitting
    /// onto the screen, cascade offset) are not saved, so the original position
    /// comes back when e.g. the monitor is reconnected.
    fn save_settings(&self, window: &Window) {
        let bounds = window.bounds();
        let pos = (f32::from(bounds.origin.x).round() as i32, f32::from(bounds.origin.y).round() as i32);
        let size = (f32::from(bounds.size.width).round() as i32, f32::from(bounds.size.height).round() as i32);
        let mut settings = self.shared.settings.borrow_mut();
        if Some(size) != self.placed_size {
            settings.set_value(SIZE_KEY, &format_qt_size(size));
        }
        if pos != self.placed_pos {
            settings.set_value(POS_KEY, &format_qt_point(pos));
        }
    }

    fn on_mouse_down(&mut self, event: &MouseDownEvent, window: &mut Window, _cx: &mut Context<Self>) {
        if event.modifiers.control {
            self.resize_drag = Some((event.position.x, window.bounds().size.width));
        } else {
            platform::start_system_move(self.hwnd);
        }
    }

    fn on_mouse_move(&mut self, event: &MouseMoveEvent, window: &mut Window, _cx: &mut Context<Self>) {
        let Some((start_x, start_width)) = self.resize_drag else {
            return;
        };
        if event.pressed_button != Some(MouseButton::Left) {
            self.resize_drag = None;
            return;
        }
        let width = (start_width + event.position.x - start_x).max(px(MINIMUM_WIDTH)).round();
        let current = window.bounds().size;
        if width != current.width {
            window.resize(size(width, current.height));
        }
    }

    /// Height the content needs at `width` (the window's height always follows it).
    fn content_height(&self, width: Pixels, metrics: &TextMetrics, window: &Window) -> Pixels {
        let alarm_message = self.message();
        let message_height = if alarm_message.is_empty() {
            px(0.)
        } else {
            let wrap_width = width - px(2. * PAD_SIDE + CONTENT_INDENT);
            let run = TextRun {
                len: alarm_message.len(),
                font: metrics.message_font.clone(),
                color: gpui::white(),
                background_color: None,
                underline: None,
                strikethrough: None,
            };
            let lines = window
                .text_system()
                .shape_text(alarm_message.clone(), metrics.message_size, &[run], Some(wrap_width), None)
                .map(|lines| lines.iter().map(|line| line.wrap_boundaries().len() + 1).sum::<usize>())
                .unwrap_or(1);
            metrics.message_line_height * lines as f32 + px(CONTENT_GAP)
        };
        px(PAD_TOP + BIG_ICON + 3. + ROW_GAP + PAD_BOTTOM) + metrics.countdown_ascent + message_height
    }

    fn message(&self) -> SharedString {
        self.message.clone()
    }
}

/// Fonts and metrics of the countdown and the message, resolved for a window.
struct TextMetrics {
    countdown_font: Font,
    countdown_size: Pixels,
    countdown_ascent: Pixels,
    countdown_descent: Pixels,
    message_font: Font,
    message_size: Pixels,
    message_line_height: Pixels,
}

impl TextMetrics {
    fn new(shared: &Shared, window: &Window) -> Self {
        let config = &shared.config;
        let ts = window.text_system();
        let countdown_font = style::font(&config.countdown_font);
        let countdown_size = style::points(config.countdown_font.point_size);
        let countdown_id = ts.resolve_font(&countdown_font);
        let message_font = style::font(&config.message_font);
        let message_size = style::points(config.message_font.point_size);
        let message_id = ts.resolve_font(&message_font);
        Self {
            countdown_ascent: ts.ascent(countdown_id, countdown_size).ceil(),
            countdown_descent: ts.descent(countdown_id, countdown_size).abs(),
            countdown_font,
            countdown_size,
            message_line_height: (ts.ascent(message_id, message_size) + ts.descent(message_id, message_size).abs())
                .ceil(),
            message_font,
            message_size,
        }
    }
}

impl Render for CountdownWindow {
    fn render(&mut self, window: &mut Window, cx: &mut Context<Self>) -> impl IntoElement {
        let shared = self.shared.clone();
        let theme = &shared.config.theme;
        let icons = &shared.icons;
        let metrics = TextMetrics::new(&shared, window);

        // Width at least the minimum, height always that of the content.
        let viewport = window.viewport_size();
        let width = viewport.width.max(px(MINIMUM_WIDTH));
        let height = self.content_height(width, &metrics, window);
        if !self.frameless {
            // Not fitted yet (see make_frameless); the next render does it.
        } else if (viewport.height - height).abs() > px(0.5) || viewport.width < px(MINIMUM_WIDTH) {
            window.resize(size(width, height));
        } else if self.placed_size.is_none() {
            // Remember where the window was actually put (the system may adjust
            // the requested origin slightly) and its fitted size.
            let origin = window.bounds().origin;
            self.placed_pos = (f32::from(origin.x).round() as i32, f32::from(origin.y).round() as i32);
            self.placed_size = Some((f32::from(width).round() as i32, f32::from(height).round() as i32));
        }

        let running = self.alarm.read(cx).state() == AlarmState::Running;
        let message = self.message();

        // The countdown: digits in equal-width slots (so the time does not jiggle),
        // with the font's letter spacing.
        let text = format_duration(self.shown_seconds);
        let ts = window.text_system().clone();
        let countdown_id = ts.resolve_font(&metrics.countdown_font);
        let layout = layout_fixed_digits(
            &text,
            |c| ts.advance(countdown_id, metrics.countdown_size, c).map(|s| f32::from(s.width)).unwrap_or(0.),
            shared.config.countdown_font.letter_spacing_percent / 100.,
            0.,
        );
        let countdown_font = metrics.countdown_font.clone();
        let countdown_size = metrics.countdown_size;
        let line_height = metrics.countdown_ascent + metrics.countdown_descent;
        let glyph_x = layout.glyph_x.clone();
        let countdown = canvas(
            |_, _, _| {},
            move |bounds, _, window, cx| {
                for (ch, x) in text.chars().zip(glyph_x.iter()) {
                    let glyph: SharedString = ch.to_string().into();
                    let run = TextRun {
                        len: glyph.len(),
                        font: countdown_font.clone(),
                        color: gpui::white(),
                        background_color: None,
                        underline: None,
                        strikethrough: None,
                    };
                    let line = window.text_system().shape_line(glyph, countdown_size, &[run], None);
                    let _ = line.paint(point(bounds.origin.x + px(*x), bounds.origin.y), line_height, window, cx);
                }
            },
        )
        .w(px(layout.width.ceil()))
        .h(metrics.countdown_ascent)
        .flex_none();

        let stop_propagation = |_: &MouseDownEvent, _: &mut Window, cx: &mut App| cx.stop_propagation();

        div()
            .id("countdown-window")
            .relative()
            .size_full()
            .overflow_hidden()
            .on_mouse_down(MouseButton::Left, cx.listener(Self::on_mouse_down))
            .on_mouse_move(cx.listener(Self::on_mouse_move))
            .on_mouse_up(MouseButton::Left, cx.listener(|this, _, _, _| this.resize_drag = None))
            .on_mouse_down(
                MouseButton::Right,
                cx.listener(|this, _, _, cx| tray::show_context_menu(cx, this.context_menu.clone(), this.hwnd)),
            )
            .child(style::panel(
                theme,
                PanelStyle {
                    corner_radius: 0.,
                    side_edge_alpha: 200,
                    side_inset_alpha: 150,
                    side_light_alpha: 80,
                    light_band_left: true,
                },
                size(width, height),
            ))
            .child(
                div()
                    .absolute()
                    .top_0()
                    .left_0()
                    .size_full()
                    .flex()
                    .flex_col()
                    .gap(px(ROW_GAP))
                    .pt(px(PAD_TOP))
                    .pb(px(PAD_BOTTOM))
                    .px(px(PAD_SIDE))
                    .child(
                        div()
                            .flex()
                            .flex_row()
                            .child(
                                style::tool_button(
                                    "play-pause",
                                    if running { icons.pause.clone() } else { icons.play.clone() },
                                    BIG_ICON,
                                    theme,
                                    false,
                                )
                                .tooltip(Tooltip::text(if running { "Pause" } else { "Start" }))
                                .on_mouse_down(MouseButton::Left, stop_propagation)
                                .on_click(cx.listener(|this, _, _, cx| this.toggle_play_pause(cx))),
                            )
                            .child(
                                style::tool_button("stop", icons.stop.clone(), BIG_ICON, theme, false)
                                    .on_mouse_down(MouseButton::Left, stop_propagation)
                                    .on_click(cx.listener(|this, _, _, cx| {
                                        this.alarm.update(cx, |alarm, _| alarm.stop());
                                        this.state_changed(cx);
                                    })),
                            )
                            .child(
                                style::tool_button("restart", icons.restart.clone(), BIG_ICON, theme, false)
                                    .tooltip(Tooltip::text("Restart"))
                                    .on_mouse_down(MouseButton::Left, stop_propagation)
                                    .on_click(
                                        cx.listener(|this, _, window, cx| this.run_action("restart", window, cx)),
                                    ),
                            ),
                    )
                    .child(div().flex().flex_col().gap(px(CONTENT_GAP)).pl(px(CONTENT_INDENT)).child(countdown).when(
                        !message.is_empty(),
                        |this| {
                            this.child(
                                div()
                                    .w_full()
                                    .font(metrics.message_font.clone())
                                    .text_size(metrics.message_size)
                                    .line_height(metrics.message_line_height)
                                    .text_color(rgb(0xf0f0f0))
                                    .child(message.clone()),
                            )
                        },
                    )),
            )
            // Window actions: pinned to the top-right corner, outside the padding.
            .child(
                div()
                    .absolute()
                    .top(px(WINDOW_ACTIONS_INSET))
                    .right(px(WINDOW_ACTIONS_INSET))
                    .flex()
                    .flex_row()
                    .child(
                        style::tool_button("stay-on-top", icons.anchor.clone(), SMALL_ICON, theme, self.on_top)
                            .tooltip(Tooltip::text("Always on top"))
                            .on_mouse_down(MouseButton::Left, stop_propagation)
                            .on_click(cx.listener(|this, _, _, cx| this.set_stay_on_top(!this.on_top, cx))),
                    )
                    .child(
                        style::tool_button("minimize", icons.minimize.clone(), SMALL_ICON, theme, false)
                            .tooltip(Tooltip::text("Minimize"))
                            .on_mouse_down(MouseButton::Left, stop_propagation)
                            .on_click(cx.listener(|this, _, window, cx| this.hide(window, cx))),
                    )
                    .child(
                        style::tool_button("quit", icons.quit.clone(), SMALL_ICON, theme, false)
                            .tooltip(Tooltip::text("Quit"))
                            .on_mouse_down(MouseButton::Left, stop_propagation)
                            .on_click(cx.listener(|this, _, _, cx| manager::remove_alarm(this.id, cx))),
                    ),
            )
    }
}

fn window_tooltip(message: &str, seconds: u64) -> String {
    let time = format_duration(seconds);
    if message.is_empty() { time } else { format!("{time} - {message}") }
}

/// The app icon, faint, with the elapsed part (from the bottom up to
/// `fraction_left` of the height) drawn opaque and a white line at the edge.
fn progress_icon(source: &RgbaImage, fraction_left: f64) -> RgbaImage {
    let (w, h) = source.dimensions();
    let y = h as f64 * fraction_left;
    let mut image = source.clone();
    for (_, row, pixel) in image.enumerate_pixels_mut() {
        // Pixel rows whose centre is below the edge belong to the elapsed part.
        if (row as f64 + 0.5) <= y {
            pixel.0[3] = ((pixel.0[3] as u32 * 64 + 127) / 255) as u8; // 25 % opacity
        }
    }
    // A 5 px white pen across the image at the edge.
    let first = (y - 2.5).round().max(0.) as u32;
    let last = ((y + 2.5).round() as u32).min(h);
    for row in first..last {
        for x in 0..w {
            image.put_pixel(x, row, image::Rgba([255, 255, 255, 255]));
        }
    }
    image
}

/// Saved (or default) position and size, made fully visible on a screen and
/// offset if other tTime windows are open (cascade).
fn initial_placement(shared: &Shared, slot: i32) -> ((i32, i32), i32, Rect) {
    let settings = shared.settings.borrow();
    let pos = settings.value(POS_KEY).and_then(|v| parse_qt_point(&v)).unwrap_or(DEFAULT_POS);
    let (w, h) = settings.value(SIZE_KEY).and_then(|v| parse_qt_size(&v)).unwrap_or(DEFAULT_SIZE);
    let saved = if w > 0 && h > 0 {
        Rect { x: pos.0, y: pos.1, width: w, height: h }
    } else {
        Rect { x: DEFAULT_POS.0, y: DEFAULT_POS.1, width: DEFAULT_SIZE.0, height: DEFAULT_SIZE.1 }
    };
    let mut areas = platform::available_areas();
    if areas.is_empty() {
        areas.push(Rect { x: 0, y: 0, width: 1024, height: 768 }); // no screen info at all; be defensive
    }
    let visible = fit_into_visible_area(&saved, &areas);
    let width = visible.geometry.width.max(MINIMUM_WIDTH as i32);
    let pos = cascade_position(
        (visible.geometry.x, visible.geometry.y),
        (width, visible.geometry.height),
        &visible.area,
        slot,
        32,
    );
    (pos, width, visible.area)
}

/// The gpui display that shows `area`, so the window opens on the right monitor.
pub fn display_for(area: Rect, cx: &App) -> Option<gpui::DisplayId> {
    let center = point(px(area.x as f32 + area.width as f32 / 2.), px(area.y as f32 + area.height as f32 / 2.));
    cx.displays().into_iter().find(|display| display.bounds().contains(&center)).map(|display| display.id())
}
