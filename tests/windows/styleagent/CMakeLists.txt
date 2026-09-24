qm_find_qt(Core Gui Test)
add_executable(tst_windowsstyleagent tst_windowsstyleagent.cpp)
set_target_properties(tst_windowsstyleagent PROPERTIES AUTOMOC ON)
target_compile_features(tst_windowsstyleagent PRIVATE cxx_std_17)
target_link_libraries(tst_windowsstyleagent PRIVATE QWKCore Qt${QT_VERSION_MAJOR}::Test user32)

add_test(NAME windows.styleagent
    COMMAND ${CMAKE_COMMAND} -E env
    "PATH=$<TARGET_FILE_DIR:QWKCore>;$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>;$ENV{PATH}"
    "QT_QPA_PLATFORM=windows"
    ${CMAKE_COMMAND}
    "-DTEST_EXE=$<TARGET_FILE:tst_windowsstyleagent>"
    "-DRESULT_FILE=${CMAKE_CURRENT_BINARY_DIR}/styleagent-$<CONFIG>"
    "-DEXPECTED_PASSES=9"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/../windowproc/RunTest.cmake")
set_tests_properties(windows.styleagent PROPERTIES
    TIMEOUT 20 LABELS "windows;integration;native-message" RESOURCE_LOCK desktop)
