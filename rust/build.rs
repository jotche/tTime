//! Embeds the application icon and version information into tTime.exe and
//! copies the shipped default settings (tTime.ini) next to the built executable.

use std::path::PathBuf;

fn main() {
    let resources = PathBuf::from("../src/Resources");
    let ini = PathBuf::from("../src/tTime.ini");
    println!("cargo:rerun-if-changed={}", ini.display());
    println!("cargo:rerun-if-changed={}", resources.join("icons/alarm.ico").display());

    // target/<profile>/ is three levels above OUT_DIR (target/<profile>/build/<pkg>/out).
    if let Some(profile_dir) = PathBuf::from(std::env::var("OUT_DIR").unwrap()).ancestors().nth(3)
        && let Err(error) = std::fs::copy(&ini, profile_dir.join("tTime.ini"))
    {
        println!("cargo:warning=could not copy tTime.ini: {error}");
    }

    if std::env::var("CARGO_CFG_TARGET_OS").as_deref() == Ok("windows") {
        let mut res = winresource::WindowsResource::new();
        // Icon id 1: gpui loads it as the default window icon.
        res.set_icon(resources.join("icons/alarm.ico").to_str().unwrap())
            .set("FileDescription", "tTime - countdown alarms in the system tray")
            .set("ProductName", "tTime")
            .set("OriginalFilename", "tTime.exe")
            .set("LegalCopyright", "GPL-3.0-or-later");
        if let Err(error) = res.compile() {
            println!("cargo:warning=could not embed the Windows resources: {error}");
        }
    }
}
