qm_find_qt(Core Gui Quick Test)

add_executable(tst_quickgeometry tst_quickgeometry.cpp)
set_target_properties(tst_quickgeometry PROPERTIES AUTOMOC ON)
target_compile_features(tst_quickgeometry PRIVATE cxx_std_17)
target_link_libraries(tst_quickgeometry PRIVATE QWKQuick Qt${QT_VERSION_MAJOR}::Test)

qwk_register_fast_test(tst_quickgeometry quickgeometry.unit 14)
set_property(TEST quickgeometry.unit APPEND PROPERTY LABELS quick)

# Real QML module/agent integration, with a hidden window and no context substitute.
add_executable(tst_systembuttons tst_systembuttons.cpp)
set_target_properties(tst_systembuttons PROPERTIES AUTOMOC ON)
target_compile_features(tst_systembuttons PRIVATE cxx_std_17)
target_compile_definitions(tst_systembuttons PRIVATE
    QWK_TEST_QML_IMPORT_PATH="${QWindowKit_QML_IMPORT_PATH}")
target_link_libraries(tst_systembuttons PRIVATE QWKQuick Qt${QT_VERSION_MAJOR}::Test)
qwk_register_fast_test(tst_systembuttons systembuttons.qml 13)
set_tests_properties(systembuttons.qml PROPERTIES LABELS "component;quick;fast;windows")
