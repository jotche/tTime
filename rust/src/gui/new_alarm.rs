//! Small input window where the user types e.g. "5m green tea".

use std::rc::Rc;

use gpui::{
    App, Bounds, Context, Entity, Focusable, IntoElement, ParentElement, Render, Styled, TitlebarOptions, Window,
    WindowBounds, WindowHandle, WindowKind, WindowOptions, div, prelude::*, px, rgb, size,
};
use ttime::alarm_spec::parse_alarm_spec;
use ttime::theme::themed_app_image;

use super::manager::{self, Shared};
use super::platform;
use super::style;
use super::text_input::{TextInput, TextInputEvent};

const USAGE_HINT: &str = "Enter a duration and an optional message, e.g.\n  5m green tea\n  1h 30m meeting\n  90s";
const WIDTH: f32 = 350.;
const HEIGHT: f32 = 52.;
const HINT_HEIGHT: f32 = 76.;

pub struct NewAlarmWindow {
    input: Entity<TextInput>,
    show_hint: bool,
}

impl NewAlarmWindow {
    pub fn open(shared: Rc<Shared>, text: String, cx: &mut App) -> anyhow::Result<WindowHandle<Self>> {
        let bounds = Bounds::centered(None, size(px(WIDTH), px(HEIGHT)), cx);
        let options = WindowOptions {
            window_bounds: Some(WindowBounds::Windowed(bounds)),
            titlebar: Some(TitlebarOptions {
                title: Some("New alarm".into()),
                appears_transparent: false,
                traffic_light_position: None,
            }),
            focus: true,
            show: true,
            kind: WindowKind::Normal,
            is_movable: true,
            is_resizable: true,
            is_minimizable: true,
            display_id: None,
            window_background: gpui::WindowBackgroundAppearance::Opaque,
            app_id: Some("tTime".into()),
            window_min_size: Some(size(px(WIDTH), px(HEIGHT))),
            window_decorations: None,
            tabbing_identifier: None,
        };
        let handle = cx.open_window(options, |window, cx| {
            // Windows shows a window's own icon on the taskbar: the themed one.
            platform::set_window_icon(platform::native_handle(window), &themed_app_image(&shared.config.theme));
            let input = cx.new(|cx| TextInput::new("e.g. 5m green tea", cx));
            cx.new(|cx| {
                cx.subscribe_in(&input, window, |this: &mut Self, _, event, window, cx| match event {
                    TextInputEvent::Edited => this.set_input_valid(true, window, cx),
                    TextInputEvent::Submit => this.submit(window, cx),
                })
                .detach();
                window.on_window_should_close(cx, |_, cx| {
                    manager::new_alarm_window_closed(cx);
                    true
                });
                let mut view = Self { input, show_hint: false };
                view.set_text(&text, cx);
                window.focus(&view.input.focus_handle(cx));
                view
            })
        })?;
        Ok(handle)
    }

    /// Pre-fills the input (e.g. with an unparsable command line) and
    /// highlights it as invalid if it cannot be parsed.
    pub fn set_text(&mut self, text: &str, cx: &mut Context<Self>) {
        let valid = text.trim().is_empty() || parse_alarm_spec(text).is_some();
        self.input.update(cx, |input, cx| {
            input.set_text(text, cx);
            input.set_invalid(!valid, cx);
        });
    }

    pub fn show(&mut self, window: &mut Window, cx: &mut Context<Self>) {
        platform::show_and_activate(platform::native_handle(window));
        window.activate_window();
        window.focus(&self.input.focus_handle(cx));
    }

    fn submit(&mut self, window: &mut Window, cx: &mut Context<Self>) {
        let text = self.input.read(cx).text().to_string();
        match parse_alarm_spec(&text) {
            Some(spec) => {
                self.input.update(cx, |input, cx| input.set_text("", cx));
                manager::add_alarm(spec, cx);
            }
            None => {
                self.input.update(cx, |input, cx| input.set_invalid(true, cx));
                self.show_hint = true;
                let width = window.bounds().size.width;
                window.resize(size(width, px(HEIGHT + HINT_HEIGHT)));
                cx.notify();
            }
        }
    }

    fn set_input_valid(&mut self, valid: bool, window: &mut Window, cx: &mut Context<Self>) {
        self.input.update(cx, |input, cx| input.set_invalid(!valid, cx));
        if valid && self.show_hint {
            self.show_hint = false;
            let width = window.bounds().size.width;
            window.resize(size(width, px(HEIGHT)));
            cx.notify();
        }
    }
}

impl Render for NewAlarmWindow {
    fn render(&mut self, _window: &mut Window, cx: &mut Context<Self>) -> impl IntoElement {
        div()
            .size_full()
            .bg(rgb(0xf3f3f3))
            .font_family(style::UI_FONT)
            .text_size(px(style::UI_FONT_SIZE))
            .p(px(11.))
            .flex()
            .flex_col()
            .gap(px(8.))
            .child(
                div()
                    .flex()
                    .flex_row()
                    .items_center()
                    .gap(px(10.))
                    .child(div().flex_1().child(self.input.clone()))
                    .child(
                        style::push_button("start", "Start")
                            .w(px(78.))
                            .h(px(style::CONTROL_HEIGHT))
                            .on_click(cx.listener(|this, _, window, cx| this.submit(window, cx))),
                    ),
            )
            .when(self.show_hint, |this| {
                this.child(
                    div()
                        .px(px(6.))
                        .py(px(3.))
                        .rounded(px(4.))
                        .border_1()
                        .border_color(style::rgba8(0, 0, 0, 0.15))
                        .bg(rgb(0xfbfbfb))
                        .text_color(rgb(0x1b1b1b))
                        .line_height(px(style::UI_LINE_HEIGHT))
                        .children(
                            USAGE_HINT
                                .lines()
                                .map(|line| div().whitespace_nowrap().child(line.replace("  ", "\u{2003}"))),
                        ),
                )
            })
    }
}
