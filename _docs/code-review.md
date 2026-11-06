# tTime code review

Review of the Qt 6.12 / CMake / C++20 port, 2026-11-06. Performed by a Claude Fable
agent at high effort, read-only (it rebuilt the project and ran the tests, nothing
else was changed).

**Status legend:** open · fixed · partly fixed · won't fix (with reason)

## Build and test result at review time

- Clean rebuild with `-Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Woverloaded-virtual`:
  0 warnings, 0 errors.
- `ctest --preset mingw-release`: 11/11 passed (2.96 s). `tst_teaalarm` takes 2.2 s because of
  real-time waits; `tst_progressdialog` briefly shows a real window on the desktop.

## Already done well (no need to change)

- Clear model/view split for the alarm: `TeaAlarm` is a QObject model driven by
  `QElapsedTimer` (monotonic, survives system-time changes and sleep); views only connect to
  its signals.
- Pure, testable value-type helpers with `std::optional`/`std::chrono` and no GUI dependency:
  `alarm-spec`, `wav-info`, `digit-layout`, `font-weight`, `window-placement`, with precise,
  data-driven tests.
- `ttime_options` INTERFACE target, `QT_DISABLE_DEPRECATED_UP_TO`,
  `QT_NO_NARROWING_CONVERSIONS_IN_CONNECT`, `QT_USE_QSTRINGBUILDER`,
  `qt_standard_project_setup`, `qt_generate_deploy_app_script` with a documented minimal
  deploy, `CMAKE_EXPORT_COMPILE_COMMANDS`.
- Native `startSystemMove()` / `startSystemResize()` with a manual fallback for the frameless
  window (correct for high-DPI).
- `QLockFile` cross-process window cascade with `setStaleLockTime(0)`: Qt still detects locks
  of dead processes; only the age check is disabled.
- Logging category for the sound, `QCommandLineParser`, `[[nodiscard]]`, deleted copy
  operations, `Q_ENUM`, lifetime comments on raw-pointer parameters.
- README is accurate and useful.

## Summary

| ID | Finding | Kind | Effort | Status |
|----|---------|------|--------|--------|
| H1 | Countdown window blocks Windows shutdown / log-off | ~~bug~~ | S | won't fix: verified not a bug in Qt 6 |
| H2 | Shipped `tTime.ini` found only because organization name is "tTime" | latent bug | S | fixed |
| H3 | Settings read ad hoc in 6 places | design | M | fixed |
| H4 | Tests recompile app sources instead of linking libraries | build | M | fixed |
| H5 | Requires Qt 6.5 but uses Qt 6.7 APIs | bug | S | fixed |
| H6 | No project licence, no Qt LGPL notice, non-OFL fonts without terms | licensing | S | partly fixed (icon/image/sound origin unknown) |
| H7 | No `.gitattributes`; line-ending churn | repo | S | fixed |
| H8 | ~25 unused assets of unknown licence tracked | repo / licensing | S | fixed (46 files removed) |
| M1 | UI intents routed through the alarm model | design | M | fixed |
| M2 | Lifetime relies on queued connections and connection order | design | S | fixed (with M1) |
| M3 | Duplicated gradient painting / tray boilerplate in the two windows | design | M | fixed |
| M4 | Static `QPixmap` cache outlives `QApplication` | latent bug | S | fixed |
| M5 | Label, tray icon and tooltip updated 4× per second | efficiency | S | fixed |
| M6 | Tool-button size lock instead of transparent border | design | S | won't fix: alternatives tried, both regress |
| M7 | Mixed old/new naming; "Dialog" classes are not dialogs; UI text issues | naming | S | fixed |
| M8 | Presets hard-code `C:/Qt`; inconsistent CMake minimum | build | S | fixed |
| M9 | No clang-format / clang-tidy / CI | tooling | M | fixed (CI not run yet) |
| M10 | No VERSIONINFO, no CPack, no tags | packaging | S | fixed (tag after the first commit) |
| T1–T6 | Test gaps (see below) | tests | S–M | T1 and T3 fixed, others open |
| N1–N12 | Smaller improvements (see below) | misc | S | N4, N11 fixed; N6, N9 partly; others open |

## High value

### H1. Countdown window blocks Windows shutdown / log-off (likely bug)
`src/tea-alarm-progress-dialog.cpp` `closeEvent` always does `hide()` + `event->ignore()`.
On shutdown Windows sends `WM_QUERYENDSESSION`; Qt then tries to close all windows, and if
one ignores the close it cancels the session end, so Windows shows "tTime is preventing
shutdown". A tray timer must never do that.
**Fix:** accept the close while the session is ending (`QGuiApplication::isSavingSession()`).

**Resolution: won't fix, verified not a bug in Qt 6.** Closing all windows at session end was
Qt 5's "fallback session management", which no longer exists in Qt 6
(`QGuiApplication::setFallbackSessionManagementEnabled` is gone from Qt 6.12). Verified against
the real app by sending the countdown window `WM_QUERYENDSESSION`, as Windows does at log-off
and shutdown: tTime answers 1 ("session may end") for both, and the window receives no close
request and stays visible.

### H2. Shipped `tTime.ini` found only by a naming coincidence (latent bug)
`main.cpp` sets the INI *system scope* directory to the executable's folder. QSettings then
looks for `<exe dir>/<org>/<app>.ini` and `<exe dir>/<org>.ini`; the shipped file is found
only because the organization name is "tTime". Renaming the organization silently stops the
defaults from loading, and nothing tests the resolution.
**Fix:** one explicit settings layer (user file → shipped defaults → built-in) with a test.

**Resolution: fixed.** New `AppSettings` (`src/app-settings.*`, in `ttime_core`) opens
`%APPDATA%\tTime\tTime.ini` and `<exe dir>\tTime.ini` by explicit path. `value()` returns the
first file that has the key, else the built-in fallback; `setValue()` writes the user file only.
All settings access (countdown font, theme, window geometry) goes through it, and `main.cpp` no
longer calls `QSettings::setPath`. `tests/tst_appsettings.cpp` covers the precedence, missing
files, writes, and that the paths no longer depend on the organization/application name.
Verified in the real app: the user's file overrides the shipped one, and with the key removed
from the user's file the shipped value applies.

### H3. Settings read ad hoc in 6 places
`main.cpp`, `create-alarm-dialog.cpp`, `tea-alarm-progress-dialog.cpp`,
`timer-finished-dialog.cpp`, `theme.cpp`, `countdown-font.cpp` each open QSettings.
Widget tests implicitly depend on global QSettings state.
**Fix:** load an immutable configuration once in `main` and pass it to the windows; keep
theme/font code as pure functions.

**Resolution: fixed.** New `AppConfig` (`src/app-config.*`: theme, countdown font, alarm sound)
is read once in `main.cpp` from `AppSettings` and passed, together with the `AppSettings` used
for the window position, to `TeaAlarmManager` → `AlarmHolder` → the windows. `themeFromSettings()`
and `countdownFontFromSettings()` take the settings as a parameter; `configuredTheme()` and
`countdownFont()` are gone, and no window opens settings files itself. An empty
`AppConfig::alarmSound` means silent, which the tests use. `tst_progressdialog` and the new
`tst_alarmmanager` use `AppConfig::defaults()` and throw-away settings files, so they no longer
depend on (or change) the user's `tTime.ini`. Settings are read at startup, so changes to
`tTime.ini` apply the next time tTime starts.

### H4. Tests recompile application sources
`tests/CMakeLists.txt` compiles `theme.cpp` + `bell-animation.cpp` four times,
`countdown-label.cpp` + `countdown-font.cpp` twice, and `tst_progressdialog` lists six
sources plus the `.ui` and `.qrc`.
**Fix:** static libraries `ttime_core` (Qt Core), `ttime_gui` (theme, bell animation, fonts,
resources via `qt_add_resources`), `ttime_ui` (widgets); the app is just `main.cpp`.

**Resolution: fixed.** `ttime_core` (alarm model, parser, settings, layout helpers),
`ttime_gui` (themes, icon/bell animation, fonts, `AppConfig`, resources) and `ttime_ui`
(windows, holder, manager); `tTime` is `main.cpp` plus `ttime.rc`. Every test links one library
(`ttime_add_test(name LIBS ttime_ui)`); no test compiles application sources any more.
`teaclock.qrc` was replaced by `qt_add_resources()` on `ttime_gui` (same `:/fonts/...` etc.
paths), so Qt links the resources into every executable using the library; the `.ui` files no
longer refer to the `.qrc`. The generated `Ui::` classes are held through `std::unique_ptr` with
forward declarations, so `ui_*.h` is only included by the `.cpp` files and `ttime_ui` does not
need to export its generated headers.

### H5. Requires Qt 6.5 but uses Qt 6.7 APIs (bug)
`CMakeLists.txt` declares `find_package(Qt6 6.5)` / `REQUIRES 6.5`, but `countdown-font.cpp`
uses `QFont::Tag` and `QFont::setVariableAxis`, added in Qt 6.7. Qt 6.5/6.6 users get a
compile error instead of a clear CMake error.
**Fix:** require Qt 6.7.

**Resolution: fixed.** `find_package(Qt6 6.7 ...)` and `qt_standard_project_setup(REQUIRES 6.7)`;
the README states the minimum.

### H6. Licensing gaps
No `LICENSE` file for tTime (which means "all rights reserved"); the Qt DLLs (LGPL-3) are
shipped without a notice; Sansation and GeosansLight are compiled in and used with no
licence text; the origin of icons and the WAV is undocumented.
**Fix:** add a licence, a third-party notices file installed with the app, and replace or
document the non-OFL fonts.

**Resolution: partly fixed.**
- `LICENSE`: MIT, copyright 2010-2026 Vladimir Gjokjevski (owner's choice).
- `THIRD-PARTY-NOTICES.md`: Qt 6 (LGPL-3.0, modules shipped, where to get the source, how to
  replace the DLLs), FFmpeg when `TTIME_MINIMAL_DEPLOY=OFF` (LGPL-2.1+), MinGW-w64 runtime
  (GPL-3.0 with GCC Runtime Library Exception; winpthreads MIT-style), all bundled fonts.
- `licenses/`: GPL-3.0, LGPL-3.0, GCC Runtime Library Exception 3.1 and winpthreads texts, taken
  from the MinGW installation. CMake installs them, `LICENSE`, the notices and all font OFL
  files to `dist/<preset>/licenses/`.
- Fonts: the 2008 freeware Sansation file (its terms forbid passing it on without its licence
  text) was replaced by Bernd Montag's OFL release of Sansation (v1.301, from Google Fonts).
  GeosansLight (Manfred Klein) may not be included in products without permission, so the
  message text now uses Sansation Light (owner's choice). Every bundled font is now OFL.
- **Still open:** the alarm icon, toolbar icons, tree picture and alarm sound come from the
  original 2010 project with no recorded origin and no author metadata in the files. They are
  flagged in the notices and the README; confirm their terms or replace them before a public
  release.

### H7. No `.gitattributes`
`git diff` warns "LF will be replaced by CRLF" for 14 files.
**Fix:** `.gitattributes` (`* text=auto eol=lf`, binary types) plus `.editorconfig`, then
renormalize.

**Resolution: fixed.** `.gitattributes` (LF for all text, independent of `core.autocrlf`; fonts,
images, icons and WAV marked binary) and `.editorconfig`. No renormalization was needed: all
text files were already LF in the index and working tree. The copied licence texts were
converted from CRLF to LF.

### H8. Unused assets of unknown licence are tracked
About 25 fonts, icons and images in `src/Resources` are no longer in `teaclock.qrc`.
**Fix:** remove them from the repository (history keeps them).

**Resolution: fixed.** 46 files not referenced by `teaclock.qrc` or `ttime.rc` were removed with
`git rm`: 16 fonts (including the replaced Sansation and GeosansLight files), 7 `.ico` files,
18 toolbar icons (including the replaced `play_16.png`) and 5 images.

## Medium

- **M1. UI intents routed through the model.** `TeaAlarm::close()` / `requestNewAlarm()` exist
  only so windows can talk to the manager; the manager also listens to the "Time's Up" window
  separately. Make `AlarmHolder` a QObject that owns the alarm and its current window and
  forwards `finished/closed/newAlarmRequested`; drop string IDs and `findHolder`.
  **Resolution: fixed.** `TeaAlarm` is a pure countdown model (no ID, no `close()` /
  `requestNewAlarm()`, `finished()` without arguments). The countdown window emits
  `quitRequested` / `newAlarmRequested`, the "Time's Up" window `dismissed` /
  `newAlarmRequested`. `AlarmHolder` is a QObject (child of the manager) that owns the alarm and
  its current window, switches to the "Time's Up" window itself and forwards `finished`,
  `closed` and `newAlarmRequested`. `TeaAlarmManager` only talks to holders: no string IDs,
  `findHolder` or `QUuid`; it emits `idle()` and `main` connects that to `quit()`.
- **M2. Lifetime relies on queued connections and connection order.** Use `deleteLater()` and
  save the window geometry in the window itself; add a lifecycle test (see T1).
  **Resolution: fixed (with M1).** Holders are disposed of with `deleteLater()`; the only
  remaining queued connection is the input window's `dismissed` (the window is still visible
  during its `closeEvent`). The countdown window saves its geometry in its destructor, so this
  no longer depends on connection order.
- **M3. Duplicated window code.** Both frameless windows paint the same gradient and build the
  same tray/menu boilerplate; extract a base class or a `paintThemedBackground()` helper.
  The 1 px border also clips by half a pixel on the right and bottom edges.
  **Resolution: fixed.** `paintPanel(QPainter&, QRect, Theme, PanelStyle)` in `theme.cpp` paints
  both windows; `PanelStyle` holds the differences (corner radius, side-gradient opacities, light
  band centred or towards the left). `src/window-utils.h` has `showAndActivate()` (was repeated
  4×) and `createTrayIcon()`. The outline is drawn on the pixel centres, so it is fully visible;
  apart from that outline the windows are pixel-identical to before.
- **M4. Static `QPixmap` cache outlives `QApplication`** (`theme.cpp`, latent bug). Use
  `QPixmapCache`, which Qt clears on exit.
  **Resolution: fixed.** `themedAppPixmap()` now uses `QPixmapCache` (key
  `ttime/app-icon/<theme>`); an evicted entry is rebuilt.
- **M5. 4 Hz updates.** `TeaAlarm` emits `timeLeftChanged` on every 250 ms tick, causing label,
  tray-icon and tooltip updates even when the displayed second did not change. Emit only on
  change.
  **Resolution: fixed.** `Alarm::reportTimeLeft()` emits only when the displayed second changes
  (`tst_alarm::reportsOnlyWhenTheDisplayedSecondChanges`).
- **M6. Tool-button size lock.** Alternative: a transparent 1 px border in the normal state.
  (Note from the owner's session: that alternative was tried and shifted the tuned layout by
  1–5 px, which is why the size lock was chosen.)
  **Resolution: won't fix.** Two style-only alternatives were tried and measured: a transparent
  1 px border in the normal state moves the tuned layout by 1–5 px; adding `margin: -1px`
  keeps the layout pixel-identical but Qt then draws the highlight border outside the button,
  where it is clipped (the checked "always on top" button lost its frame). The size lock keeps
  both, is documented in `countdown-window.cpp` and guarded by `tst_countdownwindow`.
- **M7. Naming.** `TeaAlarm`, `TeaAlarmProgressDialog` (not a QDialog), `TimerFinishedDialog`,
  `CreateAlarmDialog`, `Ui::…Class`, `teaclock.qrc`; UI mixes "New Timer" and "New Alarm",
  tooltip typo "alway on top", tool buttons have the text "PushButton" (read by screen
  readers). Suggested: `Alarm`, `AlarmManager`, `CountdownWindow`, `TimeUpWindow`,
  `NewAlarmWindow`, `ttime.qrc`.
  **Resolution: fixed.** `Alarm` (`alarm.*`), `AlarmManager` (`alarm-manager.*`),
  `CountdownWindow` (`countdown-window.*`), `TimeUpWindow` (`time-up-window.*`),
  `NewAlarmWindow` (`new-alarm-window.*`), `Ui::CountdownWindow` etc.; tests `tst_alarm`,
  `tst_countdownwindow` (tracked files moved with `git mv`). UI: "Always on top" (typo fixed),
  tool buttons named "Pause/Start", "Stop", "Restart", "Always on top", "Minimize", "Quit"
  (read by screen readers; icon-only, so nothing changes visually), "New Alarm" everywhere,
  window titles "tTime", "Time's up", "New alarm". The settings keys `[progress-dialog]` keep
  their old name so saved window positions still work.
- **M8. Presets.** `CMakePresets.json` hard-codes `C:/Qt`; `cmake_minimum_required(3.22)` vs
  presets requiring 3.25. Move machine paths to `CMakeUserPresets.json`.
  **Resolution: fixed.** The presets read the Qt installation from the `QT_ROOT` environment
  variable (set on the owner's machine with `setx QT_ROOT C:\Qt`); without it the `mingw-*`
  presets are disabled by a condition instead of failing later. Preset format 3 / CMake 3.22,
  matching `cmake_minimum_required`. `CMakeLists.txt` prints a clear message when Qt is not
  found.
- **M9. Tooling.** No `.clang-format`, `.clang-tidy` or CI. Suggested: GitHub Actions on Windows
  (MinGW) and Linux (offscreen) with sanitizers on Linux; warnings-as-errors in CI.
  **Resolution: fixed (CI not yet run).** `.clang-format` (Qt-like, 120 columns) applied to all
  sources and tests; `.clang-tidy` (bugprone, performance, modernize, selected readability and
  core-guidelines checks) with all 103 initial findings fixed or deliberately disabled
  (`bugprone-throwing-static-initialization`: global `QStringLiteral` constants;
  `bugprone-unchecked-optional-access` in tests only: `QVERIFY` guards) - 0 findings now.
  `TTIME_SANITIZE` CMake option (ASan + UBSan, not MinGW). `.github/workflows/ci.yml`: format
  check, Windows MinGW build (warnings as errors, tests, ZIP artifact), Linux build with
  sanitizers, offscreen tests and clang-tidy, on Qt 6.8 LTS. The workflow is validated as YAML
  but has not run yet: it runs on the first push to GitHub.
- **M10. Packaging.** No `VERSIONINFO` in `ttime.rc`, no CPack, no release tags.
  **Resolution: fixed, tag pending.** `src/ttime.rc.in` is configured with the project version
  (file and product version, description, copyright in Explorer). CPack builds
  `tTime-<version>-windows-x64.zip` (`--target package`) with the app, the Qt runtime, the
  shipped `tTime.ini` and the licences; verified by running tTime from the unpacked ZIP. A
  `v1.0.0` tag needs a commit first (command in the README).

## Tests

- **T1. No manager/alarm lifecycle test** (quit from the menu, dismiss, "New Alarm"); this is
  exactly where the original segfault was.
  **Resolution: fixed.** `tests/tst_alarmmanager.cpp` (11 cases) drives the real manager,
  holders and windows: quit through the real context menu (right-click, click "Quit") and the
  toolbar button, quitting one of two alarms, "New Alarm" from both windows, dismissing the
  "Time's Up" window, the window position being saved, and destroying the manager with
  running alarms. Verified that it catches the original bug: replacing `deleteLater()` with an
  immediate `delete` makes the test process crash in the context-menu case.
- **T2. `WindowSlot` untested**, and `tst_progressdialog` claims a real cascade slot in `%TEMP%`.
- **T3. Settings resolution untested** (user → shipped → built-in, invalid values).
  **Resolution: fixed** for the resolution order (`tst_appsettings`); the warnings for invalid
  values are still untested.
- **T4. GUI tests open real windows**; label them so CI can skip them or run offscreen.
- **T5. `tst_teaalarm` uses wall-clock waits** (fine for now; inject a clock if it flakes).
- **T6. `CreateAlarmDialog` validation untested.**

## Nice to have

- **N1.** Use plain `width/height/x/y` in the ini instead of `@Size(200 95)` / `@Point(…)`.
- **N2.** Turn the eight theme factory functions into one data table.
- **N3.** Stricter Qt defines (`QT_NO_CAST_FROM_ASCII`, `QT_NO_CAST_FROM_BYTEARRAY`,
  `QT_NO_FOREACH`, `QT_NO_CONTEXTLESS_CONNECT`).
- **N4.** `const TeaAlarmManager manager` in `main.cpp` is misleading; drop `const`. **Fixed** (with H3).
- **N5.** Put settings warnings under a logging category.
- **N6.** Split the long `BellAnimation` constructor; use scanlines; `std::numbers::pi`.
  **Partly fixed:** `std::numbers::pi`, a grid index helper instead of repeated casts,
  `std::iota`/`std::ranges::sort`; the constructor is not split yet.
- **N7.** Store a PNG copy of the icon so the `.ico` image plugin need not ship.
- **N8.** Place the "Time's Up" window where the countdown window was.
- **N9.** Keyboard shortcuts (Space = pause/resume, Esc = minimize) and accessible names.
  **Partly fixed:** tool buttons have real texts (accessible names, M7); no shortcuts yet.
- **N10.** Decide on i18n (English-only vs translatable) and apply `tr()` consistently.
- **N11.** README "Project layout" test description is outdated. **Fixed:** the layout now lists
  the tests, `licenses/`, `_docs/`, `LICENSE` and the notices.
- **N12.** Split the port into 3–4 commits.

## Changes made after the review (2026-11-06)

Scope: the four bugs (H1, H2, H5, M4) and the licensing items (H6, H7, H8).

| Item | Result | Files |
|------|--------|-------|
| H1 | Verified not a bug in Qt 6 (no change) | none |
| H2 | Explicit settings layer with tests | `src/app-settings.*`, `src/main.cpp`, `src/countdown-font.cpp`, `src/theme.cpp`, `src/tea-alarm-progress-dialog.cpp`, `tests/tst_appsettings.cpp` |
| H5 | Qt 6.7 required | `CMakeLists.txt`, `README.md` |
| M4 | `QPixmapCache` instead of a static `QHash` | `src/theme.cpp` |
| H6 | MIT licence, third-party notices, licence texts installed, all fonts OFL | `LICENSE`, `THIRD-PARTY-NOTICES.md`, `licenses/`, `src/Resources/fonts/Sansation-*`, `src/teaclock.qrc`, `src/main.cpp`, `src/tea-alarm-progress-dialog.cpp`, `CMakeLists.txt`, `README.md` |
| H7 | `.gitattributes`, `.editorconfig` | `.gitattributes`, `.editorconfig` |
| H8 | 46 unused assets removed | `src/Resources/**` |

Verification:
- Build with `-Werror`: 0 warnings. `ctest`: 12/12 suites pass (new: `tst_appsettings`).
- The deployed `dist/mingw-release` contains `licenses/` with `LICENSE`, `THIRD-PARTY-NOTICES.md`,
  GPL-3.0, LGPL-3.0, the GCC Runtime Library Exception, winpthreads and four font OFL files.
- Real app: `WM_QUERYENDSESSION` is answered "session may end"; the user file overrides the
  shipped defaults, and the shipped defaults apply when the user file lacks the key; the
  countdown message and the "Time's Up" heading render in the OFL Sansation fonts (screenshots
  compared before and after).

## Changes made for H3, H4 and M1 (2026-11-06)

| Item | Result | Files |
|------|--------|-------|
| H3 | Configuration read once into `AppConfig` and injected; settings readers are pure functions | `src/app-config.*` (new), `src/theme.*`, `src/countdown-font.*`, `src/main.cpp`, all windows |
| M1 | `AlarmHolder` QObject owns alarm + window; windows emit their own intents; `TeaAlarm` is a pure model | `src/alarm-holder.*`, `src/tea-alarm-manager.*`, `src/tea-alarm.*`, `src/tea-alarm-progress-dialog.*`, `src/timer-finished-dialog.*` |
| M2 | `deleteLater()` disposal; geometry saved in the window's destructor | as M1 |
| H4 | `ttime_core` / `ttime_gui` / `ttime_ui` libraries; resources via `qt_add_resources`; `Ui::` classes behind `unique_ptr` | `CMakeLists.txt`, `tests/CMakeLists.txt`, `src/*.ui`, `src/create-alarm-dialog.*`, `src/teaclock.qrc` (removed) |
| T1 | Lifecycle test | `tests/tst_alarmmanager.cpp` (new), `tests/tst_teaalarm.cpp`, `tests/tst_progressdialog.cpp` |

Verification:
- Build with `-Werror`: 0 warnings. `ctest`: 13/13 suites pass.
- `tst_alarmmanager` fails (the test process crashes) when holders are deleted immediately
  instead of with `deleteLater()`, so it guards the original bug.
- Real app: blue and hibiscus-red windows and taskbar icons as before (icon hues 205 / 338),
  "Time's Up" window works, the alarm sound plays its 3 loops (8.07 s), and the user's and the
  shipped `tTime.ini` were restored byte for byte after the checks.

## Changes made for M3 and M5-M10 (2026-11-06)

| Item | Result |
|------|--------|
| M3 | `paintPanel()` + `PanelStyle` for both windows; `showAndActivate()`, `createTrayIcon()` helpers; outline no longer half clipped |
| M5 | Remaining time reported only when the displayed second changes |
| M6 | Won't fix: both alternatives measured, both regress (layout shift / clipped highlight border) |
| M7 | Classes, files, `Ui::` classes and tests renamed; UI texts fixed; settings keys kept |
| M8 | Presets use `QT_ROOT` (set for the owner); disabled without it; CMake 3.22 throughout |
| M9 | `.clang-format` applied, `.clang-tidy` at 0 findings, `TTIME_SANITIZE`, GitHub Actions CI |
| M10 | VERSIONINFO from the project version; CPack ZIP; tag after the first commit |

Verification:
- Fresh build tree with the presets and `QT_ROOT`; build with `-Werror`: 0 warnings;
  `ctest`: 13/13 suites pass. clang-format check clean; clang-tidy 0 findings.
- Countdown window pixel-identical to before apart from the outline; "Time's Up" window
  identical apart from the outline and the "New Alarm" button text.
- tTime run from the unpacked release ZIP: "Time's Up" window shown in the user's theme, the
  alarm sound plays 3 loops (8.06 s), exit code 0; `tTime.exe` shows version 2.0.0, product
  name, description and copyright in its file properties.
- Not verified locally: the GitHub Actions workflow (YAML validated; it first runs on push).


Note (2026-11-06): the version was later set to 1.0.0, the first official release (the Qt 4
project never had a version number). The checks above were made with 2.0.0.
