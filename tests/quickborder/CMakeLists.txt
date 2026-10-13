qm_find_qt(Core Quick Test)

add_executable(tst_quickborder tst_quickborder.cpp)
set_target_properties(tst_quickborder PROPERTIES AUTOMOC ON)
target_compile_features(tst_quickborder PRIVATE cxx_std_17)
target_link_libraries(tst_quickborder PRIVATE QWKQuick Qt${QT_VERSION_MAJOR}::Test)

# Each scale runs in a fresh process. No network, external images or input driver is needed.
foreach(_scale IN ITEMS 1 1.25 1.5 2)
    add_test(NAME quickborder.software.${_scale}
        COMMAND ${CMAKE_COMMAND} -E env
        "PATH=$<TARGET_FILE_DIR:QWKQuick>;$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>;$ENV{PATH}"
        "QT_QPA_PLATFORM=windows" "QT_QUICK_BACKEND=software"
        "QT_ENABLE_HIGHDPI_SCALING=0" "QT_SCALE_FACTOR=${_scale}"
        ${CMAKE_COMMAND}
        "-DTEST_EXE=$<TARGET_FILE:tst_quickborder>"
        "-DRESULT_FILE=${CMAKE_CURRENT_BINARY_DIR}/software-${_scale}-$<CONFIG>.txt"
        "-DTEST_FUNCTIONS=agentBeforeWindow;windowBeforeAgent;geometryColorAndSceneAttachment"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/RunTest.cmake")
    set_tests_properties(quickborder.software.${_scale} PROPERTIES
        TIMEOUT 20 LABELS "quick;windows;ui" RESOURCE_LOCK desktop)
endforeach()

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    add_test(NAME quickborder.native.d3d11
        COMMAND ${CMAKE_COMMAND} -E env
        "PATH=$<TARGET_FILE_DIR:QWKQuick>;$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>;$ENV{PATH}"
        "QT_QPA_PLATFORM=windows" "QT_QUICK_BACKEND=" "QSG_RHI_BACKEND=d3d11"
        "QSG_RHI_PREFER_SOFTWARE_RENDERER=1" "QSG_RENDER_LOOP=threaded"
        ${CMAKE_COMMAND}
        "-DTEST_EXE=$<TARGET_FILE:tst_quickborder>"
        "-DRESULT_FILE=${CMAKE_CURRENT_BINARY_DIR}/native-d3d11-$<CONFIG>.txt"
        "-DTEST_FUNCTIONS=nativeSurfaceLifecycle"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/RunTest.cmake")
    set_tests_properties(quickborder.native.d3d11 PROPERTIES
        TIMEOUT 20 LABELS "quick;windows;ui" RESOURCE_LOCK desktop)
endif()
