# tTime

Countdown alarms ("tea timer") that live in the Windows system tray.
Type `5m green tea`, and a small translucent window counts down. When time is
up, a sound plays, the bell in the tray and taskbar icon swings and the tree in the
"Time's Up" window sways in a light breeze until you dismiss it (`animations=off` in
`tTime.ini` keeps everything still).

Built with **Qt 6 (community / LGPL)**, **C++20** and **CMake**. Requires Qt 6.7 or newer;
developed and tested with Qt 6.12.

## Requirements (Windows)

All tools are installed under `C:\Qt`, in the same layout the official Qt
online installer uses:

| Component            | Location                              |
|----------------------|---------------------------------------|
| Qt 6.12.0, MinGW 64  | `C:\Qt\6.12.0\mingw_64`               |
| MinGW 13.1 (g++)     | `C:\Qt\Tools\mingw1310_64`            |
| CMake 3.30           | `C:\Qt\Tools\CMake_64`                |
| Ninja 1.12           | `C:\Qt\Tools\Ninja`                   |

Qt modules used: `Core`, `Gui`, `Widgets`, `Multimedia` (alarm sound), and `Test` (unit tests).

> To set this up on another machine, use the Qt Online Installer
> (<https://www.qt.io/download-qt-installer-oss>). Select *Qt 6.12.0 → MinGW 13.1
> 64-bit* and *Qt Multimedia*, plus *Developer and Designer Tools → MinGW 13.1,
> CMake, Ninja*.

The `mingw-*` presets find Qt, MinGW and Ninja through the `QT_ROOT` environment variable
(the folder the Qt installer used). Set it once, then open a new terminal:

```powershell
setx QT_ROOT C:\Qt
```

Without `QT_ROOT` the `mingw-*` presets are disabled (`cmake --list-presets` shows only
`default`).

## Build, test, run

From a PowerShell prompt in the repository root:

```powershell
$env:PATH = "C:\Qt\Tools\CMake_64\bin;$env:PATH"   # only cmake is needed on PATH

cmake --preset mingw-release          # configure (presets set compiler/Qt/Ninja paths)
cmake --build --preset mingw-release  # build
ctest --preset mingw-release          # run unit tests
cmake --install build/mingw-release   # deploy: exe + Qt DLLs/plugins -> dist/mingw-release
cmake --build --preset mingw-release --target package   # ZIP -> build/mingw-release/packages

.\dist\mingw-release\bin\tTime.exe            # opens the "new alarm" input
.\dist\mingw-release\bin\tTime.exe 5m green tea
.\dist\mingw-release\bin\tTime.exe 10s test   # quick test: rings after 10 s
```

Use `mingw-debug` instead of `mingw-release` for a debug build.
`dist\mingw-release` is self-contained, and the `package` target zips the same content as
`tTime-<version>-windows-x64.zip`; it runs on any Windows 10/11 x64 machine. The version comes
from `project(tTime VERSION ...)` in `CMakeLists.txt` and is also shown in the file properties of
`tTime.exe`. After committing a release, tag it: `git tag -a v1.0.0 -m "tTime 1.0.0"`.
It contains only the Qt parts tTime uses: about 39 MB on disk, about 16 MB zipped.
To deploy the full Qt runtime instead (OpenGL fallback, FFmpeg, translations, all
plugins), configure with `-DTTIME_MINIMAL_DEPLOY=OFF`.

### Release packages (Windows x64 and macOS arm64)

macOS packages can only be built on a Mac, so both packages are built by the **Release**
workflow (`.github/workflows/release.yml`) on GitHub Actions:

- **Actions -> Release -> Run workflow** builds, tests and packages both platforms and keeps
  `tTime-<version>-windows-x64.zip` and `tTime-<version>-macos-arm64.dmg` as workflow artifacts.
- Pushing a version tag (`git push origin v1.0.0`) does the same and attaches both files to a
  GitHub release for that tag.

On a Mac (Apple silicon, Qt 6.7+ for macOS, Xcode command line tools, CMake, Ninja) the DMG can
also be built locally:

```sh
cmake -S . -B build/mac -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=~/Qt/6.8.3/macos -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build/mac && ctest --test-dir build/mac && cmake --build build/mac --target package
```

The DMG holds `tTime.app` (Qt frameworks inside, default `tTime.ini` in
`Contents/Resources`), an Applications link and the `licenses` folder; user settings are kept in
`~/.config/tTime/tTime.ini`. It needs macOS 12 or newer. The app is signed ad hoc, not with an
Apple developer certificate, and is not notarized, so macOS blocks the first start: open it with
right-click -> **Open** (or *System Settings -> Privacy & Security -> Open Anyway*), or run
`xattr -dr com.apple.quarantine /Applications/tTime.app`. A Developer ID signature and
notarization would remove that step. On macOS the tray icon opens its menu on click (there is no
double-click); use **Restore** there to show a hidden countdown window.

The other text uses Sansation (Regular and Light, SIL Open Font License 1.1).
Bundled countdown fonts are variable fonts, so any weight in their range works. They use the SIL
Open Font License 1.1, are compiled into `tTime.exe`, and their licences are installed to
`dist/<preset>/licenses/`:

| `countdown-font` | Font | Weights | Source |
|------------------|------|---------|--------|
| `oxanium` (default) | Oxanium | 200-800 | <https://github.com/sevmeyer/oxanium> |
| `jura` | Jura | 300-700 | <https://github.com/ossobuffo/jura> |
| `doto` | Doto (dot matrix) | 100-900 | <https://github.com/oliverlalan/Doto> |

Settings are read from two files. The first one that has a setting wins:

1. `%APPDATA%\tTime\tTime.ini`: your own settings. tTime also stores the window size and
   position here.
2. `tTime.ini` next to `tTime.exe` (`dist/<preset>/bin/tTime.ini`, from `src/tTime.ini`): the
   documented defaults for everyone using this copy of tTime. tTime never writes to it.

If neither file has a setting, the built-in defaults apply. The shipped file contains:

```ini
[appearance]
; oxanium, jura or doto
countdown-font=oxanium
; 100-900, or a name: thin, extralight, light, regular, medium, semibold, bold, extrabold, black
countdown-weight=400
; look of the windows and icon: blue, sage, prussian-blue, hibiscus-red,
; jade-green, forest-green, pine-green or hunter-green
theme=blue
; "Time's Up" window: tree swaying in a light breeze and ringing tray icon (on or off)
animations=on
```

Comments must be on their own line: tTime.ini does not support comments after a value.

Weights outside a font's range are clamped to the nearest supported one. `theme` sets the look of the
countdown and "Time's Up" windows and the button highlights. tTime reads these settings when it
starts, so changes apply the next time tTime starts. The built-in defaults are `defaultCountdownFontId` and
`defaultCountdownWeight` in `src/countdown-font.h`.

**Qt Creator / VS Code / CLion:** open the folder (or `CMakeLists.txt`) and pick
the *Qt 6.12 MinGW 64-bit* preset.

To run the non-deployed exe straight from `build\`, put
`C:\Qt\6.12.0\mingw_64\bin` and `C:\Qt\Tools\mingw1310_64\bin` on `PATH` first.

## Usage

* Duration syntax: `[N h] [N m|min] [N s|sec] [message]`, for example `90s`,
  `1h30m`, `3 min sencha`, `1 hour 2 minutes`.
* Drag the countdown window with the left mouse button. **Ctrl+drag** resizes it.
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
CMakeLists.txt          build definition (app, core library, install/deploy, tests)
CMakePresets.json       ready-made configure/build/test presets
src/                    sources, .ui forms, assets (src/Resources), shipped default
                        settings (tTime.ini)
tests/                  Qt Test suites (logic, settings, themes, widgets)
licenses/               licence texts installed with the app
_docs/                  project documentation (code review)
LICENSE                 tTime's licence (MIT)
THIRD-PARTY-NOTICES.md  what is bundled and under which licence
```

The build has three static libraries, which the app (`src/main.cpp` only) and the tests link:

| Library | Contents | Qt modules |
|---------|----------|------------|
| `ttime_core` | alarm model, parser, settings, layout helpers | Core |
| `ttime_gui` | themes, icon and bell animation, tree breeze, fonts, `AppConfig`, resources | Gui |
| `ttime_ui` | windows, `AlarmHolder`, `AlarmManager` | Widgets, Multimedia |

Resources (fonts, icons, the tree picture, the sound) are listed in `CMakeLists.txt`
(`ttime_add_resources`) under their `:/fonts/...`, `:/icons/...` aliases.

At startup `main.cpp` reads the settings once into an `AppConfig` (theme, countdown font, alarm
sound) and passes it, with `AppSettings` for the window position, to `AlarmManager`. The
manager owns one `AlarmHolder` per alarm; a holder owns the alarm (`Alarm`) and its current
window (`CountdownWindow`, then `TimeUpWindow`), and reports `finished`, `closed` and
`newAlarmRequested`.

CMake options: `-DTTIME_BUILD_TESTS=OFF`, `-DTTIME_WARNINGS_AS_ERRORS=ON`, `-DTTIME_MINIMAL_DEPLOY=OFF`,
`-DTTIME_SANITIZE=ON` (AddressSanitizer + UBSan; GCC/Clang on Linux/macOS, not MinGW).

## Code style and checks

- **Formatting:** `.clang-format` (Qt-like style, 120 columns). Format a file with
  `clang-format -i <file>`; check everything with
  `clang-format --dry-run --Werror src/*.cpp src/*.h tests/*.cpp`.
- **Static analysis:** `.clang-tidy` (bugprone, performance, modernize and selected readability
  checks; `tests/.clang-tidy` relaxes one check for `QVERIFY`-guarded code). With MinGW:
  `clang-tidy -p build/mingw-release --extra-arg=--target=x86_64-w64-mingw32 --extra-arg=--sysroot=%QT_ROOT%/Tools/mingw1310_64 <file>`.
- Both tools are available from pip (`pip install clang-format==23.1.5 clang-tidy==23.1.4`).
- `.editorconfig` and `.gitattributes` set UTF-8, LF line endings and 4-space indentation.
- **CI** (`.github/workflows/ci.yml`, GitHub Actions): formatting; Windows MinGW build with
  warnings as errors, tests and the ZIP as an artifact; Linux build with sanitizers, offscreen
  tests and clang-tidy. The Release workflow (`release.yml`) builds the Windows ZIP and the
  macOS arm64 DMG. Both use Qt 6.8 LTS (the installer tooling for CI cannot install the
  Qt 6.12 layout yet).

Sound diagnostics: set `QT_LOGGING_RULES=ttime.sound.debug=true` to log alarm playback.

## License

tTime is released under the MIT License (see `LICENSE`). Distributions also contain Qt (LGPL-3.0),
the MinGW-w64 runtime and the bundled fonts (SIL Open Font License 1.1); see
`THIRD-PARTY-NOTICES.md`. The origin and licence of the icons, the tree picture and the alarm
sound from the original 2010 project are not recorded; confirm or replace them before a public
release.
