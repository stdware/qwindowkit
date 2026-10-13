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

add_executable(tst_acrylic tst_acrylic.cpp)
if(NOT QWINDOWKIT_BUILD_STATIC)
    target_sources(tst_acrylic PRIVATE
        ../../../src/core/contexts/win32windowcontext.cpp
        ../../../src/core/contexts/win32windowcontext_p.h)
endif()
set_target_properties(tst_acrylic PROPERTIES AUTOMOC ON)
target_compile_features(tst_acrylic PRIVATE cxx_std_17)
target_include_directories(tst_acrylic PRIVATE
    ../../../src/core ../../../src/core/contexts ../../../src/core/shared)
target_link_libraries(tst_acrylic PRIVATE QWKWidgets Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::CorePrivate Qt${QT_VERSION_MAJOR}::GuiPrivate
    user32 gdi32 shell32 uxtheme)
set(_acrylic_passes 94)
if(QWINDOWKIT_BUILD_QUICK)
    target_compile_definitions(tst_acrylic PRIVATE TEST_QUICK)
    target_link_libraries(tst_acrylic PRIVATE QWKQuick)
    set(_acrylic_passes 96)
endif()
add_test(NAME windows.acrylic
    COMMAND ${CMAKE_COMMAND} -E env
    "PATH=$<TARGET_FILE_DIR:QWKCore>;$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>;$ENV{PATH}"
    "QT_QPA_PLATFORM=windows" "QT_QUICK_BACKEND=software"
    ${CMAKE_COMMAND}
    "-DTEST_EXE=$<TARGET_FILE:tst_acrylic>"
    "-DRESULT_FILE=${CMAKE_CURRENT_BINARY_DIR}/acrylic-$<CONFIG>"
    "-DEXPECTED_PASSES=${_acrylic_passes}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/RunTest.cmake")
set_tests_properties(windows.acrylic PROPERTIES
    TIMEOUT 20 LABELS "windows;integration;component" RESOURCE_LOCK desktop)
unset(_acrylic_passes)

add_executable(tst_effects tst_effects.cpp)
if(NOT QWINDOWKIT_BUILD_STATIC)
    target_sources(tst_effects PRIVATE
        ../../../src/core/contexts/win32windowcontext.cpp
        ../../../src/core/contexts/win32windowcontext_p.h)
endif()
set_target_properties(tst_effects PROPERTIES AUTOMOC ON)
target_compile_features(tst_effects PRIVATE cxx_std_17)
target_include_directories(tst_effects PRIVATE
    ../../../src/core ../../../src/core/contexts ../../../src/core/shared)
target_link_libraries(tst_effects PRIVATE QWKWidgets Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::CorePrivate Qt${QT_VERSION_MAJOR}::GuiPrivate
    user32 gdi32 shell32 uxtheme)
set(_effects_passes 204)
if(QWINDOWKIT_BUILD_QUICK)
    target_compile_definitions(tst_effects PRIVATE TEST_QUICK)
    target_link_libraries(tst_effects PRIVATE QWKQuick)
    set(_effects_passes 205)
endif()
add_test(NAME windows.effects
    COMMAND ${CMAKE_COMMAND} -E env
    "PATH=$<TARGET_FILE_DIR:QWKCore>;$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>;$ENV{PATH}"
    "QT_QPA_PLATFORM=windows" "QT_QUICK_BACKEND=software"
    ${CMAKE_COMMAND}
    "-DTEST_EXE=$<TARGET_FILE:tst_effects>"
    "-DRESULT_FILE=${CMAKE_CURRENT_BINARY_DIR}/effects-$<CONFIG>"
    "-DEXPECTED_PASSES=${_effects_passes}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/RunTest.cmake")
set_tests_properties(windows.effects PROPERTIES
    TIMEOUT 20 LABELS "windows;integration;component" RESOURCE_LOCK desktop)
unset(_effects_passes)

add_executable(tst_titlebar tst_titlebar.cpp)
set_target_properties(tst_titlebar PROPERTIES AUTOMOC ON)
target_compile_features(tst_titlebar PRIVATE cxx_std_17)
target_link_libraries(tst_titlebar PRIVATE QWKWidgets Qt${QT_VERSION_MAJOR}::Test user32)
set(_titlebar_passes 5)
if(QWINDOWKIT_BUILD_QUICK)
    target_compile_definitions(tst_titlebar PRIVATE TEST_QUICK)
    target_link_libraries(tst_titlebar PRIVATE QWKQuick)
    set(_titlebar_passes 8)
endif()
add_test(NAME windows.titlebar
    COMMAND ${CMAKE_COMMAND} -E env
    "PATH=$<TARGET_FILE_DIR:QWKCore>;$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>;$ENV{PATH}"
    "QT_QPA_PLATFORM=windows" "QT_QUICK_BACKEND=software" "QT_SCALE_FACTOR=1"
    ${CMAKE_COMMAND}
    "-DTEST_EXE=$<TARGET_FILE:tst_titlebar>"
    "-DRESULT_FILE=${CMAKE_CURRENT_BINARY_DIR}/titlebar-$<CONFIG>"
    "-DEXPECTED_PASSES=${_titlebar_passes}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/RunTest.cmake")
set_tests_properties(windows.titlebar PROPERTIES
    TIMEOUT 20 LABELS "windows;integration" RESOURCE_LOCK desktop)
unset(_titlebar_passes)

if(QWINDOWKIT_ENABLE_WINDOWS_SYSTEM_BORDERS)
    add_executable(tst_borderattributes tst_borderattributes.cpp)
    if(NOT QWINDOWKIT_BUILD_STATIC)
        target_sources(tst_borderattributes PRIVATE
            ../../../src/core/contexts/win32windowcontext.cpp
            ../../../src/core/contexts/win32windowcontext_p.h)
    endif()
    set_target_properties(tst_borderattributes PROPERTIES AUTOMOC ON)
    target_compile_features(tst_borderattributes PRIVATE cxx_std_17)
    target_include_directories(tst_borderattributes PRIVATE
        ../../../src/core ../../../src/core/contexts ../../../src/core/shared)
    target_link_libraries(tst_borderattributes PRIVATE QWKWidgets Qt${QT_VERSION_MAJOR}::Test
        Qt${QT_VERSION_MAJOR}::CorePrivate Qt${QT_VERSION_MAJOR}::GuiPrivate
        user32 gdi32 shell32 uxtheme)
    set(_border_attribute_passes 22)
    if(QWINDOWKIT_BUILD_QUICK)
        target_compile_definitions(tst_borderattributes PRIVATE TEST_QUICK)
        target_link_libraries(tst_borderattributes PRIVATE QWKQuick)
        set(_border_attribute_passes 42)
    endif()
    add_test(NAME windows.borderattributes
        COMMAND ${CMAKE_COMMAND} -E env
        "PATH=$<TARGET_FILE_DIR:QWKCore>;$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>;$ENV{PATH}"
        "QT_QPA_PLATFORM=windows" "QT_QUICK_BACKEND=software"
        ${CMAKE_COMMAND}
        "-DTEST_EXE=$<TARGET_FILE:tst_borderattributes>"
        "-DRESULT_FILE=${CMAKE_CURRENT_BINARY_DIR}/borderattributes-$<CONFIG>"
        "-DEXPECTED_PASSES=${_border_attribute_passes}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/RunTest.cmake")
    set_tests_properties(windows.borderattributes PROPERTIES
        TIMEOUT 20 LABELS "windows;integration;component" RESOURCE_LOCK desktop)
    unset(_border_attribute_passes)
endif()
