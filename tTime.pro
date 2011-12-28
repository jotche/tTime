#-------------------------------------------------
#
# Project created by QtCreator 2010-12-13T20:13:28
#
#-------------------------------------------------

INSTALLS += target ttime_files ttime_resources

QT += core gui

TARGET = tTime
TEMPLATE = app

SOURCES += main.cpp\
    src/countdown-widget.cpp \
    src/tea-clock.cpp \
    src/timer-finished-dialog.cpp \
    src/tea-alarm.cpp \
    src/create-alarm-dialog.cpp \
    src/tea-alarm-progress-dialog.cpp \
    src/tea-alarm-manager.cpp \
    src/alarm-holder.cpp

HEADERS += src/countdown-widget.h \
    src/tea-clock.h \
    src/timer-finished-dialog.h \
    src/tea-alarm.h \
    src/create-alarm-dialog.h \
    src/tea-alarm-progress-dialog.h \
    src/tea-alarm-manager.h \
    src/alarm-holder.h

FORMS += src/tea-clock.ui\
    src/timer-finished-dialog.ui\
    src/create-alarm-dialog.ui\
    src/tea-alarm-progress-dialog.ui

RESOURCES += \
    src/teaclock.qrc

RC_FILE += \
    src/ttime.rc


target.path = /bin

ttime_resources.files = src/Resources/sounds
ttime_resources.path = /bin/Resources


MINGW_DIR = O:/_app/qt/mingw/bin
QT_BASE_DIR = O:/_app/qt/Desktop/Qt/4.7.4/mingw
QT_LIB_DIR = $$QT_BASE_DIR/bin
QT_PLUGIN_DIR = $$QT_BASE_DIR/plugins

ttime_files.files += $${MINGW_DIR}/mingwm10.dll \
    $${MINGW_DIR}/libgcc_s_dw2-1.dll \
    $${QT_LIB_DIR}/QtCore4.dll \
    $${QT_LIB_DIR}/QtGui4.dll \
    $${QT_PLUGIN_DIR}/iconengines \
    $${QT_PLUGIN_DIR}/imageformats
ttime_files.path = /bin
