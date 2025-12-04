project(QWKExample_AccentColor)

qwk_add_example(${PROJECT_NAME}
    FEATURES cxx_std_17
    SOURCES main.cpp
    QT_LINKS Core Gui
    LINKS QWKCore
)
