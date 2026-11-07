# tTime.exe
include($$PWD/../common.pri)
include($$PWD/ttime-link.pri)

TEMPLATE = app
TARGET = tTime
DESTDIR = $$OUT_PWD

SOURCES += main.cpp
RESOURCES += ttime.qrc
win32: RC_FILE = ttime.rc

OTHER_FILES += tTime.ini

# mingw32-make install INSTALL_ROOT=<dist dir>: tTime.exe, the shipped default
# settings, the Resources folder (files tTime cannot use from its compiled-in
# resources, see externalResourcePath()) and the licences. The Qt and MinGW runtime
# DLLs are not copied; see README.md for the files to put next to tTime.exe.
target.path = /
settings.path = /
settings.files = tTime.ini
sounds.path = /Resources/sounds
sounds.files = Resources/sounds/submarine1.wav
licenses.path = /licenses
licenses.files = \
    ../LICENSE \
    ../THIRD-PARTY-NOTICES.md \
    ../licenses/LGPL-2.1.txt \
    ../licenses/Qt-LGPL-Exception-1.1.txt \
    ../licenses/GPL-3.0.txt \
    ../licenses/GCC-Runtime-Library-Exception-3.1.txt \
    Resources/fonts/Doto-OFL.txt \
    Resources/fonts/Jura-OFL.txt \
    Resources/fonts/Outfit-OFL.txt \
    Resources/fonts/Oxanium-OFL.txt \
    Resources/fonts/Sansation-OFL.txt
INSTALLS += target settings sounds licenses
