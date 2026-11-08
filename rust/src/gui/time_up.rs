//! "Time's up" window: plays the alarm sound, rings the bell in the tray and
//! taskbar icon and lets the tree sway in a light breeze until dismissed.

use std::collections::VecDeque;
use std::f64::consts::PI;
use std::rc::Rc;
use std::sync::Arc;
use std::time::{Duration, Instant};

use gpui::{
    App, Bounds, Context, IntoElement, ParentElement, Render, RenderImage, SharedString, Styled, Task, TitlebarOptions,
    Window, WindowBackgroundAppearance, WindowBounds, WindowHandle, WindowKind, WindowOptions, div, img, point,
    prelude::*, px, size,
};
use image::RgbaImage;
use tray_icon::TrayIcon;
use ttime::theme::{recolor_for_theme, themed_app_image, themed_bell_animation};
use ttime::tree_sway::{TreeSway, tree_image};

use super::manager::{self, Shared};
use super::platform;
use super::sound::AlarmSound;
use super::style::{self, PanelStyle};
use super::tray::{self, TrayEvent};

const WINDOW_SIZE: (f32, f32) = (421., 196.);
const MARGIN: f32 = 11.; // Qt's top-level layout margin
const TREE_FRAME_INTERVAL: Duration = Duration::from_millis(50);
const BELL_FRAME_INTERVAL: Duration = Duration::from_millis(100);

pub struct TimeUpWindow {
    id: u64,
    shared: Rc<Shared>,
    message: SharedString,
    hwnd: isize,
    dismissed: bool,

    sound: Option<AlarmSound>,
    tray_key: String,
    tray: Option<TrayIcon>,
    bell_frames: Vec<(tray_icon::Icon, RgbaImage)>,
    bell_frame: usize,
    _bell_task: Option<Task<()>>,

    tree_sway: Option<TreeSway>,
    tree_clock: Instant,
    tree_image: Arc<RenderImage>,
    retired_images: VecDeque<Arc<RenderImage>>, // shown recently; dropped from the atlas a little later
    _tree_task: Option<Task<()>>,
}

impl TimeUpWindow {
    pub fn open(id: u64, message: String, shared: Rc<Shared>, cx: &mut App) -> anyhow::Result<WindowHandle<Self>> {
        let window_size = size(px(WINDOW_SIZE.0), px(WINDOW_SIZE.1));
        // Centred on the primary screen's usable area, like a new Qt window.
        let bounds = match platform::primary_area() {
            Some(area) => Bounds::new(
                point(
                    px((area.x as f32 + (area.width as f32 - WINDOW_SIZE.0) / 2.).round()),
                    px((area.y as f32 + (area.height as f32 - WINDOW_SIZE.1) / 2.).round()),
                ),
                window_size,
            ),
            None => Bounds::centered(None, window_size, cx),
        };
        let title = if message.is_empty() { "Time is up".to_string() } else { message.clone() };
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
            display_id: None,
            window_background: WindowBackgroundAppearance::Transparent,
            app_id: Some("tTime".into()),
            window_min_size: None,
            window_decorations: None,
            tabbing_identifier: None,
        };
        let handle = cx.open_window(options, |window, cx| cx.new(|cx| Self::new(id, message, shared, window, cx)))?;
        let _ = handle.update(cx, |view, window, _| {
            platform::set_topmost(view.hwnd, true);
            platform::show_and_activate(view.hwnd);
            window.activate_window();
        });
        Ok(handle)
    }

    fn new(id: u64, message: String, shared: Rc<Shared>, window: &mut Window, cx: &mut Context<Self>) -> Self {
        let hwnd = platform::native_handle(window);
        platform::square_corners(hwnd); // tTime paints its own rounded corners
        // Outside of this update: it makes gpui handle resize messages synchronously.
        cx.foreground_executor().spawn(async move { platform::make_frameless(hwnd) }).detach();
        let config = &shared.config;
        let theme = &config.theme;
        let animations = config.animations;

        // The tree artwork has a translucent fill drawn for the blue theme.
        let picture = recolor_for_theme(&tree_image(), theme);
        let tree_sway = animations.then(|| TreeSway::new(&picture)).filter(TreeSway::is_valid);

        // Ringing bell: the colored background stays still, the bell swings from the
        // top of its handle and the clapper follows a little later and further.
        let app_image = themed_app_image(theme);
        let bell = themed_bell_animation(theme);
        let mut frames: Vec<RgbaImage> = Vec::new();
        if bell.is_valid() {
            const FRAMES: usize = 12; // one swing cycle (frames x timer interval)
            for i in 0..FRAMES {
                let phase = 2.0 * PI * i as f64 / FRAMES as f64;
                if let Some(frame) = bell.frame(12.0 * phase.sin(), 16.0 * (phase - 0.9).sin()) {
                    frames.push(frame);
                }
            }
        }
        if frames.is_empty() {
            // An icon without a separable bell: rock the whole icon.
            for angle in [0., 10., 20., 10., 0., -10., -20., -10.] {
                frames.push(rotate_about_center(&app_image, angle));
            }
        }
        let bell_frames: Vec<_> = frames.into_iter().map(|f| (tray::tray_icon_image(&f), f)).collect();

        let tray_key = tray::new_key("time-up");
        let tray_menu = tray::menu(&[
            &tray::item(&tray_key, "new", "&New Alarm", true),
            &tray::separator(),
            &tray::icon_item(&tray_key, "close", "&Close", &app_image),
        ]);
        let tooltip = if message.is_empty() { "Time is up".to_string() } else { message.clone() };
        let tray = tray::create_tray_icon(&tray_key, bell_frames[0].0.clone(), &tooltip, &tray_menu);
        platform::set_window_icon(hwnd, &bell_frames[0].1);

        let handle = window.window_handle();
        let weak = cx.entity().downgrade();
        tray::register(cx, &tray_key, move |event, cx| {
            let weak = weak.clone();
            let _ = handle.update(cx, |_, window, cx| {
                if let Some(view) = weak.upgrade() {
                    view.update(cx, |view, cx| match event {
                        TrayEvent::Menu(action) if action == "new" => view.request_new_alarm(cx),
                        TrayEvent::Menu(action) if action == "close" => view.dismiss(cx),
                        _ => {
                            platform::show_and_activate(view.hwnd);
                            window.activate_window();
                        }
                    });
                }
            });
        });

        // Alt+F4 behaves like the Close button (stops the sound, releases the alarm).
        let weak = cx.entity().downgrade();
        window.on_window_should_close(cx, move |_, cx| {
            if let Some(view) = weak.upgrade() {
                view.update(cx, |view, cx| view.dismiss(cx));
            }
            false
        });

        let bell_task = animations.then(|| {
            cx.spawn(async move |this, cx| {
                loop {
                    cx.background_executor().timer(BELL_FRAME_INTERVAL).await;
                    if this.update(cx, |view, _| view.advance_bell()).is_err() {
                        break;
                    }
                }
            })
        });
        let tree_task = tree_sway.is_some().then(|| {
            cx.spawn(async move |this, cx| {
                loop {
                    cx.background_executor().timer(TREE_FRAME_INTERVAL).await;
                    if this.update(cx, |view, cx| view.advance_tree(cx)).is_err() {
                        break;
                    }
                }
            })
        });

        let sound = config.alarm_sound.then(AlarmSound::play);

        Self {
            id,
            message: message.into(),
            hwnd,
            dismissed: false,
            sound,
            tray_key,
            tray,
            bell_frames,
            bell_frame: 0,
            _bell_task: bell_task,
            tree_sway,
            tree_clock: Instant::now(),
            tree_image: style::render_image(&picture),
            retired_images: VecDeque::new(),
            _tree_task: tree_task,
            shared,
        }
    }

    fn advance_bell(&mut self) {
        self.bell_frame = (self.bell_frame + 1) % self.bell_frames.len();
        let (icon, image) = &self.bell_frames[self.bell_frame];
        if let Some(tray) = &self.tray {
            let _ = tray.set_icon(Some(icon.clone()));
        }
        platform::set_window_icon(self.hwnd, image);
    }

    fn advance_tree(&mut self, cx: &mut Context<Self>) {
        // Driven by a clock, not by counting ticks, so late ticks do not slow the breeze.
        let Some(sway) = &self.tree_sway else { return };
        let seconds = self.tree_clock.elapsed().as_secs_f64();
        let frame = sway.frame(
            TreeSway::BREEZE_SWAY * TreeSway::breeze(seconds),
            2.0 * PI * seconds / TreeSway::LEAF_FLUTTER_PERIOD,
            TreeSway::LEAF_FLUTTER,
        );
        if let Some(frame) = frame {
            let previous = std::mem::replace(&mut self.tree_image, style::render_image(&frame));
            self.retired_images.push_back(previous);
            cx.notify();
        }
    }

    fn request_new_alarm(&mut self, cx: &mut Context<Self>) {
        manager::show_new_alarm_window(None, cx);
        self.dismiss(cx);
    }

    fn dismiss(&mut self, cx: &mut Context<Self>) {
        if self.dismissed {
            return;
        }
        self.dismissed = true;
        if let Some(mut sound) = self.sound.take() {
            sound.stop();
        }
        self._bell_task = None;
        self._tree_task = None;
        self.tray = None;
        platform::hide(self.hwnd);
        manager::remove_alarm(self.id, cx);
    }

    /// Called by the manager just before the window goes away.
    pub fn close(&mut self, window: &mut Window, cx: &mut Context<Self>) {
        self.sound = None;
        self._bell_task = None;
        self._tree_task = None;
        self.tray = None;
        tray::unregister(cx, &self.tray_key);
        for image in self.retired_images.drain(..) {
            let _ = window.drop_image(image);
        }
        platform::forget_window(self.hwnd);
    }
}

impl Render for TimeUpWindow {
    fn render(&mut self, window: &mut Window, cx: &mut Context<Self>) -> impl IntoElement {
        // Frames older than the one on screen are no longer referenced by the scene.
        while self.retired_images.len() > 2 {
            if let Some(image) = self.retired_images.pop_front() {
                let _ = window.drop_image(image);
            }
        }
        let config = &self.shared.config;
        let sansation = style::sansation_family();
        let mut message_font = style::font(&config.message_font);
        message_font.weight = gpui::FontWeight(config.message_font.weight as f32);
        let message_size = style::points(config.time_up_message_point_size);
        let tree_size = self.tree_image.size(0);
        let text_left = MARGIN + tree_size.width.0 as f32 + 6.;

        div()
            .id("time-up-window")
            .relative()
            .size_full()
            .child(style::panel(
                &config.theme,
                PanelStyle {
                    corner_radius: 15.,
                    side_edge_alpha: 150,
                    side_inset_alpha: 100,
                    side_light_alpha: 80,
                    light_band_left: false,
                },
                window.viewport_size(),
            ))
            .child(
                img(self.tree_image.clone())
                    .absolute()
                    .left(px(MARGIN))
                    .top(px(MARGIN))
                    .w(px(tree_size.width.0 as f32))
                    .h(px(tree_size.height.0 as f32)),
            )
            .child(
                div()
                    .absolute()
                    .left(px(text_left + 10.))
                    .top(px(MARGIN + 15.))
                    .right(px(MARGIN + 25.))
                    .flex()
                    .flex_col()
                    .gap(px(6.))
                    .text_color(gpui::white())
                    .child(
                        div()
                            .font_family(sansation)
                            .font_weight(gpui::FontWeight::LIGHT)
                            .text_size(style::points(20))
                            .line_height(style::points(20) * 1.2)
                            .whitespace_nowrap()
                            .child("Time's Up"),
                    )
                    .child(
                        div()
                            .font(message_font)
                            .text_size(message_size)
                            .line_height(message_size * 1.26)
                            .child(self.message.clone()),
                    ),
            )
            .child(
                div()
                    .absolute()
                    .right(px(MARGIN + 8.))
                    .bottom(px(MARGIN + 2.))
                    .flex()
                    .flex_row()
                    .gap(px(9.))
                    .child(
                        style::push_button("new-alarm", "New Alarm")
                            .w(px(78.))
                            .h(px(23.))
                            .on_click(cx.listener(|this, _, _, cx| this.request_new_alarm(cx))),
                    )
                    .child(
                        style::push_button("close", "Close")
                            .w(px(78.))
                            .h(px(23.))
                            .on_click(cx.listener(|this, _, _, cx| this.dismiss(cx))),
                    ),
            )
    }
}

/// `source` rotated by `degrees` about its centre (bilinear), for icons without a
/// separable bell.
fn rotate_about_center(source: &RgbaImage, degrees: f64) -> RgbaImage {
    let (w, h) = source.dimensions();
    let (cx, cy) = (w as f64 / 2., h as f64 / 2.);
    let (sin, cos) = (-degrees.to_radians()).sin_cos();
    RgbaImage::from_fn(w, h, |x, y| {
        let (dx, dy) = (x as f64 + 0.5 - cx, y as f64 + 0.5 - cy);
        let sx = cx + dx * cos - dy * sin - 0.5;
        let sy = cy + dx * sin + dy * cos - 0.5;
        image::imageops::interpolate_bilinear(source, sx as f32, sy as f32).unwrap_or(image::Rgba([0, 0, 0, 0]))
    })
}
