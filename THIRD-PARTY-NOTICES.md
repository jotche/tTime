# Third-party notices

tTime itself is licensed under the MIT License (see `LICENSE`). A tTime distribution
(`tTime.exe` with the DLLs listed in `README.md`) also contains the third-party components
below. Their licence texts are installed in the `licenses` folder by `mingw32-make install`.

## Qt 4 (Nokia Corporation and other contributors)

- **Used:** `QtCore4.dll` and `QtGui4.dll`, unmodified, as shipped with the Qt 4.7.4 (or 4.8)
  open source release for MinGW.
- **Licence:** GNU Lesser General Public License v2.1 (`licenses/LGPL-2.1.txt`) with the Nokia
  Qt LGPL Exception version 1.1 (`licenses/Qt-LGPL-Exception-1.1.txt`).
- **Source code:** <https://download.qt.io/archive/qt/4.7/> (and `/4.8/`).
- A shared build loads the Qt DLLs, which can be replaced with other builds of the same Qt
  version. The static build (`tTime-<version>-windows-x86-static.zip`) links QtCore and QtGui
  (with Qt's bundled zlib and libpng, under their own permissive licences) into `tTime.exe`;
  as the LGPL requires, tTime's complete source code is available under the MIT License, so
  `tTime.exe` can be relinked against a modified Qt (see "Static build" in `README.md`).

## MinGW runtime

- `libgcc_s_dw2-1.dll` (and, with MinGW-w64, `libstdc++-6.dll`): GNU General Public License
  v3 (`licenses/GPL-3.0.txt`) with the GCC Runtime Library Exception 3.1
  (`licenses/GCC-Runtime-Library-Exception-3.1.txt`), which allows distributing them with
  tTime under its own licence.
- `mingwm10.dll` (MinGW GCC 4.4): public domain.
- `libwinpthread-1.dll` (MinGW-w64): MIT/BSD-style licence of the MinGW-w64 project.

## Fonts (compiled into `tTime.exe`)

All bundled fonts are under the SIL Open Font License 1.1. tTime bundles the static font files
of each project's release (one file per weight):

| Font | Copyright | Licence file |
|------|-----------|--------------|
| Oxanium (ExtraLight-ExtraBold) | 2019 The Oxanium Project Authors (<https://github.com/sevmeyer/oxanium>) | `licenses/Oxanium-OFL.txt` |
| Jura (Light-Bold) | 2019 The Jura Project Authors (<https://github.com/ossobuffo/jura>) | `licenses/Jura-OFL.txt` |
| Doto (Black) | 2024 The Doto Project Authors (<https://github.com/oliverlalan/Doto>) | `licenses/Doto-OFL.txt` |
| Outfit (Thin-Black) | 2021 The Outfit Project Authors (<https://github.com/Outfitio/Outfit-Fonts>) | `licenses/Outfit-OFL.txt` |
| Sansation (Regular, Light) | 2011 Bernd Montag, Reserved Font Name "Sansation" (OFL release via <https://github.com/google/fonts/tree/main/ofl/sansation>) | `licenses/Sansation-OFL.txt` |

## Images and sound

The images are compiled into `tTime.exe`; the alarm sound is shipped as
`Resources/sounds/submarine1.wav`. The alarm icon (`alarm.ico`, and `alarm.png` made from it),
the toolbar icons (`icons01/*.png`), the tree picture (`tree02.png`) and the alarm sound come
from the original 2010 tTime project. **Their origin and licence were not recorded**, and the
files contain no author metadata. Confirm their terms, or replace them, before distributing
tTime publicly. `play_matched_16.png` was drawn for tTime and falls under tTime's MIT licence.
