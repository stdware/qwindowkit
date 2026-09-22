qm_find_qt(Core Gui Test)

add_executable(tst_windowhittest tst_windowhittest.cpp)
set_target_properties(tst_windowhittest PROPERTIES AUTOMOC ON)
target_compile_features(tst_windowhittest PRIVATE cxx_std_17)
target_link_libraries(tst_windowhittest PRIVATE QWKCore Qt${QT_VERSION_MAJOR}::Test user32)
set(_expected_passes 2) # Qt Test initialization and cleanup.

if(QWINDOWKIT_BUILD_WIDGETS)
    target_link_libraries(tst_windowhittest PRIVATE QWKWidgets)
    target_compile_definitions(tst_windowhittest PRIVATE TEST_WIDGETS)
    math(EXPR _expected_passes "${_expected_passes} + 6")
endif()
if(QWINDOWKIT_BUILD_QUICK)
    target_link_libraries(tst_windowhittest PRIVATE QWKQuick)
    target_compile_definitions(tst_windowhittest PRIVATE TEST_QUICK)
    math(EXPR _expected_passes "${_expected_passes} + 6")
endif()

add_test(NAME windows.hittest
    COMMAND ${CMAKE_COMMAND} -E env
    "PATH=$<TARGET_FILE_DIR:QWKCore>;$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>;$ENV{PATH}"
    "QT_QPA_PLATFORM=windows" "QT_QUICK_BACKEND=software" "QT_SCALE_FACTOR=1"
    ${CMAKE_COMMAND}
    "-DTEST_EXE=$<TARGET_FILE:tst_windowhittest>"
    "-DRESULT_FILE=${CMAKE_CURRENT_BINARY_DIR}/hittest-$<CONFIG>"
    "-DEXPECTED_PASSES=${_expected_passes}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/RunTest.cmake")
set_tests_properties(windows.hittest PROPERTIES
    TIMEOUT 120 LABELS "windows;integration" RESOURCE_LOCK desktop)
