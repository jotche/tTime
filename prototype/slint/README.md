# tTime with Slint (C++)

tTime ported from Qt 6 to [Slint](https://slint.dev) 1.18, with the same features, look and
settings, to compare memory use, startup and code. It reuses the fonts, icons, pictures and
sound in `src/Resources` and reads and writes the same `tTime.ini` files as the Qt version.

## What is ported

- **Alarms:** `5m green tea`, `1h 30m meeting`, ... on the command line or in the input window
  (same grammar, invalid entries are marked and explained); several alarms at once; the
  program ends when no alarm and no input window is left; `--help`, `--version`.
- **Countdown window:** frameless and translucent with the themed gradient; pause/start, stop,
  restart; always on top, minimise to tray, quit; fixed-width digits; drag to move, Ctrl+drag
  to widen (the height follows the content); right-click menu; tooltips; Alt+F4 minimises.
- **Tray icon:** fills up as time passes, tooltip `hh:mm:ss - message`, click shows or hides the
  window, menu with Minimize / Restore / New Alarm / Quit. The taskbar icon shows the same.
- **Time's up window:** centred, on top, swaying tree, ringing bell in the tray and the taskbar,
  alarm sound for about 7 s in whole loops, New Alarm / Close, Alt+F4 closes.
- **Settings:** `%APPDATA%\tTime\tTime.ini` over the shipped `tTime.ini`: theme (8 themes, icon
  and tree recolored), countdown and message fonts (variable weights), sizes, animations; the
  countdown window's position and size are remembered, fitted onto the visible screens and
  cascaded across processes.
- **Accessibility:** buttons, texts and the countdown are exposed to screen readers.

## Differences from the Qt version

- The tray icon reacts to a single click (Slint reports no double click).
- Tooltips are drawn inside the window; the input window shows its usage hint below the input
  after an invalid entry instead of as a tooltip.
- Slint does not update the taskbar icon on Windows, so `src/platform.cpp` sets it with
  `WM_SETICON`.
- Screen areas (for fitting and centring windows), bringing a window to the front and the
  taskbar icon are implemented for Windows only; on macOS and Linux windows are placed without
  fitting. The alarm sound uses `PlaySoundW` on Windows and `afplay` / `paplay` / `aplay`
  elsewhere. macOS and Linux are untested.
- Two alarms ringing at the same moment share one sound (`PlaySoundW` plays one at a time).

## Build (Windows)

Needs the Visual Studio 2022 Build Tools (C++ workload) and Slint's prebuilt C++ package
(`Slint-cpp-1.18.1-win64-MSVC-AMD64.exe` from the Slint GitHub releases). From an
"x64 Native Tools Command Prompt for VS 2022", in the repository root:

```bat
cmake -S prototype/slint -B build/slint-prototype -G Ninja -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_PREFIX_PATH=%LOCALAPPDATA%/Slint/cpp-1.18.1
cmake --build build/slint-prototype
ctest --test-dir build/slint-prototype            :: logic tests
build\slint-prototype\ttime-slint.exe 5m green tea
```

The program uses Slint's software renderer (`winit-software`); set `SLINT_BACKEND` to try
another one (`winit-femtovg`, `winit-skia`, `winit-skia-software`).

## Layout

| Path | Contents |
|---|---|
| `ui/*.slint` | the windows, tray icons, theme, fonts and widgets |
| `src/alarm*.{h,cpp}` | alarm model, parsing, holder and manager |
| `src/countdown-window.*`, `time-up-window.*`, `new-alarm-window.*` | the windows' C++ side |
| `src/settings.*`, `config.*`, `theme.*` | INI files, configuration, themes and recoloring |
| `src/bell-animation.*`, `tree-sway.*`, `image.h` | icon and tree animation |
| `src/sound.*`, `platform.*`, `window-placement.*` | sound, OS specifics, window placement |
| `tests/logic-tests.cpp` | tests of the logic (cases from the Qt test suites) |

## Measurements

Windows 11, 96 DPI, `45m memtest`, read 6 s after start (two runs each).
"Private WS" is what Task Manager shows as memory.

| Build | Private WS | Private commit | Threads | DLLs | First window |
|---|---|---|---|---|---|
| Qt 6.12, trimmed static (`clang64-release-static-min`) | 8.5 MB | 10.1 MB | 13 | 74 | 150-200 ms |
| Slint port, software renderer | **6.8 MB** | 7.8 MB | 10 | 52 | ~27 ms |

While the alarm rings ("Time's up" window, sound, animations): Qt 21.3 MB, Slint 11.1 MB. Qt's
jump comes from Qt Multimedia starting the Windows audio stack; the port uses `PlaySoundW`.

The GPU renderers use far more memory (FemtoVG 41-93 MB, Skia 96-105 MB in the first
prototype), so the software renderer is the default.

`slint_cpp.dll` (the prebuilt package, with every renderer, the interpreter and live preview)
is 26.7 MB; `ttime-slint.exe` is about 1 MB. A Slint built from source with only the software
renderer (needs a Rust toolchain) would be much smaller and could be linked statically.
