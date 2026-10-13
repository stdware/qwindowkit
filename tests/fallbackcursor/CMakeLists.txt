qm_find_qt(Core Gui Test)

add_executable(tst_fallbackcursor tst_fallbackcursor.cpp)
set_target_properties(tst_fallbackcursor PROPERTIES AUTOMOC ON)
target_compile_features(tst_fallbackcursor PRIVATE cxx_std_17)
target_link_libraries(tst_fallbackcursor PRIVATE QWKCore Qt${QT_VERSION_MAJOR}::Test)
if(NOT QWINDOWKIT_BUILD_STATIC)
    target_sources(tst_fallbackcursor PRIVATE
        ../../src/core/contexts/qtwindowcontext.cpp ../../src/core/contexts/qtwindowcontext_p.h)
    target_include_directories(tst_fallbackcursor PRIVATE ../../src/core ../../src/core/shared)
endif()
set(_passes 2)
if(QWINDOWKIT_BUILD_WIDGETS)
    target_link_libraries(tst_fallbackcursor PRIVATE QWKWidgets)
    target_compile_definitions(tst_fallbackcursor PRIVATE TEST_WIDGETS)
    math(EXPR _passes "${_passes} + 36")
endif()
if(QWINDOWKIT_BUILD_QUICK)
    target_link_libraries(tst_fallbackcursor PRIVATE QWKQuick)
    target_compile_definitions(tst_fallbackcursor PRIVATE TEST_QUICK)
    math(EXPR _passes "${_passes} + 32")
endif()
foreach(_platform offscreen windows)
    add_test(NAME fallbackcursor.${_platform}
        COMMAND ${CMAKE_COMMAND} -E env
        "PATH=$<TARGET_FILE_DIR:QWKCore>;$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>;$ENV{PATH}"
        "QT_QPA_PLATFORM=${_platform}" "QT_QUICK_BACKEND=software"
        ${CMAKE_COMMAND}
        "-DTEST_EXE=$<TARGET_FILE:tst_fallbackcursor>"
        "-DRESULT_FILE=${CMAKE_CURRENT_BINARY_DIR}/cursor-${_platform}-$<CONFIG>"
        "-DEXPECTED_PASSES=${_passes}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/RunTest.cmake")
    set_tests_properties(fallbackcursor.${_platform} PROPERTIES TIMEOUT 45)
endforeach()
set_tests_properties(fallbackcursor.offscreen PROPERTIES LABELS component)
set_tests_properties(fallbackcursor.windows PROPERTIES LABELS "windows;integration" RESOURCE_LOCK desktop)
