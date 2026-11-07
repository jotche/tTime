# tTime 0.2.2

Countdown alarms ("tea timer") that live in the Windows system tray.
Type `5m green tea`, and a small translucent window counts down. When time is
up, a sound plays, the bell in the tray and taskbar icon swings and the tree in the
"Time's Up" window sways in a light breeze until you dismiss it (`animations=off` in
`tTime.ini` keeps everything still).

This is the **0.2 line**, built with **Qt 4.7** and **qmake** (C++98). It carries the
code, layout and design of tTime 1.x (themes, fonts, animations, window layout, settings,
tests) without the move to Qt 6 / CMake and without the CI and packaging setup.

## Requirements (Windows)

- **Qt 4.7.4 for MinGW** (`qt-win-opensource-4.7.4-mingw.exe` from
  <https://download.qt.io/archive/qt/4.7/>) and the **MinGW compiler it was built with**
  (the Qt SDK's MinGW, GCC 4.4). Qt 4.8 works as well, with its matching MinGW
  (`qt-opensource-windows-x86-mingw482-4.8.7.exe` + MinGW-w64 GCC 4.8.2).
- Qt modules used: `QtCore`, `QtGui` (widgets, tray icon and `QSound` for the alarm) and
  `QtTest` (unit tests).

The code uses only C++98 and Qt 4.7 APIs.

## Build, test, run

From a command prompt with Qt's `bin` and MinGW's `bin` on `PATH` (for example the
"Qt 4.7.4 Command Prompt"), preferably in a separate build directory:

```bat
mkdir build && cd build
qmake ..\tTime.pro -r
mingw32-make
mingw32-make check                    :: runs the unit tests
mingw32-make install INSTALL_ROOT=%CD%\..\dist   :: tTime.exe, tTime.ini, Resources, licences

..\dist\tTime.exe                     :: opens the "new alarm" input
..\dist\tTime.exe 5m green tea
..\dist\tTime.exe 10s test            :: quick test: rings after 10 s
..\dist\tTime.exe --help
```

Qt Creator: open `tTime.pro`.

`mingw32-make install` lays out a complete installation:

```
tTime.exe
tTime.ini                     shipped default settings
Resources\sounds\submarine1.wav   the alarm sound
licenses\
```

Fonts, icons and pictures are compiled into the exe, so no Qt plugins are needed. Files tTime
cannot use from its compiled-in resources live in the `Resources` folder next to `tTime.exe`
(`externalResourcePath()` in `src/app-config.h`): the alarm sound is one, because `QSound` plays
files only. `src\tTime.exe` in the build tree has no `Resources` folder next to it, so it runs
silently (it logs "Alarm sound not found"); run the installed copy to hear the alarm.

To run tTime on another machine, put these next to `tTime.exe` (from Qt's and MinGW's `bin`):
`QtCore4.dll`, `QtGui4.dll` and the MinGW runtime (`mingwm10.dll` and
`libgcc_s_dw2-1.dll` for GCC 4.4; `libgcc_s_dw2-1.dll`, `libstdc++-6.dll` and
`libwinpthread-1.dll` for MinGW-w64), plus the installation above.

### Static build (one exe, no DLLs)

Qt 4 has no static binaries; build a static Qt once from the source
(`qt-everywhere-opensource-src-4.7.4.zip`), only the parts tTime uses:

```bat
configure.exe -static -release -opensource -confirm-license -platform win32-g++ ^
  -no-qt3support -no-opengl -no-openvg -no-openssl -no-dbus -no-webkit -no-phonon ^
  -no-phonon-backend -no-multimedia -no-audio-backend -no-script -no-scripttools ^
  -no-declarative -no-xmlpatterns -no-sql-sqlite -no-libtiff -no-libmng -no-libjpeg -no-gif ^
  -qt-libpng -qt-zlib -no-style-plastique -no-style-cleanlooks -no-style-motif -no-style-cde ^
  -nomake examples -nomake demos -nomake docs -nomake tools -no-native-gestures
for %t in (sub-tools-bootstrap sub-moc sub-rcc sub-uic sub-winmain sub-corelib sub-gui sub-testlib) do mingw32-make %t
```

Then build tTime with that Qt's `qmake` as above. Against a static Qt, `common.pri` also links
the MinGW runtime statically, so `tTime.exe` (about 12 MB) needs only Windows' own DLLs; with
`tTime.ini`, `Resources` and `licenses` it is the whole distribution
(`tTime-<version>-windows-<arch>-static.zip`).

Built and tested with MinGW-w64 GCC 4.8.2, posix threads, rev3:

| Package | Compiler | Qt source |
|---------|----------|-----------|
| `windows-x86-static` | `i686-4.8.2-release-posix-dwarf-rt_v3-rev3` | Qt 4.7.4 as released |
| `windows-x64-static` | `x86_64-4.8.2-release-posix-seh-rt_v3-rev3` | Qt 4.7.4 with `patches/qt-4.7.4-mingw-x64.patch` |

Qt 4.7 predates 64-bit MinGW support: the patch (one line, as fixed in Qt 4.8) makes QtCore's
CPU feature detection use GCC's inline assembly instead of the MSVC-only `__cpuid`. Apply it in
the Qt source directory before building: `patch -p1 < patches/qt-4.7.4-mingw-x64.patch`.

The version is `VERSION` in `common.pri` (also in `src/ttime.rc`, which puts it into the file
properties of `tTime.exe`).

## Fonts

The countdown and the alarm message each use a configurable font (`countdown-font` and
`message-font`; the message font is used in both the countdown and the "Time's Up" window); the
other text uses Sansation. All fonts use the SIL Open Font License 1.1 and are compiled into
`tTime.exe`.

Qt 4 cannot select a weight from a variable font, so each weight is bundled as a static font
file (`src/Resources/fonts/<Font>-<Style>.ttf`, the upstream static releases). A configured
weight is rounded to the nearest hundred and clamped to the font's range:

| Font id | Font | Weights | Source |
|---------|------|---------|--------|
| `oxanium` (countdown default) | Oxanium | 200-800 | <https://github.com/sevmeyer/oxanium> |
| `jura` | Jura | 300-700 | <https://github.com/ossobuffo/jura> |
| `doto` | Doto (dot matrix) | 900 (Black) | <https://github.com/oliverlalan/Doto> |
| `outfit` (message default) | Outfit | 100-900 | <https://github.com/Outfitio/Outfit-Fonts> |
| `sansation` | Sansation | 300-400 | <https://github.com/google/fonts/tree/main/ofl/sansation> |

## Settings

Settings are read from two files. The first one that has a setting wins:

1. `%APPDATA%\tTime\tTime.ini`: your own settings. tTime also stores the window size and
   position here.
2. `tTime.ini` next to `tTime.exe` (from `src/tTime.ini`): the documented defaults for
   everyone using this copy of tTime. tTime never writes to it.

If neither file has a setting, the built-in defaults apply. The shipped file contains:

```ini
[appearance]
; oxanium, jura, doto, outfit or sansation
countdown-font=oxanium
; 100-900, or a name: thin, extralight, light, regular, medium, semibold, bold, extrabold, black
countdown-weight=400
; font and weight of the message below the countdown (same choices)
message-font=outfit
message-weight=300
; font sizes in points (6-96): countdown, message below it, message in the "Time's Up" window
countdown-size=18
message-size=13
time-up-message-size=13
; look of the windows and icon: blue, sage, prussian-blue, hibiscus-red,
; jade-green, forest-green, pine-green or hunter-green
theme=blue
; "Time's Up" window: tree swaying in a light breeze and ringing tray icon (on or off)
animations=on
```

Comments must be on their own line: tTime.ini does not support comments after a value.
tTime reads these settings when it starts, so changes apply the next time it starts. The
built-in defaults are the `default*` constants in `src/countdown-font.h` and `src/theme.h`.

## Usage

* Duration syntax: `[N h] [N m|min] [N s|sec] [message]`, for example `90s`,
  `1h30m`, `3 min sencha`, `1 hour 2 minutes`.
* Drag the countdown window with the left mouse button. **Ctrl+drag** changes its width; the
  height always follows the content.
* Right-click the window or its tray icon to restart, minimise, create a new alarm, or quit.
* Double-click the tray icon to show or hide the window.
* Window size and position are remembered in `%APPDATA%\tTime\tTime.ini`.
* Several alarms open at once (in one or several tTime processes) are cascaded
  32 px right/down from the remembered position. Near a screen edge, that axis
  moves left/up instead. The cascade offset is never saved, but a window you drag
  yourself saves its new position.
* The app exits when the last alarm is closed and the input window is not open.

## Project layout

```
tTime.pro               qmake project (subdirs: library, app, tests)
common.pri              version and settings shared by all projects
src/ttime-lib.pro       everything but main(), as a static library
src/ttime-app.pro       tTime.exe (main.cpp, resources, Windows .rc)
src/                    sources, .ui forms, resources (ttime.qrc, src/Resources),
                        shipped default settings (tTime.ini)
tests/                  Qt Test suites, one project per test (tests/<name>/<name>.pro)
licenses/               licence texts installed with the app
LICENSE                 tTime's licence (MIT)
THIRD-PARTY-NOTICES.md  what is bundled and under which licence
```

At startup `main.cpp` reads the settings once into an `AppConfig` (theme, fonts, alarm
sound) and passes it, with `AppSettings` for the window position, to `AlarmManager`. The
manager owns one `AlarmHolder` per alarm; a holder owns the alarm (`Alarm`) and its current
window (`CountdownWindow`, then `TimeUpWindow`), and reports `finished`, `closed` and
`newAlarmRequested`.

Differences from tTime 1.x that come from Qt 4:

- The alarm sound plays with `QSound`, which needs a file, so it is shipped in
  `Resources\sounds` next to `tTime.exe` instead of being compiled in.
- Windows are moved and resized (Ctrl+drag) by tTime itself, not by the window system.
- Cascade slots across processes use named shared memory (`QSharedMemory`) instead of lock
  files.
- The app icon resource is a PNG (`src/Resources/icons/alarm.png`, the 128 px image of
  `alarm.ico`), so no image plugin is needed; `alarm.ico` is still the exe icon.
- `--help` and `--version` show a message box (tTime has no console).

## Code style

`.clang-format` (Qt-like style, 120 columns) and `.editorconfig` / `.gitattributes` (UTF-8,
LF line endings, 4-space indentation).

## License

tTime is released under the MIT License (see `LICENSE`). Distributions also contain Qt
(LGPL 2.1 with the Nokia Qt LGPL Exception 1.1), the MinGW runtime and the bundled fonts (SIL
Open Font License 1.1); see `THIRD-PARTY-NOTICES.md`. The origin and licence of the icons, the
tree picture and the alarm sound from the original 2010 project are not recorded; confirm or
replace them before a public release.
