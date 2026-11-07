# Links the static tTime library (src/ttime-lib.pro), built in the src directory of
# the build tree. Set TTIME_LIB_DIR before including this from another directory.
isEmpty(TTIME_LIB_DIR): TTIME_LIB_DIR = $$OUT_PWD

LIBS += -L$$TTIME_LIB_DIR -lttime
win32-g++*: PRE_TARGETDEPS += $$TTIME_LIB_DIR/libttime.a
else:win32: PRE_TARGETDEPS += $$TTIME_LIB_DIR/ttime.lib
else: PRE_TARGETDEPS += $$TTIME_LIB_DIR/libttime.a
