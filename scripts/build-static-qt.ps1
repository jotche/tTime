<#
.SYNOPSIS
    Builds a static Qt for the mingw-release-static preset.

.DESCRIPTION
    Downloads the Qt source archives of the modules tTime needs (qtbase, qtshadertools,
    qtmultimedia), checks them against Qt's published MD5 sums, and builds and installs
    them as static libraries with the MinGW and Ninja from the Qt online installer
    (under QT_ROOT). The MinGW runtime (libgcc, libstdc++, winpthread) is linked
    statically as well, so a tTime built against this Qt is a single exe.

    The result goes to <QtRoot>\<QtVersion>\mingw_64_static, where the
    mingw-release-static preset looks for it. Downloads and build trees go to
    <QtRoot>\static-build\<QtVersion>; delete that folder afterwards to free the space.

    Takes about 30-60 minutes; the downloads and finished modules are reused when the
    script is run again (pass -Clean to start over).

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts\build-static-qt.ps1

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts\build-static-qt.ps1 -QtRoot D:\Qt -Jobs 8
#>
[CmdletBinding()]
param(
    [string]$QtVersion = '6.12.0',
    [string]$QtRoot = $env:QT_ROOT,
    [string]$MinGWDir = 'mingw1310_64',
    [string]$Prefix,
    [string]$WorkDir,
    [int]$Jobs = [Environment]::ProcessorCount,
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'   # Invoke-WebRequest is very slow with the progress bar

if (-not $QtRoot) { throw 'Set the QT_ROOT environment variable (e.g. C:\Qt) or pass -QtRoot.' }
if (-not $Prefix) { $Prefix = Join-Path $QtRoot "$QtVersion\mingw_64_static" }
if (-not $WorkDir) { $WorkDir = Join-Path $QtRoot "static-build\$QtVersion" }

# Build tools from the Qt online installer, ahead of anything else on PATH (another
# GCC or CMake on PATH would otherwise be picked up).
$toolDirs = @(
    (Join-Path $QtRoot "Tools\$MinGWDir\bin"),
    (Join-Path $QtRoot 'Tools\Ninja'),
    (Join-Path $QtRoot 'Tools\CMake_64\bin'))
foreach ($dir in $toolDirs) {
    if (-not (Test-Path $dir)) { throw "Not found: $dir (install it with the Qt online installer)." }
}
$env:PATH = ($toolDirs -join ';') + ';' + $env:PATH

# Modules in dependency order. Qt Multimedia needs Qt Shader Tools (for its video
# shaders); tTime itself only uses Core, Gui, Widgets and Multimedia.
$modules = @('qtbase', 'qtshadertools', 'qtmultimedia')

$minor = ($QtVersion -split '\.')[0..1] -join '.'
$baseUrl = "https://download.qt.io/official_releases/qt/$minor/$QtVersion/submodules"
$downloadDir = Join-Path $WorkDir 'download'
$sourceDir = Join-Path $WorkDir 'src'
$buildDir = Join-Path $WorkDir 'build'

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

if ($Clean -and (Test-Path $WorkDir)) { Remove-Item $WorkDir -Recurse -Force }
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
            Invoke-Checked (Join-Path $src 'configure.bat') @(
                '-prefix', $Prefix,
                '-static',
                '-static-runtime',   # also links libgcc/libstdc++/winpthread statically
                '-release',
                '-nomake', 'examples',
                '-nomake', 'tests')
        } elseif ($module -eq 'qtmultimedia') {
            # tTime plays its WAV through the native Windows media backend; no FFmpeg.
            Invoke-Checked (Join-Path $Prefix 'bin\qt-configure-module.bat') @(
                $src, '-no-feature-ffmpeg')
        } else {
            Invoke-Checked (Join-Path $Prefix 'bin\qt-configure-module.bat') @($src)
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
Write-Host "Static Qt $QtVersion installed in $Prefix" -ForegroundColor Green
Write-Host 'Build tTime with:  cmake --preset mingw-release-static; cmake --build --preset mingw-release-static'
Write-Host "Delete $WorkDir to free the space used by the sources and build trees."
