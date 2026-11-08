# Builds the gpui (Rust) version of tTime and assembles a distribution:
#
#   dist/gpui-release/bin/tTime.exe, tTime.ini
#   dist/gpui-release/licenses/...            tTime, fonts, and every Rust crate in the exe
#   dist/tTime-<version>-windows-x64-gpui.zip  the same, in a tTime-<version>-windows-x64-gpui folder
#
# Usage (from anywhere):  powershell -ExecutionPolicy Bypass -File rust\scripts\dist.ps1 [-SkipBuild]

param([switch]$SkipBuild)

$ErrorActionPreference = "Stop"
$crateDir = Split-Path -Parent $PSScriptRoot          # rust/
$repo = Split-Path -Parent $crateDir                  # repository root
$target = "x86_64-pc-windows-msvc"

Push-Location $crateDir
try {
    if (-not $SkipBuild) {
        cargo build --release
        if ($LASTEXITCODE -ne 0) { throw "cargo build failed" }
    }

    $metadata = cargo metadata --format-version 1 --filter-platform $target | ConvertFrom-Json
    if ($LASTEXITCODE -ne 0) { throw "cargo metadata failed" }
    $version = ($metadata.packages | Where-Object { $_.name -eq "ttime" }).version

    # Crates compiled into tTime.exe: normal (not build or dev) dependencies for the target.
    $crates = cargo tree -e normal --target $target --prefix none --format "{p}" |
        ForEach-Object { ($_ -replace " \(\*\)$", "" -replace " \(.*\)$", "").Trim() } |
        Where-Object { $_ -and $_ -notlike "ttime *" } |
        Sort-Object -Unique
    if ($LASTEXITCODE -ne 0) { throw "cargo tree failed" }
} finally {
    Pop-Location
}

$out = Join-Path $repo "dist\gpui-release"
if (Test-Path $out) {
    # Empty it rather than delete it: the folder may be open in Explorer or a terminal.
    Get-ChildItem $out -Recurse -File | Remove-Item -Force
    Get-ChildItem $out -Recurse -Directory | Sort-Object { $_.FullName.Length } -Descending |
        ForEach-Object { Remove-Item $_.FullName -Force -ErrorAction SilentlyContinue }
}
$bin = New-Item -ItemType Directory -Force (Join-Path $out "bin")
$licenses = New-Item -ItemType Directory -Force (Join-Path $out "licenses")
$crateLicenses = New-Item -ItemType Directory -Force (Join-Path $licenses "crates")

Copy-Item (Join-Path $crateDir "target\release\tTime.exe") $bin
Copy-Item (Join-Path $repo "src\tTime.ini") $bin

Copy-Item (Join-Path $repo "LICENSE") $licenses
Copy-Item (Join-Path $crateDir "THIRD-PARTY-NOTICES.md") $licenses
Get-ChildItem (Join-Path $repo "src\Resources\fonts\*-OFL.txt") | Copy-Item -Destination $licenses

# Licence files of each crate, from its package in the cargo registry. Crates that
# publish none get the standard text of their licence (rust/licenses, in crates/common).
$common = New-Item -ItemType Directory -Force (Join-Path $crateLicenses "common")
Copy-Item (Join-Path $crateDir "licenses\*.txt") $common
$commonIds = Get-ChildItem $common -File | ForEach-Object { $_.BaseName }

# The licence ids whose standard text applies: one alternative of an "OR" (or "/"), all parts of an "AND".
function Get-CommonLicenses([string]$expression) {
    $alternatives = $expression -replace "[()]", "" -split "\s+OR\s+|\s*/\s*"
    foreach ($alternative in $alternatives) {
        $parts = @($alternative -split "\s+AND\s+" | ForEach-Object { ($_ -replace "\s+WITH\s+.*$", "").Trim() })
        if (-not ($parts | Where-Object { $commonIds -notcontains $_ })) { return $parts }
    }
    return @()
}

$byId = @{}
foreach ($p in $metadata.packages) { $byId["$($p.name) v$($p.version)"] = $p }
$table = @("# Rust crates in tTime.exe", "",
    "Each crate's licence files, as published with the crate, are in the folder ``<crate>-<version>``.",
    "Crates published without licence files are covered by the standard licence text in ``common``",
    "(the copyright holders are the authors listed here).", "",
    "| Crate | Version | Licence | Authors | Licence text |", "|-------|---------|---------|---------|--------------|")
$unresolved = @()
$linked = 0
foreach ($crate in $crates) {
    $package = $byId[$crate]
    if (-not $package) { $unresolved += $crate; continue }
    # Procedural macros run at build time only; they are not part of the exe.
    if ($package.targets | Where-Object { $_.kind -contains "proc-macro" }) { continue }
    $linked++
    $source = Split-Path -Parent $package.manifest_path
    $files = Get-ChildItem $source -File | Where-Object { $_.Name -match "^(LICEN[CS]E|COPYING|NOTICE|UNLICENSE)" }
    $license = if ($package.license) { $package.license } else { "see files" }
    $authors = (@($package.authors) -join ", ") -replace "\|", "/" -replace "<[^>]*>", "" -replace "\s+,", ","
    if ($files) {
        $folder = "$($package.name)-$($package.version)"
        $dest = New-Item -ItemType Directory -Force (Join-Path $crateLicenses $folder)
        $files | Copy-Item -Destination $dest
        $text = $folder
    } else {
        $ids = Get-CommonLicenses $license
        if (-not $ids) { $unresolved += "$crate ($license)" }
        $text = ($ids | ForEach-Object { "common/$_.txt" }) -join ", "
    }
    $table += "| $($package.name) | $($package.version) | $license | $($authors.Trim()) | $text |"
}
[System.IO.File]::WriteAllLines((Join-Path $crateLicenses "CRATES.md"), [string[]]$table, (New-Object System.Text.UTF8Encoding($false)))

# ZIP with a top-level folder, like the Qt release package.
$name = "tTime-$version-windows-x64-gpui"
$staging = Join-Path ([System.IO.Path]::GetTempPath()) "ttime-dist-$([guid]::NewGuid())"
$root = New-Item -ItemType Directory -Force (Join-Path $staging $name)
Copy-Item -Recurse (Join-Path $out "*") $root
# Files unpacked from the cargo registry are dated 1970, which ZIP cannot store.
$now = Get-Date
Get-ChildItem -Recurse -File $root | ForEach-Object { $_.LastWriteTime = $now }
$zip = Join-Path $repo "dist\$name.zip"
if (Test-Path $zip) { Remove-Item -Force $zip }
# Entries are added one by one so their names use "/" (Windows PowerShell's
# Compress-Archive and ZipFile write "\", which other systems do not unpack as folders).
Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
$archive = [System.IO.Compression.ZipFile]::Open($zip, [System.IO.Compression.ZipArchiveMode]::Create)
try {
    foreach ($file in Get-ChildItem -Recurse -File $staging) {
        $entry = $file.FullName.Substring($staging.Length).TrimStart("\") -replace "\\", "/"
        [void][System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
            $archive, $file.FullName, $entry, [System.IO.Compression.CompressionLevel]::Optimal)
    }
} finally {
    $archive.Dispose()
}
Remove-Item -Recurse -Force $staging

Write-Output "Distribution: $out"
Write-Output "Package:      $zip ($([math]::Round((Get-Item $zip).Length / 1MB, 1)) MB)"
Write-Output "Crates:       $linked linked into tTime.exe"
if ($unresolved) {
    Write-Output "WARNING: no licence text for these crates (add the text to rust/licenses):"
    $unresolved | ForEach-Object { Write-Output "  $_" }
    exit 1
}
