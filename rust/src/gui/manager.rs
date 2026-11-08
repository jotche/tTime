//! Creates alarms (from the command line or the input window), keeps them while
//! they are in use and quits the application when nothing is left to do.
//!
//! Each alarm is presented by one window at a time: the countdown window while
//! it runs, the "Time's up" window once it has finished. All operations are
//! deferred to the end of the current gpui update, because they are usually
//! triggered from inside a window that is about to be closed.

use std::cell::RefCell;
use std::rc::Rc;

use gpui::{App, AppContext, Entity, Global, WindowHandle};
use ttime::alarm::Alarm;
use ttime::alarm_spec::{AlarmSpec, parse_alarm_spec};
use ttime::config::AppConfig;
use ttime::settings::AppSettings;

use super::countdown::CountdownWindow;
use super::new_alarm::NewAlarmWindow;
use super::style::Icons;
use super::time_up::TimeUpWindow;

/// What every window needs: read once at startup and shared.
pub struct Shared {
    pub config: AppConfig,
    pub settings: RefCell<AppSettings>,
    pub icons: Icons,
}

enum Presenter {
    Countdown(WindowHandle<CountdownWindow>),
    TimeUp(WindowHandle<TimeUpWindow>),
}

struct AlarmEntry {
    id: u64,
    presenter: Presenter,
}

pub struct Manager {
    shared: Rc<Shared>,
    new_alarm_window: Option<WindowHandle<NewAlarmWindow>>,
    alarms: Vec<AlarmEntry>,
    next_id: u64,
}

impl Global for Manager {}

pub fn init(shared: Rc<Shared>, cx: &mut App) {
    cx.set_global(Manager { shared, new_alarm_window: None, alarms: Vec::new(), next_id: 1 });
}

pub fn shared(cx: &App) -> Rc<Shared> {
    cx.global::<Manager>().shared.clone()
}

/// `initial_spec` is e.g. the command line ("5m green tea"): a valid spec starts
/// an alarm, anything else opens the input window (pre-filled if not empty).
pub fn start(initial_spec: String, cx: &mut App) {
    match parse_alarm_spec(&initial_spec) {
        Some(spec) => add_alarm(spec, cx),
        None => show_new_alarm_window(Some(initial_spec), cx),
    }
}

/// Opens (or brings to the front) the "New alarm" input window.
pub fn show_new_alarm_window(prefill: Option<String>, cx: &mut App) {
    cx.defer(move |cx| {
        let existing = cx.global::<Manager>().new_alarm_window;
        if let Some(handle) = existing {
            let shown = handle.update(cx, |view, window, cx| {
                if let Some(text) = &prefill {
                    view.set_text(text, cx);
                }
                view.show(window, cx);
            });
            if shown.is_ok() {
                return;
            }
        }
        let shared = shared(cx);
        match NewAlarmWindow::open(shared, prefill.unwrap_or_default(), cx) {
            Ok(handle) => cx.global_mut::<Manager>().new_alarm_window = Some(handle),
            Err(error) => {
                log::error!("Cannot open the New alarm window: {error}");
                check_idle(cx);
            }
        }
    });
}

/// The user closed the input window.
pub fn new_alarm_window_closed(cx: &mut App) {
    cx.defer(|cx| {
        cx.global_mut::<Manager>().new_alarm_window = None;
        check_idle(cx);
    });
}

pub fn add_alarm(spec: AlarmSpec, cx: &mut App) {
    cx.defer(move |cx| {
        // The input window goes away when an alarm starts (it is reopened on demand).
        if let Some(handle) = cx.global_mut::<Manager>().new_alarm_window.take() {
            let _ = handle.update(cx, |_, window, _| window.remove_window());
        }
        let shared = shared(cx);
        let manager = cx.global_mut::<Manager>();
        let id = manager.next_id;
        manager.next_id += 1;

        let alarm = cx.new(|_| Alarm::new(spec.duration, spec.message.clone()));
        match CountdownWindow::open(id, alarm.clone(), shared, cx) {
            Ok(handle) => {
                cx.global_mut::<Manager>().alarms.push(AlarmEntry { id, presenter: Presenter::Countdown(handle) });
                alarm.update(cx, |alarm, _| alarm.start());
            }
            Err(error) => {
                log::error!("Cannot open the countdown window: {error}");
                check_idle(cx);
            }
        }
    });
}

/// Time is up: the "Time's up" window replaces the countdown window.
pub fn alarm_finished(id: u64, alarm: Entity<Alarm>, cx: &mut App) {
    cx.defer(move |cx| {
        let Some(index) = cx.global::<Manager>().alarms.iter().position(|entry| entry.id == id) else {
            return;
        };
        let shared = shared(cx);
        let message = alarm.read(cx).message().to_string();
        match TimeUpWindow::open(id, message, shared, cx) {
            Ok(handle) => {
                let old = std::mem::replace(
                    &mut cx.global_mut::<Manager>().alarms[index].presenter,
                    Presenter::TimeUp(handle),
                );
                close_presenter(old, cx);
            }
            Err(error) => log::error!("Cannot open the Time's up window: {error}"),
        }
    });
}

/// The user is done with an alarm (quit, or dismissed the "Time's up" window).
pub fn remove_alarm(id: u64, cx: &mut App) {
    cx.defer(move |cx| {
        let manager = cx.global_mut::<Manager>();
        let Some(index) = manager.alarms.iter().position(|entry| entry.id == id) else {
            return; // already removed (e.g. closed twice)
        };
        let entry = manager.alarms.remove(index);
        close_presenter(entry.presenter, cx);
        check_idle(cx);
    });
}

fn close_presenter(presenter: Presenter, cx: &mut App) {
    let _ = match presenter {
        Presenter::Countdown(handle) => handle.update(cx, |view, window, cx| {
            view.close(window, cx);
            window.remove_window();
        }),
        Presenter::TimeUp(handle) => handle.update(cx, |view, window, cx| {
            view.close(window, cx);
            window.remove_window();
        }),
    };
}

/// No alarm is left and the input window is closed: quit.
fn check_idle(cx: &mut App) {
    let manager = cx.global::<Manager>();
    if manager.alarms.is_empty() && manager.new_alarm_window.is_none() {
        cx.quit();
    }
}
