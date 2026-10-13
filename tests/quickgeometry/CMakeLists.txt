qm_find_qt(Core Gui Quick Test)

add_executable(tst_quickgeometry tst_quickgeometry.cpp)
set_target_properties(tst_quickgeometry PROPERTIES AUTOMOC ON)
target_compile_features(tst_quickgeometry PRIVATE cxx_std_17)
target_link_libraries(tst_quickgeometry PRIVATE QWKQuick Qt${QT_VERSION_MAJOR}::Test)

qwk_register_fast_test(tst_quickgeometry quickgeometry.unit 14)
set_property(TEST quickgeometry.unit APPEND PROPERTY LABELS quick)
