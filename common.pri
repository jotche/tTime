# Settings shared by the library, the application and the tests.

VERSION = 0.2.3

QT += core gui
CONFIG += warn_on release
CONFIG -= debug_and_release debug_and_release_target

DEFINES += TTIME_VERSION=\\\"$$VERSION\\\" QT_NO_CAST_TO_ASCII
INCLUDEPATH += $$PWD/src
DEPENDPATH += $$PWD/src

*-g++*: QMAKE_CXXFLAGS_WARN_ON += -Wextra

# windres pipes the preprocessor output through cmd.exe by default, which breaks when
# cmd has an AutoRun command; a temporary file avoids the pipe.
win32-g++*: QMAKE_RC = windres --use-temp-file

# Against a static Qt (CONFIG contains static, from Qt's qconfig.pri), also link the
# MinGW runtime (libgcc, libstdc++, winpthreads) statically: tTime.exe then needs only
# Windows' own DLLs.
win32-g++*:static: QMAKE_LFLAGS += -static
