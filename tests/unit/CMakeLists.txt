qm_find_qt(Core Gui Test)

add_custom_target(qwk_fast_unit_tests)

# Only Qt and the production libraries: no downloads, external test runner or desktop.
function(qwk_register_fast_test target name expected_passes)
    add_dependencies(qwk_fast_unit_tests ${target})
    add_test(NAME ${name}
        COMMAND ${CMAKE_COMMAND}
        "-DTEST_EXE=$<TARGET_FILE:${target}>"
        "-DQWK_BIN_DIR=$<TARGET_FILE_DIR:QWKCore>"
        "-DQT_BIN_DIR=$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>"
        "-DRESULT_FILE=${CMAKE_CURRENT_BINARY_DIR}/${name}-$<CONFIG>"
        "-DEXPECTED_PASSES=${expected_passes}"
        -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/RunTest.cmake")
    set_tests_properties(${name} PROPERTIES TIMEOUT 10 LABELS "unit;fast")
endfunction()

function(qwk_add_unit_test target name expected_passes)
    add_executable(${target} ${target}.cpp)
    set_target_properties(${target} PROPERTIES AUTOMOC ON)
    target_compile_features(${target} PRIVATE cxx_std_17)
    target_link_libraries(${target} PRIVATE QWKCore Qt${QT_VERSION_MAJOR}::Test)
    qwk_register_fast_test(${target} ${name} ${expected_passes})
endfunction()

qwk_add_unit_test(tst_eventdispatch core.eventdispatch.unit 24)
qwk_add_unit_test(tst_dispatchlifetime core.dispatchlifetime.unit 10)
qwk_add_unit_test(tst_objecteventfilters core.objecteventfilters.unit 7)
qwk_add_unit_test(tst_windowcontext core.windowcontext.unit 28)
qwk_add_unit_test(tst_windowmovegeometry core.windowmovegeometry.unit 24)
qwk_add_unit_test(tst_windowmove core.windowmove.component 10)
set_tests_properties(core.windowmove.component PROPERTIES LABELS "component;fast")
qwk_add_unit_test(tst_qtwindowcontext core.qtwindowcontext.unit 184)
if(NOT QWINDOWKIT_BUILD_STATIC)
    # The fallback class is private and not exported by shared QWKCore. Compile its
    # unchanged implementation here, using the exported production base/delegate.
    # Static builds already contain these symbols (including moc), so link them directly.
    target_sources(tst_qtwindowcontext PRIVATE
        ../../src/core/contexts/qtwindowcontext.cpp ../../src/core/contexts/qtwindowcontext_p.h)
    target_include_directories(tst_qtwindowcontext PRIVATE ../../src/core ../../src/core/shared)
endif()

if(QWINDOWKIT_ENABLE_STYLE_AGENT)
    # StyleAgentPrivate is not exported. Compile the real notification implementation into
    # this test, with deterministic platform-hook substitutes, without linking QWKCore.
    add_executable(tst_styleagent tst_styleagent.cpp
        ../../src/core/style/styleagent.cpp ../../src/core/style/styleagent.h)
    set_target_properties(tst_styleagent PROPERTIES AUTOMOC ON)
    target_compile_features(tst_styleagent PRIVATE cxx_std_17)
    target_compile_definitions(tst_styleagent PRIVATE QWK_CORE_STATIC)
    target_include_directories(tst_styleagent PRIVATE
        "$<TARGET_PROPERTY:QWKCore,INTERFACE_INCLUDE_DIRECTORIES>")
    target_link_libraries(tst_styleagent PRIVATE Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Test)
    add_dependencies(tst_styleagent QWKCore)
    qwk_register_fast_test(tst_styleagent core.styleagent.unit 11)
endif()

if(QWINDOWKIT_BUILD_WIDGETS OR QWINDOWKIT_BUILD_QUICK)
    set(_agent_passes 2) # Qt Test initialization and cleanup.
    if(QWINDOWKIT_BUILD_WIDGETS)
        math(EXPR _agent_passes "${_agent_passes} + 5")
    endif()
    if(QWINDOWKIT_BUILD_QUICK)
        math(EXPR _agent_passes "${_agent_passes} + 5")
    endif()
    qwk_add_unit_test(tst_windowagents agents.unit ${_agent_passes})
    if(QWINDOWKIT_BUILD_WIDGETS)
        target_link_libraries(tst_windowagents PRIVATE QWKWidgets)
        target_compile_definitions(tst_windowagents PRIVATE TEST_WIDGETS)
    endif()
    if(QWINDOWKIT_BUILD_QUICK)
        target_link_libraries(tst_windowagents PRIVATE QWKQuick)
        target_compile_definitions(tst_windowagents PRIVATE TEST_QUICK)
    endif()
endif()
