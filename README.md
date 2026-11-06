# tTime

Countdown alarms ("tea timer") that live in the Windows system tray.
Type `5m green tea`, and a small translucent window counts down. When time is
up, a sound plays, the bell in the tray and taskbar icon swings and the tree in the
"Time's Up" window sways in a light breeze until you dismiss it (`animations=off` in
`tTime.ini` keeps everything still).

Built with **Qt 6 (community / LGPL)**, **C++20** and **CMake**. Requires Qt 6.7 or newer;
developed and tested with Qt 6.12.

## Requirements (Windows)

tTime is built in the **MSYS2 CLANG64** environment (Clang, LLVM's libc++, Universal CRT):

1. Install MSYS2 (<https://www.msys2.org/>, or `winget install MSYS2.MSYS2`) to `C:\msys64`.
2. In an MSYS2 shell, install the toolchain and Qt (Qt 6.12, shared and static):

   ```sh
   pacman -Syu
   pacman -S --needed mingw-w64-clang-x86_64-{clang,lld,cmake,ninja,pkgconf,qt6-base,qt6-multimedia,qt6-tools,qt6-static}
   ```

3. The `clang64-*` presets find MSYS2 through the `MSYS2_ROOT` environment variable. Set it
   once, then open a new terminal:

   ```powershell
   setx MSYS2_ROOT C:\msys64
   ```

Without `MSYS2_ROOT` the `clang64-*` presets are disabled (`cmake --list-presets` shows only
`default`).

Qt modules used: `Core`, `Gui`, `Widgets`, `Multimedia` (alarm sound), and `Test` (unit tests).

## Build, test, run

There are three presets:

| Preset | Qt | Use |
|--------|----|-----|
| `clang64-release-static` | static (`qt6-static`) | **the release build**: one `tTime.exe`, no DLLs |
| `clang64-release` | shared | development; deploys ~125 files (~200 MB) |
| `clang64-debug` | shared | debugging |

From a PowerShell prompt in the repository root:

```powershell
$env:PATH = "C:\msys64\clang64\bin;$env:PATH"   # cmake, clang++, ninja

cmake --preset clang64-release-static           # configure (presets set compiler/Qt paths)
cmake --build --preset clang64-release-static   # build
ctest --preset clang64-release-static           # run unit tests
cmake --install build/clang64-release-static    # exe, tTime.ini, licences -> dist/clang64-release-static
cmake --build --preset clang64-release-static --target package   # tTime-<version>-windows-x64.zip

.\dist\clang64-release-static\bin\tTime.exe            # opens the "new alarm" input
.\dist\clang64-release-static\bin\tTime.exe 5m green tea
.\dist\clang64-release-static\bin\tTime.exe 10s test   # quick test: rings after 10 s
```

The static build links Qt, its third-party libraries (FreeType, HarfBuzz, libpng, zlib, PCRE2,
OpenSSL's libcrypto, ...) and the C++ runtime into `tTime.exe` (about 30 MB, 12.5 MB zipped). It
needs only Windows' own DLLs (including the Universal CRT that ships with Windows 10/11), so
the ZIP runs on any Windows 10/11 x64 machine. It plays the alarm through Qt's native Windows
media backend. The version comes from `project(tTime VERSION ...)` in `CMakeLists.txt` and is
also shown in the file properties of `tTime.exe`. After committing a release, tag it:
`git tag -a v1.0.0 -m "tTime 1.0.0"`.

**Shared builds** (`clang64-release`, `clang64-debug`) are for development. MSYS2's shared
Qt Multimedia only has the FFmpeg backend, so `cmake --install` deploys Qt, FFmpeg and their
MSYS2 DLLs (about 125 files, 200 MB) next to the exe. That folder runs without MSYS2, but do not
distribute it: MSYS2's FFmpeg includes GPL codecs (x264, x265, Xvid), and the licences of those
DLLs are not collected. Its `package` target is named `tTime-<version>-windows-x64-shared.zip`.
`-DTTIME_MINIMAL_DEPLOY=OFF` deploys the full Qt runtime (OpenGL fallback, translations, all
plugins).

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

The countdown and the alarm message each use a configurable font (`countdown-font` and
`message-font`; the message font is used in both the countdown and the "Time's Up" window); the
other text uses Sansation. All fonts use the SIL Open Font License 1.1, are
compiled into `tTime.exe`, and their licences are installed to `dist/<preset>/licenses/`. Oxanium,
Jura, Doto and Outfit are variable fonts, so any weight in their range works; Sansation is bundled as
Light and Regular. Both settings accept every font:

| Font id | Font | Weights | Source |
|---------|------|---------|--------|
| `oxanium` (countdown default) | Oxanium | 200-800 | <https://github.com/sevmeyer/oxanium> |
| `jura` | Jura | 300-700 | <https://github.com/ossobuffo/jura> |
| `doto` | Doto (dot matrix) | 100-900 | <https://github.com/oliverlalan/Doto> |
| `outfit` (message default) | Outfit | 100-900 | <https://github.com/Outfitio/Outfit-Fonts> |
| `sansation` | Sansation | 300-400 | <https://github.com/google/fonts/tree/main/ofl/sansation> |

Settings are read from two files. The first one that has a setting wins:

1. `%APPDATA%\tTime\tTime.ini`: your own settings. tTime also stores the window size and
   position here.
2. `tTime.ini` next to `tTime.exe` (`dist/<preset>/bin/tTime.ini`, from `src/tTime.ini`): the
   documented defaults for everyone using this copy of tTime. tTime never writes to it.

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

Weights outside a font's range are clamped to the nearest supported one. `theme` sets the look of the
countdown and "Time's Up" windows and the button highlights. tTime reads these settings when it
starts, so changes apply the next time tTime starts. The built-in defaults are `defaultCountdownFontId`,
`defaultCountdownWeight`, `defaultMessageFontId`, `defaultMessageWeight` and the `default*PointSize`
constants in `src/countdown-font.h`.

**Qt Creator / VS Code / CLion:** open the folder (or `CMakeLists.txt`) and pick one of
the *MSYS2 CLANG64* presets.

The static exe in `build\clang64-release-static` runs as is. To run a shared build straight
from `build\`, put `C:\msys64\clang64\bin` on `PATH` first.

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
cmake/                  deploy-dlls.cmake (copies MSYS2 DLLs in a shared install)
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
`-DTTIME_STATIC=ON` (link everything into one exe; needs a static Qt, set by the
`clang64-release-static` preset), `-DTTIME_SANITIZE=ON` (AddressSanitizer + UBSan; GCC/Clang on
Linux/macOS, not Windows).

## Code style and checks

- **Formatting:** `.clang-format` (Qt-like style, 120 columns). Format a file with
  `clang-format -i <file>`; check everything with
  `clang-format --dry-run --Werror src/*.cpp src/*.h tests/*.cpp`.
- **Static analysis:** `.clang-tidy` (bugprone, performance, modernize and selected readability
  checks; `tests/.clang-tidy` relaxes one check for `QVERIFY`-guarded code). With CLANG64:
  `clang-tidy -p build/clang64-release <file>` (MSYS2 has it in `mingw-w64-clang-x86_64-clang-tools-extra`).
- Both tools are available from pip (`pip install clang-format==23.1.5 clang-tidy==23.1.4`).
- `.editorconfig` and `.gitattributes` set UTF-8, LF line endings and 4-space indentation.
- **CI** (`.github/workflows/ci.yml`, GitHub Actions): formatting; Windows MSYS2 CLANG64
  build (shared Qt 6.12) with warnings as errors and tests; Linux build with sanitizers,
  offscreen tests and clang-tidy. The Release workflow (`release.yml`) builds the static
  Windows ZIP with MSYS2 CLANG64 and the macOS arm64 DMG. Linux and macOS use Qt 6.8 LTS (the
  installer tooling for CI cannot install the Qt 6.12 layout yet); Windows gets Qt 6.12 from
  MSYS2.

Sound diagnostics: set `QT_LOGGING_RULES=ttime.sound.debug=true` to log alarm playback.

## License

tTime is released under the MIT License (see `LICENSE`). Distributions also contain Qt (LGPL-3.0),
the libraries linked into the static exe (FreeType, HarfBuzz, OpenSSL, LLVM runtime, ...) and
the bundled fonts (SIL Open Font License 1.1); see
`THIRD-PARTY-NOTICES.md`. The origin and licence of the icons, the tree picture and the alarm
sound from the original 2010 project are not recorded; confirm or replace them before a public
release.
