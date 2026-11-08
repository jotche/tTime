# Third-party notices (gpui build)

tTime itself is licensed under the MIT License (see `LICENSE`). The gpui (Rust) build of tTime,
`tTime-<version>-windows-x64-gpui.zip`, also contains the third-party components below. Their
licence texts are in this `licenses` folder.

## Rust crates

`tTime.exe` is statically linked from Rust crates published on <https://crates.io>, among them
[gpui](https://crates.io/crates/gpui) (Zed Industries, Apache-2.0), `tray-icon` and `muda`
(Tauri, Apache-2.0 / MIT), `rodio` and `cpal` (Apache-2.0 / MIT), `image` (Apache-2.0 / MIT) and
Microsoft's `windows` crates (Apache-2.0 / MIT). `crates/CRATES.md` lists every crate in the exe
with its version and licence; `crates/<crate>-<version>/` holds the licence files published with
it. The source of each crate is available from crates.io at that version.

The text field of the "New alarm" window (`rust/src/gui/text_input.rs`) is based on the text
input example that ships with gpui (Apache-2.0).

## Fonts (compiled into `tTime.exe`)

All under the SIL Open Font License 1.1:

- Oxanium (`Oxanium-OFL.txt`), <https://github.com/sevmeyer/oxanium>
- Jura (`Jura-OFL.txt`), <https://github.com/ossobuffo/jura>
- Doto (`Doto-OFL.txt`), <https://github.com/oliverlalan/Doto>
- Outfit (`Outfit-OFL.txt`), <https://github.com/Outfitio/Outfit-Fonts>
- Sansation (`Sansation-OFL.txt`), <https://github.com/google/fonts/tree/main/ofl/sansation>

## Windows

The exe uses only Windows' own DLLs (Direct3D 11, DirectWrite, DirectComposition, WASAPI and
the Universal CRT that ships with Windows 10/11).
