# Third-party notices

tTime itself is licensed under the MIT License (see `LICENSE`). A tTime distribution
(`cmake --install`) also contains the third-party components below. Their licence texts are
installed in the `licenses` folder next to `bin` (Windows) or next to `tTime.app` in the DMG
(macOS).

## Qt 6 (The Qt Company Ltd. and other contributors)

- **Modules shipped:** Qt Core, Gui, Widgets, Multimedia and Network as DLLs in `bin`, plus
  plugins in `plugins` (platforms, styles, imageformats, multimedia).
- **Licence:** GNU Lesser General Public License v3 (`licenses/LGPL-3.0.txt`), which builds on
  the GNU General Public License v3 (`licenses/GPL-3.0.txt`).
- **Source code:** Qt's source code is available from <https://download.qt.io/> and
  <https://code.qt.io/>. tTime uses the unmodified Qt 6.12.0 community release.
- **Replacing Qt:** the standard tTime distribution links Qt dynamically. You can replace the
  Qt DLLs and plugins in it with your own build of the same Qt version.

### Static build (`tTime-<version>-windows-x64-static.zip`)

The static build (`mingw-release-static` preset) has Qt Core, Gui, Widgets, Multimedia and
Network, and the Qt plugins it uses, linked into `tTime.exe`, so it ships no Qt DLLs. Qt is
still under the LGPL v3 there. To use a modified Qt with it, as section 4 of the LGPL v3
allows: tTime's complete source code is available under the MIT License, and
`scripts/build-static-qt.ps1` plus the `mingw-release-static` preset rebuild and relink
`tTime.exe` against your own static Qt build. The Qt in this build is the unmodified
Qt 6.12.0 community release (qtbase, qtshadertools, qtmultimedia), configured with
`-static -static-runtime -release` and built without FFmpeg.

With the default minimal deploy (`TTIME_MINIMAL_DEPLOY=ON`), FFmpeg is **not** shipped. If you
build with `-DTTIME_MINIMAL_DEPLOY=OFF`, the FFmpeg libraries (`av*.dll`, `sw*.dll`) are included
as well. They are licensed under the GNU LGPL v2.1 or later; source is available from
<https://ffmpeg.org/>.

### macOS

The macOS app bundle carries the same Qt modules as frameworks in `tTime.app/Contents/Frameworks`
and the plugins in `Contents/PlugIns`, under the same licence. macdeployqt cannot leave plugins
out, so the bundle also contains Qt Multimedia's FFmpeg backend and the FFmpeg libraries it
plays sound with (GNU LGPL v2.1 or later; source from <https://ffmpeg.org/>, and the Qt build of
FFmpeg is described at <https://doc.qt.io/qt-6/qtmultimedia-index.html>).

## MinGW-w64 runtime (Windows only)

- **GCC runtime** (`libstdc++-6.dll`, `libgcc_s_seh-1.dll`): GNU GPL v3 with the GCC Runtime
  Library Exception v3.1 (`licenses/GPL-3.0.txt`,
  `licenses/GCC-Runtime-Library-Exception-3.1.txt`). The exception allows distributing them
  with programs under any licence.
- **winpthreads** (`libwinpthread-1.dll`): MIT-style licence (`licenses/winpthreads.txt`).

The static build ships no MinGW DLLs: their code is linked into `tTime.exe` instead. The GCC
Runtime Library Exception covers that as well, and winpthreads' licence only asks for its
notice in the documentation, which this section is.

## Fonts (compiled into `tTime.exe`)

All bundled fonts are under the SIL Open Font License 1.1:

| Font | Copyright | Licence file |
|------|-----------|--------------|
| Oxanium | 2019 The Oxanium Project Authors (<https://github.com/sevmeyer/oxanium>) | `licenses/Oxanium-OFL.txt` |
| Jura | 2019 The Jura Project Authors (<https://github.com/ossobuffo/jura>) | `licenses/Jura-OFL.txt` |
| Doto | 2024 The Doto Project Authors (<https://github.com/oliverlalan/Doto>) | `licenses/Doto-OFL.txt` |
| Outfit | 2021 The Outfit Project Authors (<https://github.com/Outfitio/Outfit-Fonts>) | `licenses/Outfit-OFL.txt` |
| Sansation (Regular, Light) | 2011 Bernd Montag, Reserved Font Name "Sansation" (OFL release via <https://github.com/google/fonts/tree/main/ofl/sansation>) | `licenses/Sansation-OFL.txt` |

## Images and sound (compiled into `tTime.exe`)

The alarm icon (`alarm.ico`), the toolbar icons (`icons01/*.png`), the tree picture
(`tree02.png`) and the alarm sound (`submarine1.wav`) come from the original 2010 tTime
project. **Their origin and licence were not recorded**, and the files contain no author
metadata. Confirm their terms, or replace them, before distributing tTime publicly.
`play_matched_16.png` was drawn for tTime and falls under tTime's MIT licence.
