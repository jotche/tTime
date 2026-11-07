# Qt Test suites, one executable each (tests/<name>/<name>.pro builds tests/<name>.cpp).
TEMPLATE = subdirs

SUBDIRS = \
    tst_alarm \
    tst_alarmmanager \
    tst_alarmspec \
    tst_appsettings \
    tst_bellanimation \
    tst_countdownlabel \
    tst_countdownwindow \
    tst_defaultsettings \
    tst_digitlayout \
    tst_fontweight \
    tst_theme \
    tst_treesway \
    tst_wavinfo \
    tst_windowplacement

# mingw32-make check: runs every test.
check.CONFIG = recursive
QMAKE_EXTRA_TARGETS += check
