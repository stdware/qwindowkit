cmake_minimum_required(VERSION 3.19)
project(qwk_qml_consumer LANGUAGES CXX)

find_package(QWindowKit REQUIRED COMPONENTS Quick)
if(NOT EXISTS "${QWindowKit_QML_IMPORT_PATH}/QWindowKit/QWKQuick.qmltypes")
    message(FATAL_ERROR "The package did not expose a usable QML import path")
endif()

add_executable(qwk_qml_consumer main.cpp)
target_compile_features(qwk_qml_consumer PRIVATE cxx_std_17)
target_link_libraries(qwk_qml_consumer PRIVATE QWindowKit::Quick)
set_target_properties(qwk_qml_consumer PROPERTIES AUTOMOC ON)
qt6_add_qml_module(qwk_qml_consumer
    URI QwkConsumer
    VERSION 1.0
    RESOURCE_PREFIX /test
    IMPORT_PATH "${QWindowKit_QML_IMPORT_PATH}"
    QML_FILES Window.qml
)
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/consumer-$<CONFIG>.txt"
    CONTENT "$<TARGET_FILE:qwk_qml_consumer>")
