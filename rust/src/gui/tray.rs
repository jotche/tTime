//! Tray icons and native menus (via the `tray-icon`/`muda` crates), and the
//! routing of their events into gpui.
//!
//! Every tray icon belongs to an owner (one tTime window) with a unique key.
//! The tray icon's id is that key, and the ids of the owner's menu items are
//! "<key>/<action>", so events go back to the owner's handler as
//! [`TrayEvent::DoubleClick`], [`TrayEvent::Click`] or [`TrayEvent::Menu`].

use std::collections::HashMap;
use std::rc::Rc;

use futures::StreamExt;
use futures::channel::mpsc;
use gpui::{App, Global};
use image::RgbaImage;
use tray_icon::menu::{ContextMenu, IconMenuItem, IsMenuItem, Menu, MenuEvent, MenuItem, PredefinedMenuItem};
use tray_icon::{MouseButton, MouseButtonState, TrayIcon, TrayIconBuilder, TrayIconEvent};

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum TrayEvent {
    Click,
    DoubleClick,
    Menu(String),
}

type Handler = Rc<dyn Fn(TrayEvent, &mut App)>;

#[derive(Default)]
struct TrayRouter {
    handlers: HashMap<String, Handler>,
}

impl Global for TrayRouter {}

enum NativeEvent {
    Tray(TrayIconEvent),
    Menu(MenuEvent),
}

/// Starts forwarding tray and menu events to the registered owners.
pub fn init(cx: &mut App) {
    cx.set_global(TrayRouter::default());
    let (sender, mut receiver) = mpsc::unbounded::<NativeEvent>();
    let tray_sender = sender.clone();
    TrayIconEvent::set_event_handler(Some(move |event| {
        let _ = tray_sender.unbounded_send(NativeEvent::Tray(event));
    }));
    MenuEvent::set_event_handler(Some(move |event| {
        let _ = sender.unbounded_send(NativeEvent::Menu(event));
    }));
    cx.spawn(async move |cx| {
        while let Some(event) = receiver.next().await {
            let routed = match event {
                NativeEvent::Tray(TrayIconEvent::DoubleClick { id, button: MouseButton::Left, .. }) => {
                    Some((id.0, TrayEvent::DoubleClick))
                }
                NativeEvent::Tray(TrayIconEvent::Click {
                    id,
                    button: MouseButton::Left,
                    button_state: MouseButtonState::Up,
                    ..
                }) => Some((id.0, TrayEvent::Click)),
                NativeEvent::Tray(_) => None,
                NativeEvent::Menu(event) => event
                    .id
                    .0
                    .split_once('/')
                    .map(|(owner, action)| (owner.to_string(), TrayEvent::Menu(action.to_string()))),
            };
            if let Some((owner, event)) = routed {
                let _ = cx.update(|cx| {
                    let handler = cx.global::<TrayRouter>().handlers.get(&owner).cloned();
                    if let Some(handler) = handler {
                        handler(event, cx);
                    }
                });
            }
        }
    })
    .detach();
}

/// Registers the handler for the owner `key` (replacing any previous one).
pub fn register(cx: &mut App, key: &str, handler: impl Fn(TrayEvent, &mut App) + 'static) {
    cx.global_mut::<TrayRouter>().handlers.insert(key.to_string(), Rc::new(handler));
}

pub fn unregister(cx: &mut App, key: &str) {
    if cx.has_global::<TrayRouter>() {
        cx.global_mut::<TrayRouter>().handlers.remove(key);
    }
}

/// A unique owner key ("alarm-1", "alarm-2", ...).
pub fn new_key(prefix: &str) -> String {
    use std::sync::atomic::{AtomicU64, Ordering};
    static NEXT: AtomicU64 = AtomicU64::new(1);
    format!("{prefix}-{}", NEXT.fetch_add(1, Ordering::Relaxed))
}

/// The image as a tray icon at the size the taskbar shows (scaled smoothly,
/// rather than leaving it to the system).
pub fn tray_icon_image(image: &RgbaImage) -> tray_icon::Icon {
    let size = small_icon_size();
    let scaled = if image.width() == size && image.height() == size {
        image.clone()
    } else {
        image::imageops::resize(image, size, size, image::imageops::FilterType::Triangle)
    };
    let (w, h) = scaled.dimensions();
    tray_icon::Icon::from_rgba(scaled.into_raw(), w, h).expect("valid icon size")
}

pub fn menu_icon(image: &RgbaImage) -> tray_icon::menu::Icon {
    let size = small_icon_size();
    let scaled = image::imageops::resize(image, size, size, image::imageops::FilterType::Triangle);
    tray_icon::menu::Icon::from_rgba(scaled.into_raw(), size, size).expect("valid icon size")
}

fn small_icon_size() -> u32 {
    #[cfg(windows)]
    {
        use windows::Win32::UI::WindowsAndMessaging::{GetSystemMetrics, SM_CXSMICON};
        (unsafe { GetSystemMetrics(SM_CXSMICON) }).max(16) as u32
    }
    #[cfg(not(windows))]
    {
        32
    }
}

pub fn create_tray_icon(key: &str, icon: tray_icon::Icon, tooltip: &str, menu: &Menu) -> Option<TrayIcon> {
    TrayIconBuilder::new()
        .with_id(key)
        .with_icon(icon)
        .with_tooltip(tooltip)
        .with_menu(Box::new(menu.clone()))
        .with_menu_on_left_click(false)
        .build()
        .inspect_err(|error| log::warn!("Cannot create the tray icon: {error}"))
        .ok()
}

/// A menu item whose id routes back to `key`'s handler as `action`.
pub fn item(key: &str, action: &str, text: &str, enabled: bool) -> MenuItem {
    MenuItem::with_id(format!("{key}/{action}"), text, enabled, None)
}

pub fn icon_item(key: &str, action: &str, text: &str, icon: &RgbaImage) -> IconMenuItem {
    IconMenuItem::with_id(format!("{key}/{action}"), text, true, Some(menu_icon(icon)), None)
}

pub fn separator() -> PredefinedMenuItem {
    PredefinedMenuItem::separator()
}

pub fn menu(items: &[&dyn IsMenuItem]) -> Menu {
    let menu = Menu::new();
    if let Err(error) = menu.append_items(items) {
        log::warn!("Cannot build menu: {error}");
    }
    menu
}

/// Shows `menu` at the mouse position as the context menu of the native window
/// `handle`. The menu runs its own modal loop, so it is started from a task,
/// outside of any gpui update; the chosen item arrives as a menu event.
pub fn show_context_menu(cx: &App, menu: Menu, handle: isize) {
    if handle == 0 {
        return;
    }
    cx.foreground_executor()
        .spawn(async move {
            #[cfg(windows)]
            unsafe {
                menu.show_context_menu_for_hwnd(handle, None);
            }
            #[cfg(not(windows))]
            let _ = (menu, handle);
        })
        .detach();
}
