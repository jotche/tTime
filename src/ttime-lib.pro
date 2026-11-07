# Everything but main(): linked by tTime.exe and the tests.
include($$PWD/../common.pri)

TEMPLATE = lib
CONFIG += staticlib
TARGET = ttime
DESTDIR = $$OUT_PWD

HEADERS += \
    alarm-holder.h \
    alarm-manager.h \
    alarm-spec.h \
    alarm.h \
    app-config.h \
    app-settings.h \
    bell-animation.h \
    countdown-font.h \
    countdown-label.h \
    countdown-window.h \
    digit-layout.h \
    font-weight.h \
    image-layers.h \
    new-alarm-window.h \
    theme.h \
    time-up-window.h \
    tree-sway.h \
    wav-info.h \
    window-placement.h \
    window-slot.h \
    window-utils.h

SOURCES += \
    alarm-holder.cpp \
    alarm-manager.cpp \
    alarm-spec.cpp \
    alarm.cpp \
    app-config.cpp \
    app-settings.cpp \
    bell-animation.cpp \
    countdown-font.cpp \
    countdown-label.cpp \
    countdown-window.cpp \
    digit-layout.cpp \
    font-weight.cpp \
    image-layers.cpp \
    new-alarm-window.cpp \
    theme.cpp \
    time-up-window.cpp \
    tree-sway.cpp \
    wav-info.cpp \
    window-placement.cpp \
    window-slot.cpp

FORMS += \
    countdown-window.ui \
    new-alarm-window.ui \
    time-up-window.ui
