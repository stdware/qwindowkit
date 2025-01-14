project(QWKExample_QML)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)

add_executable(${PROJECT_NAME}
    ../shared/resources/shared.qrc
    main.cpp
)

set(QML_SOURCES main.qml QWKButton.qml)

set_source_files_properties(${QML_SOURCES}
    PROPERTIES QT_DISCARD_FILE_CONTENTS TRUE
)

qt_add_qml_module(${PROJECT_NAME}
    URI "${PROJECT_NAME}"
    RESOURCE_PREFIX "/"
    QML_FILES ${QML_SOURCES}
    NO_RESOURCE_TARGET_PATH
    #ENABLE_TYPE_COMPILER
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Quick
    QWKQuick
)

qt_import_qml_plugins(${PROJECT_NAME})
