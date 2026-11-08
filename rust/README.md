# tTime (Rust / gpui)

A port of tTime, the countdown alarm that lives in the system tray, from Qt 6 / C++ to
**Rust** with the **[gpui](https://crates.io/crates/gpui)** UI framework (Zed's GPU-accelerated
UI library). Same features, same look, same `tTime.ini`.

```powershell
tTime.exe                 # opens the "New alarm" input
tTime.exe 5m green tea    # starts a 5 minute alarm
tTime.exe 1h 30m meeting
tTime.exe 10s test        # quick test: rings after 10 s
```

## Build, test, run (Windows)

Requirements: Rust (stable, edition 2024; `winget install Rustlang.Rustup`) and the
Visual Studio 2022 Build Tools with the Windows 10/11 SDK (gpui compiles its DirectX shaders
with the SDK's `fxc.exe`).

```powershell
cd rust
cargo build --release     # target\release\tTime.exe (+ tTime.ini copied next to it)
cargo test                # unit tests of the core logic
cargo clippy --all-targets
.\target\release\tTime.exe 10s test
```

The release exe is one file of about 13 MB and needs no DLLs beyond Windows' own. Ship it with
`tTime.ini` next to it (the build copies `../src/tTime.ini` into `target/<profile>/`). Debug
builds show a console with log output (`RUST_LOG=info`); release builds are GUI-only.

The assets (fonts, icons, tree picture, sound) are compiled in from `../src/Resources`, which
the Qt and Rust versions share.

## Behaviour (same as the Qt version)

- Alarm syntax `[<n>h] [<n>m|min] [<n>s|sec] [message]`, monotonic clock, 4 ticks a second.
- Countdown window: frameless, translucent themed gradient, play/pause, stop, restart,
  "always on top", minimize to tray, quit. Drag to move, **Ctrl+drag** to change the width (the
  height follows the content), right-click for the context menu, Alt+F4 hides to the tray.
  Size and position are remembered (`[progress-dialog]` keys) and cascaded across windows and
  processes (`%TEMP%\tTime-window-slots`).
- Tray icon per alarm that fills up as time passes (also the taskbar icon); double-click shows
  or hides the window; menu: Minimize, Restore, New Alarm, Quit.
- "Time's Up" window: always on top, rounded panel, swaying tree, ringing bell in the tray and
  taskbar, the alarm sound looped for about 7 s; New Alarm / Close.
- Settings from `%APPDATA%\tTime\tTime.ini`, then `tTime.ini` next to the exe (fonts, weights,
  sizes, theme, animations; see `../src/tTime.ini`). Existing Qt settings files keep working.
- The app quits when no alarm is left and the input window is closed.

## Layout

| Path | Contents |
|------|----------|
| `src/lib.rs` + modules | GUI-free core with unit tests: alarm model, alarm syntax, INI settings (QSettings-compatible), themes and Qt-compatible HSL recoloring, bell and tree animation, digit layout, WAV length, window placement and cascade slots, configuration |
| `src/gui/` | gpui windows (`countdown`, `time_up`, `new_alarm`), `text_input` (single-line field), `style` (panel gradients, buttons), `tray` (tray icons and native menus via `tray-icon`/`muda`), `sound` (`rodio`), `platform` (Win32: hide/show, topmost, window icons, work areas, system move), `manager` (alarm lifecycle) |
| `build.rs` | embeds the icon and version info, copies `tTime.ini` |

## Differences from the Qt version

- gpui has no tray, native menus, text field, window hiding or "always on top" toggle; these use
  `tray-icon`/`muda`, an own text field and Win32 calls (`src/gui/platform.rs`). On macOS and
  Linux those native helpers are stubs (minimize instead of hide, no topmost toggle), so Windows
  is the supported platform for now.
- gpui gradients have two stops, so the multi-stop panel gradients are drawn as adjacent
  two-stop segments.
- `--help` / `--version` show a message box (the release exe has no console).
- Tooltips are drawn inside the (small) window by gpui rather than as separate popups.
