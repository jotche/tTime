cmake_minimum_required(VERSION 3.22...3.30)

project(tTime
    VERSION 1.0.0
    DESCRIPTION "Countdown alarms (tea timer) living in the system tray"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TTIME_BUILD_TESTS "Build the unit tests" ON)
option(TTIME_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(TTIME_MINIMAL_DEPLOY "Deploy only the Qt parts tTime uses (smaller dist)" ON)
option(TTIME_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer (GCC/Clang, not MinGW)" OFF)

# 6.7: QFont::Tag / QFont::setVariableAxis (variable countdown fonts)
find_package(Qt6 6.7 COMPONENTS Core Gui Widgets Multimedia)
if(NOT Qt6_FOUND)
    message(FATAL_ERROR
        "Qt 6.7 or newer (Core, Gui, Widgets, Multimedia) was not found. "
        "With the mingw-* presets, set the QT_ROOT environment variable to your Qt "
        "installation (e.g. setx QT_ROOT C:/Qt) and start a new terminal. Otherwise pass "
        "-DCMAKE_PREFIX_PATH=<Qt>/6.x.y/<compiler>.")
endif()
qt_standard_project_setup(REQUIRES 6.7)

include(GNUInstallDirs)

# ---------------------------------------------------------------------------
# Shared compiler settings
# ---------------------------------------------------------------------------
add_library(ttime_options INTERFACE)
target_compile_definitions(ttime_options INTERFACE
    QT_DISABLE_DEPRECATED_UP_TO=0x060C00   # fail the build on any API deprecated up to Qt 6.12
    QT_NO_CAST_TO_ASCII
    QT_NO_NARROWING_CONVERSIONS_IN_CONNECT
    QT_USE_QSTRINGBUILDER
    TTIME_VERSION="${PROJECT_VERSION}")

if(MSVC)
    target_compile_options(ttime_options INTERFACE /W4 /permissive- /utf-8)
    if(TTIME_WARNINGS_AS_ERRORS)
        target_compile_options(ttime_options INTERFACE /WX)
    endif()
else()
    target_compile_options(ttime_options INTERFACE
        -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Woverloaded-virtual)
    if(TTIME_WARNINGS_AS_ERRORS)
        target_compile_options(ttime_options INTERFACE -Werror)
    endif()
endif()

if(TTIME_SANITIZE)
    if(MSVC OR MINGW)
        message(WARNING "TTIME_SANITIZE is only supported with GCC/Clang on Linux/macOS; ignoring it.")
    else()
        target_compile_options(ttime_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(ttime_options INTERFACE -fsanitize=address,undefined)
    endif()
endif()

# ---------------------------------------------------------------------------
# Libraries (the app and the tests link these instead of compiling sources twice)
#   ttime_core  logic and settings, Qt Core only
#   ttime_gui   themes, icon and bell animation, fonts, configuration, resources
#   ttime_ui    the windows and the alarm manager
# ---------------------------------------------------------------------------
add_library(ttime_core STATIC
    src/alarm-spec.cpp
    src/alarm-spec.h
    src/app-settings.cpp
    src/app-settings.h
    src/digit-layout.cpp
    src/digit-layout.h
    src/font-weight.cpp
    src/font-weight.h
    src/alarm.cpp
    src/alarm.h
    src/wav-info.cpp
    src/wav-info.h
    src/window-placement.cpp
    src/window-placement.h)
target_include_directories(ttime_core PUBLIC src)
target_link_libraries(ttime_core
    PUBLIC Qt6::Core
    PRIVATE ttime_options)

add_library(ttime_gui STATIC
    src/app-config.cpp
    src/app-config.h
    src/bell-animation.cpp
    src/bell-animation.h
    src/countdown-font.cpp
    src/countdown-font.h
    src/image-layers.cpp
    src/image-layers.h
    src/theme.cpp
    src/theme.h
    src/tree-sway.cpp
    src/tree-sway.h)
target_link_libraries(ttime_gui
    PUBLIC ttime_core Qt6::Gui
    PRIVATE ttime_options)

# Resources (":/fonts/...", ":/icons/...", ...). Defined on the library with
# qt_add_resources(), so Qt links them into every executable that uses it.
function(ttime_add_resources prefix)
    set(files)
    foreach(pair IN LISTS ARGN)
        string(REPLACE "=" ";" pair "${pair}")
        list(GET pair 0 alias)
        list(GET pair 1 file)
        set_source_files_properties(${file} PROPERTIES QT_RESOURCE_ALIAS ${alias})
        list(APPEND files ${file})
    endforeach()
    string(REPLACE "/" "" name "ttime_${prefix}")
    qt_add_resources(ttime_gui ${name} PREFIX "/${prefix}" FILES ${files})
endfunction()

ttime_add_resources(fonts
    sansation_regular=src/Resources/fonts/Sansation-Regular.ttf
    sansation_light=src/Resources/fonts/Sansation-Light.ttf
    oxanium=src/Resources/fonts/Oxanium-Variable.ttf
    jura=src/Resources/fonts/Jura-Variable.ttf
    doto=src/Resources/fonts/Doto-Variable.ttf)
ttime_add_resources(icons
    alarm=src/Resources/icons/alarm.ico
    anchor=src/Resources/icons01/flag1_16.png
    down=src/Resources/icons01/arrow_down_16.png
    exit=src/Resources/icons01/delete_16.png
    pastel-repeat=src/Resources/icons01/reload_16.png
    pastel-play=src/Resources/icons01/play_matched_16.png
    pastel-pause=src/Resources/icons01/video_pause_16.png
    pastel-stop=src/Resources/icons01/video_stop_16.png)
ttime_add_resources(images
    tree02=src/Resources/images/tree02.png)
ttime_add_resources(sounds
    submarine=src/Resources/sounds/submarine1.wav)

add_library(ttime_ui STATIC
    src/alarm-holder.cpp
    src/alarm-holder.h
    src/countdown-label.cpp
    src/countdown-label.h
    src/new-alarm-window.cpp
    src/new-alarm-window.h
    src/new-alarm-window.ui
    src/alarm-manager.cpp
    src/alarm-manager.h
    src/countdown-window.cpp
    src/countdown-window.h
    src/countdown-window.ui
    src/time-up-window.cpp
    src/time-up-window.h
    src/time-up-window.ui
    src/window-slot.cpp
    src/window-slot.h
    src/window-utils.h)
target_link_libraries(ttime_ui
    PUBLIC ttime_gui Qt6::Widgets Qt6::Multimedia
    PRIVATE ttime_options)

# ---------------------------------------------------------------------------
# Application
# ---------------------------------------------------------------------------
qt_add_executable(tTime WIN32 MACOSX_BUNDLE
    src/main.cpp)

if(WIN32)
    # Icon and version information (file properties in Explorer), from the project version.
    set(TTIME_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/Resources/icons/alarm.ico")
    configure_file(src/ttime.rc.in "${CMAKE_CURRENT_BINARY_DIR}/ttime.rc" @ONLY)
    target_sources(tTime PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/ttime.rc")
endif()

if(APPLE)
    # App bundle: Finder/Dock icon and the default settings go into Contents/Resources
    # (code signing only accepts code in Contents/MacOS).
    set(ttime_bundle_resources src/Resources/icons/alarm.icns src/tTime.ini)
    target_sources(tTime PRIVATE ${ttime_bundle_resources})
    set_source_files_properties(${ttime_bundle_resources} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(tTime PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "tTime"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.codelogiq.ttime"
        MACOSX_BUNDLE_ICON_FILE "alarm.icns"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_COPYRIGHT "Copyright (c) Vladimir Gjokjevski. MIT License.")
endif()

target_link_libraries(tTime PRIVATE
    ttime_ui
    ttime_options)

# Strip symbols from optimised GCC/Clang builds (smaller exe). Apple's linker has
# no plain -s.
if(NOT MSVC AND NOT APPLE)
    target_link_options(tTime PRIVATE $<$<CONFIG:Release,MinSizeRel>:-s>)
endif()

# ---------------------------------------------------------------------------
# Install + deployment (copies the Qt runtime next to the executable)
#   cmake --install <build-dir> --prefix <dist-dir>
# ---------------------------------------------------------------------------
install(TARGETS tTime
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

set(ttime_deploy_args)
if(TTIME_MINIMAL_DEPLOY AND APPLE)
    # macdeployqt cannot leave plugins out; only the translations are skipped. Qt
    # Multimedia plays the WAV through its default (FFmpeg) backend on macOS.
    list(APPEND ttime_deploy_args NO_TRANSLATIONS)
elseif(TTIME_MINIMAL_DEPLOY)
    # tTime is a plain widgets app that plays one WAV file:
    #  - no OpenGL / Direct3D rendering   -> no software GL rasteriser, no D3D compiler
    #  - WAV via the native Windows media backend -> no FFmpeg (~27 MB)
    #  - only PNG/ICO images              -> no SVG/JPEG/GIF plugins (and no Qt6Svg)
    #  - no networking / touch            -> no TLS, network-information, TUIO plugins
    #  - English-only UI                  -> no Qt translations
    list(APPEND ttime_deploy_args
        NO_TRANSLATIONS
        DEPLOY_TOOL_OPTIONS
            --no-opengl-sw
            --no-system-d3d-compiler
            --no-system-dxc-compiler
            --no-ffmpeg
            --skip-plugin-types iconengines,generic,tls,networkinformation
            --exclude-plugins ffmpegmediaplugin,qjpeg,qgif,qsvg)
endif()

qt_generate_deploy_app_script(
    TARGET tTime
    OUTPUT_SCRIPT ttime_deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    ${ttime_deploy_args})
if(APPLE)
    # macdeployqt rewrites the binaries, which breaks their signature, and Apple
    # silicon refuses to run unsigned code: sign the whole bundle again (ad hoc, no
    # developer certificate) in the same step, right after the deployment. (CPack
    # adds the Applications link to the DMG itself.)
    install(CODE "
        include(\"${ttime_deploy_script}\")
        execute_process(
            COMMAND codesign --force --deep --sign - \"\${CMAKE_INSTALL_PREFIX}/tTime.app\"
            COMMAND_ERROR_IS_FATAL ANY)
    ")
else()
    install(SCRIPT ${ttime_deploy_script})
    # Default settings, read (never written) from next to the executable. (In the
    # macOS bundle they are in Contents/Resources, see above.)
    install(FILES src/tTime.ini DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# tTime's licence and the licences of everything shipped with it (Qt, MinGW
# runtime, fonts compiled into the exe). See THIRD-PARTY-NOTICES.md.
install(FILES
    LICENSE
    THIRD-PARTY-NOTICES.md
    licenses/GPL-3.0.txt
    licenses/LGPL-3.0.txt
    src/Resources/fonts/Doto-OFL.txt
    src/Resources/fonts/Jura-OFL.txt
    src/Resources/fonts/Oxanium-OFL.txt
    src/Resources/fonts/Sansation-OFL.txt
    DESTINATION licenses)
if(MINGW)
    # The MinGW runtime DLLs shipped next to tTime.exe.
    install(FILES
        licenses/GCC-Runtime-Library-Exception-3.1.txt
        licenses/winpthreads.txt
        DESTINATION licenses)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(TTIME_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# Packaging: the complete installation (app, Qt runtime, licences) as a ZIP on
# Windows and as a DMG (tTime.app, Applications link, licences) on macOS.
#   cmake --build --preset mingw-release --target package
# ---------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "tTime")
set(CPACK_PACKAGE_VENDOR "Vladimir Gjokjevski")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "tTime ${PROJECT_VERSION}")
    set(CPACK_DMG_SLA_USE_RESOURCE_FILE_LICENSE OFF) # no licence prompt when opening the DMG
    if(CMAKE_OSX_ARCHITECTURES)
        set(ttime_arch "${CMAKE_OSX_ARCHITECTURES}")
    else()
        set(ttime_arch "${CMAKE_SYSTEM_PROCESSOR}")
    endif()
    string(REPLACE ";" "-" ttime_arch "${ttime_arch}")
    set(CPACK_SYSTEM_NAME "macos-${ttime_arch}")
else()
    set(CPACK_GENERATOR "ZIP")
endif()
if(WIN32)
    set(CPACK_SYSTEM_NAME "windows-x64")
endif()
set(CPACK_PACKAGE_FILE_NAME "tTime-${PROJECT_VERSION}-${CPACK_SYSTEM_NAME}")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")
include(CPack)

