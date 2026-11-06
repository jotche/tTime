# Install step of a shared Windows build, run after windeployqt: copies the non-system
# DLLs that tTime.exe and the deployed Qt DLLs and plugins load from Qt's folder
# (TTIME_DLL_DIR; with MSYS2 <MSYS2_ROOT>/clang64/bin: ICU, HarfBuzz, FFmpeg, libc++,
# ...) next to tTime.exe. windeployqt deploys only Qt itself.
#
# Set by CMakeLists.txt before this script runs: TTIME_DLL_DIR, TTIME_BIN_DIR,
# TTIME_OBJDUMP.

set(bin_dir "${CMAKE_INSTALL_PREFIX}/${TTIME_BIN_DIR}")
file(GLOB_RECURSE deployed_dlls "${CMAKE_INSTALL_PREFIX}/*.dll")

set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "windows+pe")
set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL "objdump")
set(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND "${TTIME_OBJDUMP}")
file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES "${bin_dir}/tTime.exe"
    LIBRARIES ${deployed_dlls}
    DIRECTORIES "${bin_dir}" "${TTIME_DLL_DIR}"   # deployed copies first (plugins live in subfolders)
    PRE_EXCLUDE_REGEXES "^api-ms-" "^ext-ms-"
    RESOLVED_DEPENDENCIES_VAR resolved
    UNRESOLVED_DEPENDENCIES_VAR unresolved)

# Keep what comes from Qt's folder; everything else is part of Windows.
cmake_path(CONVERT "${TTIME_DLL_DIR}" TO_CMAKE_PATH_LIST dll_dir NORMALIZE)
string(TOLOWER "${dll_dir}" dll_dir)
set(copied 0)
foreach(dll IN LISTS resolved)
    cmake_path(CONVERT "${dll}" TO_CMAKE_PATH_LIST dll_path NORMALIZE)
    cmake_path(GET dll_path PARENT_PATH dll_parent)
    cmake_path(GET dll_path FILENAME dll_name)
    string(TOLOWER "${dll_parent}" dll_parent)
    if(dll_parent STREQUAL dll_dir AND NOT EXISTS "${bin_dir}/${dll_name}")
        file(COPY "${dll}" DESTINATION "${bin_dir}")
        math(EXPR copied "${copied} + 1")
    endif()
endforeach()
message(STATUS "Copied ${copied} third-party DLLs from ${TTIME_DLL_DIR}")
