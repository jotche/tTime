# Included by every test's .pro file (tests/<name>/<name>.pro), after it set TARGET
# and SOURCES.
include($$PWD/../common.pri)

TEMPLATE = app
CONFIG += qtestlib console
CONFIG -= app_bundle
DESTDIR = $$OUT_PWD
INCLUDEPATH += $$PWD

# The library is built in <build dir>/src; this test in <build dir>/tests/<name>.
TTIME_LIB_DIR = $$OUT_PWD/../../src
include($$PWD/../src/ttime-link.pri)

# Fonts, icons, pictures and the alarm sound (":/...").
RESOURCES += $$PWD/../src/ttime.qrc

# Source files the tests read directly (the shipped tTime.ini, the alarm sound).
DEFINES += TTIME_SOURCE_DIR=\\\"$$PWD/..\\\"

HEADERS += $$PWD/testutil.h

# mingw32-make check: runs the test (the Qt DLLs must be on the PATH).
TEST_EXE = $$OUT_PWD/$$TARGET
win32: TEST_EXE = $$replace(TEST_EXE, /, \\)
check.commands = $$TEST_EXE
QMAKE_EXTRA_TARGETS += check
