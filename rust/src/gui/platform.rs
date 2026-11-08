//! Native window operations gpui does not offer: hiding a window to the tray,
//! showing and activating it again, "always on top", moving a frameless window
//! with the system's move loop, per-window icons and the usable screen areas.
//!
//! On platforms other than Windows these degrade to what gpui can do itself.

use gpui::Window;
use image::RgbaImage;
use ttime::window_placement::Rect;

/// The native handle of a gpui window (an `HWND` on Windows), or 0.
pub fn native_handle(window: &Window) -> isize {
    use raw_window_handle::{HasWindowHandle, RawWindowHandle};
    match HasWindowHandle::window_handle(window).map(|h| h.as_raw()) {
        #[cfg(windows)]
        Ok(RawWindowHandle::Win32(handle)) => handle.hwnd.get(),
        #[cfg(target_os = "macos")]
        Ok(RawWindowHandle::AppKit(handle)) => handle.ns_view.as_ptr() as isize,
        _ => 0,
    }
}

#[cfg(windows)]
mod imp {
    use super::*;
    use std::cell::RefCell;
    use std::collections::HashMap;
    use windows::Win32::Foundation::{HWND, LPARAM, POINT, RECT, WPARAM};
    use windows::Win32::Graphics::Gdi::{
        EnumDisplayMonitors, GetMonitorInfoW, HDC, HMONITOR, MONITOR_DEFAULTTONEAREST, MONITOR_DEFAULTTOPRIMARY,
        MONITORINFO, MonitorFromPoint,
    };
    use windows::Win32::UI::HiDpi::{GetDpiForMonitor, MDT_EFFECTIVE_DPI};
    use windows::Win32::UI::Input::KeyboardAndMouse::ReleaseCapture;
    use windows::Win32::UI::WindowsAndMessaging::*;
    use windows::core::BOOL;

    fn hwnd(handle: isize) -> HWND {
        HWND(handle as *mut _)
    }

    pub fn hide(handle: isize) {
        if handle != 0 {
            unsafe {
                let _ = ShowWindow(hwnd(handle), SW_HIDE);
            }
        }
    }

    pub fn is_visible(handle: isize) -> bool {
        handle != 0 && unsafe { IsWindowVisible(hwnd(handle)).as_bool() }
    }

    pub fn show_and_activate(handle: isize) {
        if handle == 0 {
            return;
        }
        unsafe {
            let h = hwnd(handle);
            if IsIconic(h).as_bool() {
                let _ = ShowWindow(h, SW_RESTORE);
            } else {
                let _ = ShowWindow(h, SW_SHOW);
            }
            let _ = BringWindowToTop(h);
            let _ = SetForegroundWindow(h);
        }
    }

    pub fn set_topmost(handle: isize, on_top: bool) {
        if handle == 0 {
            return;
        }
        let after = if on_top { HWND_TOPMOST } else { HWND_NOTOPMOST };
        unsafe {
            let _ = SetWindowPos(hwnd(handle), Some(after), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }

    /// Makes a gpui window without a title bar truly frameless.
    ///
    /// gpui keeps a non-client frame around such windows on Windows 11: 1 px at
    /// the top, which the system paints as a white line, and invisible resize
    /// borders on the other sides. A subclass removes the whole non-client area
    /// (and the top-edge resize zone gpui's hit test adds), the window is then
    /// shrunk onto its old client rectangle, and gpui is made to re-measure its
    /// border offsets (it does so on WM_SETTINGCHANGE).
    ///
    /// This sends messages that gpui handles synchronously, so it must run outside
    /// of any gpui update (e.g. from a spawned task).
    pub fn make_frameless(handle: isize) {
        use windows::Win32::UI::Shell::SetWindowSubclass;
        if handle == 0 {
            return;
        }
        let h = hwnd(handle);
        unsafe {
            let mut window = RECT::default();
            let mut client = RECT::default();
            if GetWindowRect(h, &mut window).is_err() || GetClientRect(h, &mut client).is_err() {
                return;
            }
            if !SetWindowSubclass(h, Some(frameless_proc), FRAMELESS_SUBCLASS_ID, 0).as_bool() {
                return;
            }
            // gpui placed the window rect around the requested bounds with half of the
            // frame on each side; undo that to get exactly the requested bounds.
            let (width, height) = (client.right - client.left, client.bottom - client.top);
            let x = window.left + ((window.right - window.left) - width) / 2;
            let y = window.top + ((window.bottom - window.top) - height) / 2;
            let _ = SetWindowPos(
                h,
                None,
                x,
                y,
                width,
                height,
                SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE,
            );
            // gpui re-reads its border offsets here (104 = SPI_SETWHEELSCROLLLINES: harmless).
            SendMessageW(h, WM_SETTINGCHANGE, Some(WPARAM(104)), Some(LPARAM(0)));
        }
    }

    const FRAMELESS_SUBCLASS_ID: usize = 0x7454696d; // "tTim"

    unsafe extern "system" fn frameless_proc(
        handle: HWND,
        msg: u32,
        wparam: WPARAM,
        lparam: LPARAM,
        _id: usize,
        _data: usize,
    ) -> windows::Win32::Foundation::LRESULT {
        use windows::Win32::Foundation::LRESULT;
        use windows::Win32::UI::Shell::DefSubclassProc;
        match msg {
            // The client area is the whole window.
            WM_NCCALCSIZE if wparam.0 != 0 => LRESULT(0),
            // No resize edges; moving is started by the window itself.
            WM_NCHITTEST => LRESULT(HTCLIENT as isize),
            _ => unsafe { DefSubclassProc(handle, msg, wparam, lparam) },
        }
    }

    /// Turns off Windows 11's automatic rounding of the window corners (the
    /// frameless windows paint their own shape).
    pub fn square_corners(handle: isize) {
        use windows::Win32::Graphics::Dwm::{
            DWMWA_BORDER_COLOR, DWMWA_COLOR_NONE, DWMWA_WINDOW_CORNER_PREFERENCE, DWMWCP_DONOTROUND,
            DwmSetWindowAttribute,
        };
        if handle == 0 {
            return;
        }
        let preference = DWMWCP_DONOTROUND;
        unsafe {
            let _ = DwmSetWindowAttribute(
                hwnd(handle),
                DWMWA_WINDOW_CORNER_PREFERENCE,
                &preference as *const _ as *const _,
                std::mem::size_of_val(&preference) as u32,
            );
            // No system border either: the panel draws its own outline.
            let none = DWMWA_COLOR_NONE;
            let _ = DwmSetWindowAttribute(
                hwnd(handle),
                DWMWA_BORDER_COLOR,
                &none as *const _ as *const _,
                std::mem::size_of_val(&none) as u32,
            );
        }
    }

    /// Hands the window to the system's move loop (like dragging a title bar).
    /// Posted, so it starts after the current gpui event has been handled.
    pub fn start_system_move(handle: isize) {
        if handle == 0 {
            return;
        }
        const SC_DRAGMOVE: usize = 0xF012; // SC_MOVE | HTCAPTION
        unsafe {
            let _ = ReleaseCapture();
            let _ = PostMessageW(Some(hwnd(handle)), WM_SYSCOMMAND, WPARAM(SC_DRAGMOVE), LPARAM(0));
        }
    }

    thread_local! {
        // Icons we created per window (big, small), destroyed when replaced.
        static ICONS: RefCell<HashMap<isize, (isize, isize)>> = RefCell::new(HashMap::new());
    }

    fn create_icon(image: &RgbaImage, size: u32) -> Option<HICON> {
        let scaled = image::imageops::resize(image, size, size, image::imageops::FilterType::Triangle);
        let mut bgra = scaled.into_raw();
        for px in bgra.as_chunks_mut::<4>().0 {
            px.swap(0, 2);
        }
        // 1 bpp AND mask, rows padded to 16 bits; all zero (alpha comes from the color bitmap).
        let mask_stride = (size as usize).div_ceil(16) * 2;
        let and_mask = vec![0u8; mask_stride * size as usize];
        unsafe { CreateIcon(None, size as i32, size as i32, 1, 32, and_mask.as_ptr(), bgra.as_ptr()).ok() }
    }

    /// Sets the window's own icon (title bar, taskbar, Alt+Tab).
    pub fn set_window_icon(handle: isize, image: &RgbaImage) {
        if handle == 0 {
            return;
        }
        let (big_size, small_size) =
            unsafe { (GetSystemMetrics(SM_CXICON).max(16) as u32, GetSystemMetrics(SM_CXSMICON).max(16) as u32) };
        let (Some(big), Some(small)) = (create_icon(image, big_size), create_icon(image, small_size)) else {
            return;
        };
        unsafe {
            SendMessageW(hwnd(handle), WM_SETICON, Some(WPARAM(ICON_BIG as usize)), Some(LPARAM(big.0 as isize)));
            SendMessageW(hwnd(handle), WM_SETICON, Some(WPARAM(ICON_SMALL as usize)), Some(LPARAM(small.0 as isize)));
        }
        let old = ICONS.with(|icons| icons.borrow_mut().insert(handle, (big.0 as isize, small.0 as isize)));
        if let Some((old_big, old_small)) = old {
            unsafe {
                let _ = DestroyIcon(HICON(old_big as *mut _));
                let _ = DestroyIcon(HICON(old_small as *mut _));
            }
        }
    }

    /// Releases the icons created for a window that is going away.
    pub fn forget_window(handle: isize) {
        if let Some((big, small)) = ICONS.with(|icons| icons.borrow_mut().remove(&handle)) {
            unsafe {
                let _ = DestroyIcon(HICON(big as *mut _));
                let _ = DestroyIcon(HICON(small as *mut _));
            }
        }
    }

    fn monitor_scale(monitor: HMONITOR) -> f32 {
        let (mut x, mut y) = (96u32, 96u32);
        if unsafe { GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &mut x, &mut y) }.is_err() {
            return 1.0;
        }
        x as f32 / 96.0
    }

    fn logical(rect: RECT, scale: f32) -> Rect {
        Rect {
            x: (rect.left as f32 / scale).round() as i32,
            y: (rect.top as f32 / scale).round() as i32,
            width: ((rect.right - rect.left) as f32 / scale).round() as i32,
            height: ((rect.bottom - rect.top) as f32 / scale).round() as i32,
        }
    }

    unsafe extern "system" fn collect(monitor: HMONITOR, _: HDC, _: *mut RECT, data: LPARAM) -> BOOL {
        let areas = unsafe { &mut *(data.0 as *mut Vec<Rect>) };
        let mut info = MONITORINFO { cbSize: std::mem::size_of::<MONITORINFO>() as u32, ..Default::default() };
        if unsafe { GetMonitorInfoW(monitor, &mut info) }.as_bool() {
            areas.push(logical(info.rcWork, monitor_scale(monitor)));
        }
        BOOL(1)
    }

    /// The usable area (without taskbars) of every screen, in gpui's logical
    /// coordinates (each monitor's physical position divided by its scale).
    pub fn available_areas() -> Vec<Rect> {
        let mut areas: Vec<Rect> = Vec::new();
        unsafe {
            let _ = EnumDisplayMonitors(None, None, Some(collect), LPARAM(&mut areas as *mut _ as isize));
        }
        if areas.is_empty() {
            let monitor = unsafe { MonitorFromPoint(POINT::default(), MONITOR_DEFAULTTONEAREST) };
            let mut info = MONITORINFO { cbSize: std::mem::size_of::<MONITORINFO>() as u32, ..Default::default() };
            if unsafe { GetMonitorInfoW(monitor, &mut info) }.as_bool() {
                areas.push(logical(info.rcWork, monitor_scale(monitor)));
            }
        }
        areas
    }

    /// The usable area of the primary screen, in logical coordinates.
    pub fn primary_area() -> Option<Rect> {
        let monitor = unsafe { MonitorFromPoint(POINT::default(), MONITOR_DEFAULTTOPRIMARY) };
        let mut info = MONITORINFO { cbSize: std::mem::size_of::<MONITORINFO>() as u32, ..Default::default() };
        unsafe { GetMonitorInfoW(monitor, &mut info) }.as_bool().then(|| logical(info.rcWork, monitor_scale(monitor)))
    }

    /// The Windows accent color (for focus lines), from the user's personalization settings.
    pub fn accent_color() -> Option<(u8, u8, u8)> {
        use windows::Win32::System::Registry::{HKEY_CURRENT_USER, RRF_RT_REG_DWORD, RegGetValueW};
        use windows::core::w;
        let mut value: u32 = 0;
        let mut len = std::mem::size_of::<u32>() as u32;
        let status = unsafe {
            RegGetValueW(
                HKEY_CURRENT_USER,
                w!(r"Software\Microsoft\Windows\CurrentVersion\Explorer\Accent"),
                w!("AccentColorMenu"),
                RRF_RT_REG_DWORD,
                None,
                Some(&mut value as *mut u32 as *mut _),
                Some(&mut len),
            )
        };
        // Stored as 0xAABBGGRR.
        status.is_ok().then_some((value as u8, (value >> 8) as u8, (value >> 16) as u8))
    }

    /// A blocking information box (used for --help and --version, since tTime
    /// is a GUI program without a console).
    pub fn message_box(title: &str, text: &str) {
        use windows::core::HSTRING;
        unsafe {
            MessageBoxW(None, &HSTRING::from(text), &HSTRING::from(title), MB_OK | MB_ICONINFORMATION);
        }
    }
}

#[cfg(not(windows))]
mod imp {
    use super::*;

    pub fn hide(_handle: isize) {}
    pub fn is_visible(_handle: isize) -> bool {
        true
    }
    pub fn show_and_activate(_handle: isize) {}
    pub fn set_topmost(_handle: isize, _on_top: bool) {}
    pub fn start_system_move(_handle: isize) {}
    pub fn square_corners(_handle: isize) {}
    pub fn make_frameless(_handle: isize) {}
    pub fn set_window_icon(_handle: isize, _image: &RgbaImage) {}
    pub fn forget_window(_handle: isize) {}
    pub fn available_areas() -> Vec<Rect> {
        Vec::new()
    }
    pub fn message_box(_title: &str, text: &str) {
        println!("{text}");
    }
    pub fn primary_area() -> Option<Rect> {
        None
    }
    pub fn accent_color() -> Option<(u8, u8, u8)> {
        None
    }
}

pub use imp::*;
