<#
.SYNOPSIS
    Builds a trimmed static Qt for the clang64-release-static-min preset.

.DESCRIPTION
    Downloads the Qt source archives of the modules tTime needs (qtbase, qtshadertools,
    qtmultimedia), checks them against Qt's published MD5 sums, and builds them as static
    libraries with the MSYS2 CLANG64 toolchain (under MSYS2_ROOT), leaving out the Qt
    parts tTime does not use: Network, SQL, XML, D-Bus, PrintSupport, OpenGL, Vulkan,
    OpenSSL, ICU, PDF, Markdown/ODF, JPEG/GIF, session management, windeployqt, and in
    Qt Multimedia FFmpeg and spatial audio (the native Windows backend plays the alarm).

    FreeType, HarfBuzz, libpng and zlib come from MSYS2 (the same libraries the
    clang64-release-static preset links); PCRE2 is Qt's bundled copy, because MSYS2's
    PCRE2 headers declare the functions as DLL imports.

    DirectWrite stays in: Qt 6.12 does not build without it (the minimal platform plugin
    includes its header unconditionally). tTime selects the FreeType font engine at run
    time (src/main.cpp), so it is not used.

    The result goes to <QtRoot>\<QtVersion>\clang64_static_min, where the
    clang64-release-static-min preset looks for it. Downloads and build trees go to
    <QtRoot>\static-build\<QtVersion>; delete that folder afterwards to free the space
    (about 1.5 GB). Takes about 20-40 minutes; finished modules are kept when the script
    is run again (pass -Clean to start over).

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts\build-static-qt-min.ps1

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts\build-static-qt-min.ps1 -QtRoot D:\Qt -Jobs 8
#>
[CmdletBinding()]
param(
    [string]$QtVersion = '6.12.0',
    [string]$QtRoot = $env:QT_ROOT,
    [string]$Msys2Root = $env:MSYS2_ROOT,
    [string]$Prefix,
    [string]$WorkDir,
    [int]$Jobs = [Environment]::ProcessorCount,
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'   # Invoke-WebRequest is very slow with the progress bar

if (-not $QtRoot) { throw 'Set the QT_ROOT environment variable (e.g. C:\Qt) or pass -QtRoot.' }
if (-not $Msys2Root) { throw 'Set the MSYS2_ROOT environment variable (e.g. C:\msys64) or pass -Msys2Root.' }
if (-not $Prefix) { $Prefix = Join-Path $QtRoot "$QtVersion\clang64_static_min" }
if (-not $WorkDir) { $WorkDir = Join-Path $QtRoot "static-build\$QtVersion" }

# MSYS2 CLANG64 toolchain (clang, lld, cmake, ninja) ahead of anything else on PATH.
$clang64 = Join-Path $Msys2Root 'clang64'
foreach ($tool in @('clang++.exe', 'cmake.exe', 'ninja.exe')) {
    if (-not (Test-Path (Join-Path $clang64 "bin\$tool"))) {
        throw "Not found: $clang64\bin\$tool (install the MSYS2 packages listed in the README)."
    }
}
$env:PATH = "$clang64\bin;$env:PATH"
$clang64Cmake = $clang64 -replace '\\', '/'

# Modules in dependency order. Qt Multimedia needs Qt Shader Tools (for its video
# shaders); tTime itself only uses Core, Gui, Widgets and Multimedia (and Test).
$modules = @('qtbase', 'qtshadertools', 'qtmultimedia')

$qtbaseOptions = @(
    '-prefix', $Prefix,
    '-platform', 'win32-clang-g++',
    '-static', '-release', '-optimize-size',
    '-nomake', 'examples', '-nomake', 'tests',
    '-no-icu', '-no-glib', '-no-openssl', '-no-dbus', '-no-opengl',
    '-system-zlib', '-system-libpng', '-system-freetype', '-system-harfbuzz', '-qt-pcre',
    '-no-feature-network', '-no-feature-sql', '-no-feature-xml', '-no-feature-printsupport',
    '-no-feature-vulkan', '-no-feature-pdf',
    '-no-feature-textmarkdownreader', '-no-feature-textmarkdownwriter', '-no-feature-textodfwriter',
    '-no-feature-jpeg', '-no-feature-gif', '-no-feature-sessionmanager',
    '-no-feature-windeployqt',   # needs Qt Network; a static build needs no deployment
    '--', "-DCMAKE_PREFIX_PATH=$clang64Cmake")

$moduleOptions = @{
    'qtshadertools' = @()
    'qtmultimedia'  = @('-no-feature-ffmpeg', '-no-feature-spatialaudio', '-no-feature-gstreamer')
}

$minor = ($QtVersion -split '\.')[0..1] -join '.'
$baseUrl = "https://download.qt.io/official_releases/qt/$minor/$QtVersion/submodules"
$downloadDir = Join-Path $WorkDir 'download'
$sourceDir = Join-Path $WorkDir 'src'
$buildDir = Join-Path $WorkDir 'build-clang64-min'

function Invoke-Checked {
    param([string]$Exe, [string[]]$Arguments)
    Write-Host "> $Exe $($Arguments -join ' ')" -ForegroundColor DarkGray
    & $Exe @Arguments | Out-Host   # keep tool output out of the function's return value
    if ($LASTEXITCODE -ne 0) { throw "$Exe failed with exit code $LASTEXITCODE." }
}

function Get-Md5Sums {
    $file = Join-Path $downloadDir 'md5sums.txt'
    if (-not (Test-Path $file)) {
        Invoke-WebRequest -Uri "$baseUrl/md5sums.txt" -OutFile $file -UseBasicParsing
    }
    $sums = @{}
    foreach ($line in Get-Content $file) {
        $parts = $line -split '\s+', 2
        if ($parts.Count -eq 2) { $sums[$parts[1].Trim()] = $parts[0].ToLowerInvariant() }
    }
    return $sums
}

function Get-ModuleSource {
    param([string]$Module, [hashtable]$Sums)
    $target = Join-Path $sourceDir $Module
    if (Test-Path (Join-Path $target 'CMakeLists.txt')) { return $target }

    $archive = "$Module-everywhere-src-$QtVersion.zip"
    $archivePath = Join-Path $downloadDir $archive
    if (-not $Sums.ContainsKey($archive)) { throw "$archive is not listed in $baseUrl/md5sums.txt." }
    if (Test-Path $archivePath) {
        $hash = (Get-FileHash $archivePath -Algorithm MD5).Hash.ToLowerInvariant()
        if ($hash -ne $Sums[$archive]) { Remove-Item $archivePath }
    }
    if (-not (Test-Path $archivePath)) {
        Write-Host "Downloading $archive ..." -ForegroundColor Cyan
        Invoke-WebRequest -Uri "$baseUrl/$archive" -OutFile $archivePath -UseBasicParsing
    }
    $hash = (Get-FileHash $archivePath -Algorithm MD5).Hash.ToLowerInvariant()
    if ($hash -ne $Sums[$archive]) {
        throw "MD5 mismatch for ${archive}: got $hash, expected $($Sums[$archive])."
    }

    Write-Host "Extracting $archive ..." -ForegroundColor Cyan
    $extracted = Join-Path $sourceDir "$Module-everywhere-src-$QtVersion"
    if (Test-Path $extracted) { Remove-Item $extracted -Recurse -Force }
    if (Test-Path $target) { Remove-Item $target -Recurse -Force }
    # Windows' own bsdtar is much faster than Expand-Archive for archives this size.
    Invoke-Checked "$env:SystemRoot\System32\tar.exe" @('-xf', $archivePath, '-C', $sourceDir)
    Rename-Item $extracted $Module   # short paths: Qt's build trees run into MAX_PATH otherwise
    return $target
}

if ($Clean -and (Test-Path $buildDir)) { Remove-Item $buildDir -Recurse -Force }
if ($Clean -and (Test-Path $Prefix)) { Remove-Item $Prefix -Recurse -Force }
foreach ($dir in @($downloadDir, $sourceDir, $buildDir)) {
    New-Item -ItemType Directory -Force $dir | Out-Null
}

$sums = Get-Md5Sums

foreach ($module in $modules) {
    $stamp = Join-Path $buildDir "$module.installed"
    if (Test-Path $stamp) {
        Write-Host "$module is already installed in $Prefix (pass -Clean to rebuild)." -ForegroundColor Green
        continue
    }

    $src = Get-ModuleSource -Module $module -Sums $sums
    $bld = Join-Path $buildDir $module
    if (Test-Path $bld) { Remove-Item $bld -Recurse -Force }
    New-Item -ItemType Directory -Force $bld | Out-Null

    Write-Host "Configuring $module ..." -ForegroundColor Cyan
    Push-Location $bld
    try {
        if ($module -eq 'qtbase') {
            Invoke-Checked (Join-Path $src 'configure.bat') $qtbaseOptions
        } else {
            Invoke-Checked (Join-Path $Prefix 'bin\qt-configure-module.bat') (@($src) + $moduleOptions[$module])
        }

        Write-Host "Building $module ..." -ForegroundColor Cyan
        Invoke-Checked 'cmake' @('--build', '.', '--parallel', "$Jobs")
        Invoke-Checked 'cmake' @('--install', '.')
    } finally {
        Pop-Location
    }
    Set-Content -Path $stamp -Value (Get-Date -Format o) -Encoding utf8
}

Write-Host ''
Write-Host "Trimmed static Qt $QtVersion installed in $Prefix" -ForegroundColor Green
Write-Host 'Build tTime with:  cmake --preset clang64-release-static-min; cmake --build --preset clang64-release-static-min'
Write-Host "Delete $WorkDir to free the space used by the sources and build trees."
