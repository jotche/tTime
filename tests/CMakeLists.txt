find_package(Qt6 REQUIRED COMPONENTS Test)

# ttime_add_test(<name> [LIBS libraries...])   (default: ttime_core)
function(ttime_add_test name)
    cmake_parse_arguments(arg "" "" "LIBS" ${ARGN})
    if(NOT arg_LIBS)
        set(arg_LIBS ttime_core)
    endif()
    qt_add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE ${arg_LIBS} ttime_options Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
    # Windows: find the Qt DLLs without deploying them, and render with the real
    # (DirectWrite) text engine - widgets are grabbed without being shown.
    # Elsewhere: render off-screen so no display is needed.
    if(WIN32)
        set(env "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    else()
        set(env "QT_QPA_PLATFORM=set:offscreen")
    endif()
    set_tests_properties(${name} PROPERTIES ENVIRONMENT_MODIFICATION "${env}")
endfunction()

# Logic (Qt Core)
ttime_add_test(tst_alarmspec)
ttime_add_test(tst_alarm)
ttime_add_test(tst_wavinfo)
target_compile_definitions(tst_wavinfo PRIVATE
    TTIME_ALARM_WAV="${PROJECT_SOURCE_DIR}/src/Resources/sounds/submarine1.wav")
ttime_add_test(tst_windowplacement)
ttime_add_test(tst_digitlayout)
ttime_add_test(tst_appsettings)
ttime_add_test(tst_fontweight)

# Themes, icon, fonts, configuration (Qt Gui)
ttime_add_test(tst_defaultsettings LIBS ttime_gui)
target_compile_definitions(tst_defaultsettings PRIVATE
    TTIME_DEFAULT_INI="${PROJECT_SOURCE_DIR}/src/tTime.ini")
ttime_add_test(tst_theme LIBS ttime_gui)
ttime_add_test(tst_bellanimation LIBS ttime_gui)
ttime_add_test(tst_treesway LIBS ttime_gui)

# Windows and the alarm manager (Qt Widgets)
ttime_add_test(tst_countdownlabel LIBS ttime_ui)
target_compile_definitions(tst_countdownlabel PRIVATE
    TTIME_FONT_DIR="${PROJECT_SOURCE_DIR}/src/Resources/fonts")
ttime_add_test(tst_countdownwindow LIBS ttime_ui)
ttime_add_test(tst_alarmmanager LIBS ttime_ui)
