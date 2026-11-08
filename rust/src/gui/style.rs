//! Look of tTime's windows: the themed gradient panel of the frameless windows,
//! their tool buttons, Windows 11 style push buttons, tooltips and images.

use std::sync::Arc;

use gpui::{
    AnyView, App, Context, Div, Hsla, Image, ImageFormat, ImageSource, IntoElement, ParentElement, Pixels, Render,
    RenderImage, Rgba, SharedString, Size, Stateful, Styled, Window, div, img, linear_color_stop, linear_gradient,
    prelude::*, px, rgb,
};
use image::RgbaImage;
use ttime::theme::{Rgb, Theme};

#[cfg(target_os = "macos")]
pub const UI_FONT: &str = ".SystemUIFont";
#[cfg(not(target_os = "macos"))]
pub const UI_FONT: &str = "Segoe UI";
/// The system UI font size (9 pt at 96 dpi).
pub const UI_FONT_SIZE: f32 = 12.;
pub const UI_LINE_HEIGHT: f32 = 16.;
pub const CONTROL_HEIGHT: f32 = 23.;

/// Points to gpui's logical pixels (Qt point sizes assume 96 dpi = 100 %).
pub fn points(pt: i32) -> Pixels {
    px(pt as f32 * 96. / 72.)
}

pub fn color(c: Rgb, alpha: u8) -> Hsla {
    Rgba { r: c.0 as f32 / 255., g: c.1 as f32 / 255., b: c.2 as f32 / 255., a: alpha as f32 / 255. }.into()
}

pub fn rgba8(r: u8, g: u8, b: u8, a: f32) -> Hsla {
    Rgba { r: r as f32 / 255., g: g as f32 / 255., b: b as f32 / 255., a }.into()
}

/// Windows' default accent color (focus line of the text field).
pub fn accent() -> Hsla {
    match super::platform::accent_color() {
        Some((r, g, b)) => rgba8(r, g, b, 1.),
        None => rgb(0x005fb8).into(),
    }
}

// ---------------------------------------------------------------------------
// Panel

/// How a frameless window paints its themed background: a top-to-bottom
/// gradient (dark edges, light centre) overlaid with a left-to-right one.
#[derive(Clone, Copy)]
pub struct PanelStyle {
    pub corner_radius: f32,
    /// Opacity of the left/right gradient at the edges, the inset band and the light band.
    pub side_edge_alpha: u8,
    pub side_inset_alpha: u8,
    pub side_light_alpha: u8,
    /// Light band of the left/right gradient: centred (false) or towards the left
    /// at 35 %, with the inset color again at 55 % (true; the countdown window).
    pub light_band_left: bool,
}

/// Width of the dark-to-inset transition, in pixels.
const PANEL_INSET: f32 = 20.;

/// One gradient made of two-stop segments (gpui gradients have two stops):
/// `stops` are (position 0..1, color). Segment edges are snapped to whole pixels.
fn gradient_segments(stops: &[(f32, Hsla)], length: f32, vertical: bool, radius: f32) -> Vec<Div> {
    let snap = |f: f32| (f * length).round();
    let last = stops.len() - 2;
    stops
        .windows(2)
        .enumerate()
        .filter_map(|(i, pair)| {
            let (start, end) = (snap(pair[0].0), snap(pair[1].0));
            if end <= start {
                return None;
            }
            let angle = if vertical { 180. } else { 90. };
            let segment = div().absolute().bg(linear_gradient(
                angle,
                linear_color_stop(pair[0].1, 0.),
                linear_color_stop(pair[1].1, 1.),
            ));
            let segment = if vertical {
                segment.left_0().right_0().top(px(start)).h(px(end - start))
            } else {
                segment.top_0().bottom_0().left(px(start)).w(px(end - start))
            };
            let segment = match (vertical, i) {
                (true, 0) => segment.rounded_t(px(radius)),
                (true, i) if i == last => segment.rounded_b(px(radius)),
                (false, 0) => segment.rounded_l(px(radius)),
                (false, i) if i == last => segment.rounded_r(px(radius)),
                _ => segment,
            };
            Some(segment)
        })
        .collect()
}

/// The panel filling a window of `size`, to be put behind the window's content.
pub fn panel(theme: &Theme, style: PanelStyle, size: Size<Pixels>) -> Div {
    let w = f32::from(size.width).max(1.);
    let h = f32::from(size.height).max(1.);
    let top_to_bottom = [
        (0.0, color(theme.dark, 235)),
        ((PANEL_INSET / h).min(0.49), color(theme.mid, 190)),
        (0.5, color(theme.light, 150)),
        ((1.0 - PANEL_INSET / h).max(0.51), color(theme.mid, 190)),
        (1.0, color(theme.dark, 235)),
    ];
    let edge = color(theme.dark, style.side_edge_alpha);
    let inset = color(theme.mid, style.side_inset_alpha);
    let light = color(theme.light, style.side_light_alpha);
    let left_to_right: Vec<(f32, Hsla)> = if style.light_band_left {
        vec![(0.0, edge), ((PANEL_INSET / w).min(0.34), inset), (0.35, light), (0.55, inset), (1.0, edge)]
    } else {
        vec![
            (0.0, edge),
            ((PANEL_INSET / w).min(0.49), inset),
            (0.5, light),
            ((1.0 - PANEL_INSET / w).max(0.51), inset),
            (1.0, edge),
        ]
    };
    let radius = px(style.corner_radius);
    div()
        .absolute()
        .top_0()
        .left_0()
        .size_full()
        .children(gradient_segments(&top_to_bottom, h, true, style.corner_radius))
        .children(gradient_segments(&left_to_right, w, false, style.corner_radius))
        // The two outlines of the original (a faint dark one, then a light one).
        .child(div().absolute().size_full().rounded(radius).border_1().border_color(rgba8(0, 0, 0, 32. / 255.)))
        .child(div().absolute().size_full().rounded(radius).border_1().border_color(rgba8(220, 220, 220, 100. / 255.)))
}

// ---------------------------------------------------------------------------
// Tool buttons (the icon buttons of the countdown window)

/// An icon-only, flat button; highlighted in the theme's colors when hovered,
/// pressed or `checked`. Fixed size, so the highlight border never moves the layout.
pub fn tool_button(id: &'static str, icon: Arc<Image>, icon_size: f32, theme: &Theme, checked: bool) -> Stateful<Div> {
    let highlight = color(theme.highlight, 150);
    let border = color(theme.highlight_border, 220);
    // Qt's tool button: icon + 3 px (+1 px left padding of tTime's style sheet).
    div()
        .id(id)
        .flex_none()
        .flex()
        .items_center()
        .justify_center()
        .w(px(icon_size + 4.))
        .h(px(icon_size + 3.))
        .pl(px(1.))
        .border_1()
        .border_color(if checked { border } else { gpui::transparent_black() })
        .when(checked, |s| s.bg(highlight))
        .hover(move |s| s.bg(highlight).border_color(border))
        .active(move |s| s.bg(highlight).border_color(border))
        .child(img(ImageSource::Image(icon)).size(px(icon_size)).flex_none())
}

// ---------------------------------------------------------------------------
// Push buttons (Windows 11 style, as Qt draws them)

pub fn push_button(id: impl Into<gpui::ElementId>, label: impl Into<SharedString>) -> Stateful<Div> {
    div()
        .id(id)
        .flex()
        .flex_none()
        .items_center()
        .justify_center()
        .h(px(CONTROL_HEIGHT))
        .px(px(11.))
        .rounded(px(4.))
        .border_1()
        .border_color(rgba8(0, 0, 0, 0.0578))
        .bg(rgba8(255, 255, 255, 0.7))
        .hover(|s| s.bg(rgba8(249, 249, 249, 0.5)))
        .active(|s| s.bg(rgba8(249, 249, 249, 0.3)).text_color(rgba8(0, 0, 0, 0.6063)))
        .font_family(UI_FONT)
        .text_size(px(UI_FONT_SIZE))
        .line_height(px(UI_LINE_HEIGHT))
        .text_color(rgba8(0, 0, 0, 0.8956))
        .child(label.into())
}

// ---------------------------------------------------------------------------
// Tooltips

pub struct Tooltip {
    text: SharedString,
}

impl Tooltip {
    pub fn text(text: impl Into<SharedString>) -> impl Fn(&mut Window, &mut App) -> AnyView + 'static {
        let text = text.into();
        move |_window, cx| cx.new(|_| Tooltip { text: text.clone() }).into()
    }
}

impl Render for Tooltip {
    fn render(&mut self, _window: &mut Window, _cx: &mut Context<Self>) -> impl IntoElement {
        // gpui places the tooltip relative to the mouse; keep it compact so it fits small windows.
        div().pl(px(4.)).pt(px(14.)).child(
            div()
                .px(px(6.))
                .py(px(2.))
                .rounded(px(4.))
                .border_1()
                .border_color(rgba8(0, 0, 0, 0.15))
                .bg(rgb(0xf9f9f9))
                .font_family(UI_FONT)
                .text_size(px(UI_FONT_SIZE))
                .line_height(px(UI_LINE_HEIGHT))
                .text_color(rgb(0x1b1b1b))
                .whitespace_nowrap()
                .child(self.text.clone()),
        )
    }
}

// ---------------------------------------------------------------------------
// Images

pub fn png(bytes: &'static [u8]) -> Arc<Image> {
    Arc::new(Image::from_bytes(ImageFormat::Png, bytes.to_vec()))
}

pub struct Icons {
    pub play: Arc<Image>,
    pub pause: Arc<Image>,
    pub stop: Arc<Image>,
    pub restart: Arc<Image>,
    pub anchor: Arc<Image>,
    pub minimize: Arc<Image>,
    pub quit: Arc<Image>,
}

macro_rules! resource {
    ($path:literal) => {
        include_bytes!(concat!("../../../src/Resources/", $path))
    };
}

pub const PLAY_PNG: &[u8] = resource!("icons01/play_matched_16.png");
pub const PAUSE_PNG: &[u8] = resource!("icons01/video_pause_16.png");
pub const STOP_PNG: &[u8] = resource!("icons01/video_stop_16.png");
pub const RESTART_PNG: &[u8] = resource!("icons01/reload_16.png");
pub const ANCHOR_PNG: &[u8] = resource!("icons01/flag1_16.png");
pub const MINIMIZE_PNG: &[u8] = resource!("icons01/arrow_down_16.png");
pub const QUIT_PNG: &[u8] = resource!("icons01/delete_16.png");

impl Icons {
    pub fn load() -> Self {
        Self {
            play: png(PLAY_PNG),
            pause: png(PAUSE_PNG),
            stop: png(STOP_PNG),
            restart: png(RESTART_PNG),
            anchor: png(ANCHOR_PNG),
            minimize: png(MINIMIZE_PNG),
            quit: png(QUIT_PNG),
        }
    }
}

pub fn decode_png(bytes: &[u8]) -> RgbaImage {
    image::load_from_memory_with_format(bytes, image::ImageFormat::Png)
        .map(|image| image.to_rgba8())
        .unwrap_or_else(|_| RgbaImage::new(1, 1))
}

/// An RGBA image as a gpui render image (gpui wants BGRA).
pub fn render_image(image: &RgbaImage) -> Arc<RenderImage> {
    let mut bgra = image.clone();
    for px in bgra.pixels_mut() {
        px.0.swap(0, 2);
    }
    Arc::new(RenderImage::new(vec![image::Frame::new(bgra)]))
}

/// A configured font as a gpui font (family resolved at startup, exact weight).
pub fn font(spec: &ttime::config::FontSpec) -> gpui::Font {
    gpui::Font {
        family: spec.family.into(),
        features: gpui::FontFeatures::default(),
        fallbacks: None,
        weight: gpui::FontWeight(spec.weight as f32),
        style: gpui::FontStyle::Normal,
    }
}

/// Family of the bundled Sansation font (used for "Time's Up").
pub fn sansation_family() -> &'static str {
    "Sansation"
}
