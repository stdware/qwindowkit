qm_find_qt(Core Widgets Test)

add_executable(tst_widgetborder tst_widgetborder.cpp)
set_target_properties(tst_widgetborder PROPERTIES AUTOMOC ON)
target_compile_features(tst_widgetborder PRIVATE cxx_std_17)
target_link_libraries(tst_widgetborder PRIVATE QWKWidgets Qt${QT_VERSION_MAJOR}::Test)

# Each destructive data row starts a child process. The factory selects the production
# border handler on every Windows version, while its draw hook records calls instead of GDI.
add_test(NAME widgetborder.lifetime
    COMMAND ${CMAKE_COMMAND}
        "-DTEST_EXE=$<TARGET_FILE:tst_widgetborder>"
        "-DQWK_BIN_DIR=$<TARGET_FILE_DIR:QWKCore>"
        "-DQT_BIN_DIR=$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>"
        "-DRESULT_FILE=${CMAKE_CURRENT_BINARY_DIR}/widgetborder-$<CONFIG>"
        "-DEXPECTED_PASSES=15"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/../unit/RunTest.cmake")
set_tests_properties(widgetborder.lifetime PROPERTIES TIMEOUT 10 LABELS "widgets;windows;component")
