qm_find_qt(Core Gui Quick Test)

add_executable(tst_quickgeometry tst_quickgeometry.cpp)
set_target_properties(tst_quickgeometry PROPERTIES AUTOMOC ON)
target_compile_features(tst_quickgeometry PRIVATE cxx_std_17)
target_link_libraries(tst_quickgeometry PRIVATE QWKQuick Qt${QT_VERSION_MAJOR}::Test)

add_test(NAME quickgeometry.unit
    COMMAND ${CMAKE_COMMAND} -E env
    "PATH=$<TARGET_FILE_DIR:QWKQuick>;$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>;$ENV{PATH}"
    "QT_QPA_PLATFORM=offscreen"
    ${CMAKE_COMMAND}
    "-DTEST_EXE=$<TARGET_FILE:tst_quickgeometry>"
    "-DRESULT_FILE=${CMAKE_CURRENT_BINARY_DIR}/geometry-$<CONFIG>"
    "-DEXPECTED_PASSES=14"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/../windows/hittest/RunTest.cmake")
set_tests_properties(quickgeometry.unit PROPERTIES TIMEOUT 30 LABELS "quick;unit")
