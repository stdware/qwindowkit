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

# Compile the unchanged private implementation so the test can supply a derived
# context with page-protected freed storage. All HWNDs and menu loops are native.
add_executable(tst_windowlifetime tst_windowlifetime.cpp)
if(NOT QWINDOWKIT_BUILD_STATIC)
    target_sources(tst_windowlifetime PRIVATE
        ../../../src/core/contexts/win32windowcontext.cpp
        ../../../src/core/contexts/win32windowcontext_p.h)
endif()
if(QT_VERSION_MAJOR EQUAL 6)
    foreach(_module Core Gui)
        if(NOT TARGET Qt6::${_module}Private)
            find_package(Qt6 REQUIRED COMPONENTS ${_module}Private)
        endif()
    endforeach()
    unset(_module)
endif()
set_target_properties(tst_windowlifetime PROPERTIES AUTOMOC ON)
target_compile_features(tst_windowlifetime PRIVATE cxx_std_17)
target_include_directories(tst_windowlifetime PRIVATE
    ../../../src/core ../../../src/core/contexts ../../../src/core/shared)
target_link_libraries(tst_windowlifetime PRIVATE QWKWidgets Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::CorePrivate Qt${QT_VERSION_MAJOR}::GuiPrivate
    user32 gdi32 shell32 uxtheme)
add_test(NAME windows.windowlifetime
    COMMAND ${CMAKE_COMMAND} -E env
    "PATH=$<TARGET_FILE_DIR:QWKCore>;$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>;$ENV{PATH}"
    "QT_QPA_PLATFORM=windows" "QT_SCALE_FACTOR=1"
    ${CMAKE_COMMAND}
    "-DTEST_EXE=$<TARGET_FILE:tst_windowlifetime>"
    "-DRESULT_FILE=${CMAKE_CURRENT_BINARY_DIR}/windowlifetime-$<CONFIG>"
    "-DEXPECTED_PASSES=23"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/RunTest.cmake")
set_tests_properties(windows.windowlifetime PROPERTIES
    TIMEOUT 20 LABELS "windows;integration" RESOURCE_LOCK desktop)
