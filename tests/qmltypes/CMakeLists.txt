qm_find_qt(Quick)
if(QT_VERSION VERSION_LESS 6.3)
    return()
endif()

add_test(NAME qmltypes.installed
    COMMAND ${CMAKE_COMMAND}
        "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/consumer"
        "-DBUILD_DIR=${PROJECT_BINARY_DIR}"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/installed-$<CONFIG>"
        "-DBUILD_CONFIG=$<CONFIG>"
        "-DGENERATOR=${CMAKE_GENERATOR}"
        "-DGENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
        "-DGENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
        "-DQT_DIR=${Qt6_DIR}"
        "-DQT_BIN_DIR=$<TARGET_FILE_DIR:Qt6::Core>"
        "-DINSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}"
        "-DQT_QMLLINT=$<TARGET_FILE:Qt6::qmllint>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/RunInstalled.cmake"
)
set_tests_properties(qmltypes.installed PROPERTIES TIMEOUT 180 LABELS "qml;install;windows")
