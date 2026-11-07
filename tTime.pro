# tTime: countdown alarms in the system tray (Qt 4.8).
#
#   qmake tTime.pro && mingw32-make            (or from a separate build directory)
#   mingw32-make check                          runs the unit tests
#
# src/ttime-lib.pro  everything but main(), as a static library
# src/ttime-app.pro  tTime.exe
# tests/             Qt Test suites (link the library)

TEMPLATE = subdirs

SUBDIRS = lib app tests
lib.file = src/ttime-lib.pro
app.file = src/ttime-app.pro
app.depends = lib
tests.depends = lib

# mingw32-make check: builds everything and runs the tests.
check.depends = first
check.commands = cd tests && $(MAKE) check
QMAKE_EXTRA_TARGETS += check

OTHER_FILES += README.md THIRD-PARTY-NOTICES.md src/tTime.ini
