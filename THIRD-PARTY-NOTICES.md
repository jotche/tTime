# Third-party notices

tTime itself is licensed under the MIT License (see `LICENSE`). A tTime distribution
(`cmake --install`) also contains the third-party components below. Their licence texts are
installed in the `licenses` folder next to `bin` (Windows) or next to `tTime.app` in the DMG
(macOS).

## Windows: `tTime-<version>-windows-x64.zip` (static build)

The Windows release is built in the MSYS2 CLANG64 environment with the
`clang64-release-static` preset. Everything below is linked into `tTime.exe`; the ZIP contains
no DLLs. The licence files MSYS2 installs for each library are in `licenses/msys2/<package>`.

### Qt 6 (The Qt Company Ltd. and other contributors)

- **Linked in:** Qt Core, Gui, Widgets, Multimedia and Network, and the Qt plugins tTime uses
  (Windows platform and style, ICO images, Windows media backend). No FFmpeg.
- **Licence:** GNU Lesser General Public License v3 (`licenses/LGPL-3.0.txt`), which builds on
  the GNU General Public License v3 (`licenses/GPL-3.0.txt`).
- **Source code:** Qt 6.12.0 as packaged by MSYS2 (`mingw-w64-clang-x86_64-qt6-static`). The
  Qt sources are available from <https://download.qt.io/> and <https://code.qt.io/>; MSYS2's
  build recipe and patches are at <https://github.com/msys2/MINGW-packages>
  (`mingw-w64-qt6-static`).
- **Using a modified Qt** (LGPL v3 section 4): tTime's complete source code is available under
  the MIT License. Building it with the `clang64-release-static` preset against your own static
  Qt (put it first in `CMAKE_PREFIX_PATH`) relinks `tTime.exe` with that Qt.

### Libraries linked into `tTime.exe`

From the MSYS2 CLANG64 packages; each one's licence text is in `licenses/msys2/<package>`.

| Library | MSYS2 package | Licence |
|---------|---------------|---------|
| LLVM libc++, compiler-rt builtins | `libc++`, `compiler-rt` | Apache-2.0 WITH LLVM-exception |
| mingw-w64 C runtime startup code | `crt` | ZPL-2.1 and public domain |
| FreeType | `freetype` | FreeType License (FTL), chosen over GPL-2.0 |
| HarfBuzz | `harfbuzz` | MIT |
| Graphite2 | `graphite2` | LGPL-2.1-or-later |
| libpng | `libpng` | PNG Reference Library License v2 |
| zlib | `zlib` | Zlib |
| Brotli (decoder) | `brotli` | MIT |
| bzip2 | `bzip2` | bzip2 licence (BSD-style) |
| BLAKE2 (libb2) | `libb2` | CC0-1.0 |
| MD4C | `md4c` | MIT |
| PCRE2 | `pcre2` | BSD-3-Clause |
| OpenSSL (libcrypto) | `openssl` | Apache-2.0 |

Portions of this software are copyright © 2026 The FreeType Project (<https://freetype.org>).
All rights reserved.

Graphite2 is under the LGPL v2.1 or later, like Qt under the LGPL v3: its source is available
from <https://github.com/silnrsi/graphite>, and relinking `tTime.exe` against a modified
Graphite2 works the same way as for Qt.

The exe also uses the Universal C Runtime (`ucrtbase.dll`, `api-ms-win-crt-*`), which is part of
Windows 10/11 and is not shipped.

### Shared Windows builds

`clang64-release` and `clang64-debug` link Qt dynamically. Their install step deploys Qt, Qt
Multimedia's FFmpeg backend and about 100 MSYS2 DLLs, including GPL-licensed codecs (x264,
x265, Xvid). They are meant for development only: their licences are not collected, and such a
folder must not be distributed as is.

## macOS: `tTime-<version>-macos-arm64.dmg`

The app bundle carries Qt Core, Gui, Widgets, Multimedia and Network as frameworks in
`tTime.app/Contents/Frameworks` and the plugins in `Contents/PlugIns`, under the LGPL v3 as
above; the sources of the unmodified Qt community release are available from
<https://download.qt.io/>. You can replace the Qt frameworks in the bundle with your own build of
the same Qt version. macdeployqt cannot leave plugins out, so the bundle also contains Qt
Multimedia's FFmpeg backend and the FFmpeg libraries it plays sound with (GNU LGPL v2.1 or later;
source from <https://ffmpeg.org/>, and the Qt build of FFmpeg is described at
<https://doc.qt.io/qt-6/qtmultimedia-index.html>).

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
