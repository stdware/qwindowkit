qm_find_qt(Core Gui Quick Test)

add_executable(tst_quickgeometry tst_quickgeometry.cpp)
set_target_properties(tst_quickgeometry PROPERTIES AUTOMOC ON)
target_compile_features(tst_quickgeometry PRIVATE cxx_std_17)
target_link_libraries(tst_quickgeometry PRIVATE QWKQuick Qt${QT_VERSION_MAJOR}::Test)

qwk_register_fast_test(tst_quickgeometry quickgeometry.unit 14)
set_property(TEST quickgeometry.unit APPEND PROPERTY LABELS quick)

# Compile the unchanged private observer into the shared-library test; static
# builds link the implementation already present in QWKQuick.
add_executable(tst_quicksystembuttonarea tst_quicksystembuttonarea.cpp)
set_target_properties(tst_quicksystembuttonarea PROPERTIES AUTOMOC ON)
target_compile_features(tst_quicksystembuttonarea PRIVATE cxx_std_17)
target_link_libraries(tst_quicksystembuttonarea PRIVATE QWKQuick Qt${QT_VERSION_MAJOR}::Test)
if(NOT QWINDOWKIT_BUILD_STATIC)
    target_sources(tst_quicksystembuttonarea PRIVATE
        ../../src/quick/quicksystembuttonarea.cpp ../../src/quick/quicksystembuttonarea_p.h)
endif()
qwk_register_fast_test(tst_quicksystembuttonarea quicksystembuttonarea.component 29)
set_tests_properties(quicksystembuttonarea.component PROPERTIES LABELS "component;quick;fast")

# Real QML module/agent integration, with a hidden window and no context substitute.
add_executable(tst_systembuttons tst_systembuttons.cpp)
set_target_properties(tst_systembuttons PROPERTIES AUTOMOC ON)
target_compile_features(tst_systembuttons PRIVATE cxx_std_17)
target_compile_definitions(tst_systembuttons PRIVATE
    QWK_TEST_QML_IMPORT_PATH="${QWindowKit_QML_IMPORT_PATH}")
target_link_libraries(tst_systembuttons PRIVATE QWKQuick Qt${QT_VERSION_MAJOR}::Test)
qwk_register_fast_test(tst_systembuttons systembuttons.qml 13)
set_tests_properties(systembuttons.qml PROPERTIES LABELS "component;quick;fast;windows")
