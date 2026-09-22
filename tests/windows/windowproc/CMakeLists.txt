qm_find_qt(Core Widgets Test)
add_executable(tst_windowproc tst_windowproc.cpp)
set_target_properties(tst_windowproc PROPERTIES AUTOMOC ON)
target_compile_features(tst_windowproc PRIVATE cxx_std_17)
target_link_libraries(tst_windowproc PRIVATE QWKWidgets Qt${QT_VERSION_MAJOR}::Test user32)

add_test(NAME windows.windowproc
    COMMAND ${CMAKE_COMMAND} -E env
    "PATH=$<TARGET_FILE_DIR:QWKCore>;$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>;$ENV{PATH}"
    "QT_QPA_PLATFORM=windows" "QT_SCALE_FACTOR=1"
    ${CMAKE_COMMAND}
    "-DTEST_EXE=$<TARGET_FILE:tst_windowproc>"
    "-DRESULT_FILE=${CMAKE_CURRENT_BINARY_DIR}/windowproc-$<CONFIG>"
    "-DEXPECTED_PASSES=15"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/RunTest.cmake")
set_tests_properties(windows.windowproc PROPERTIES
    TIMEOUT 20 LABELS "windows;integration" RESOURCE_LOCK desktop)
