# The qmake half of the build system regression tests. Built out of source by
# `RunQmakeBuild.cmake` against a staged install tree, which passes `QWK_PREFIX` and `QWK_MODULES`
# on the command line.

TEMPLATE = app
TARGET = qwk_qmake_consumer

CONFIG += console c++17
CONFIG -= app_bundle

# One configuration per invocation. `debug_and_release` is on by default for the MSVC mkspecs, and
# would have the make tool build both, while only the one QWindowKit was built in has libraries
# installed to link against.
CONFIG -= debug_and_release debug_and_release_target

QT += core gui

isEmpty(QWK_PREFIX): error("QWK_PREFIX must be given on the qmake command line.")

QWK_QMAKE_DIR = $$QWK_PREFIX/share/QWindowKit/qmake

contains(QWK_MODULES, widgets) {
    QT += widgets
    DEFINES += CONSUMER_USE_WIDGETS

    # Pulls in QWKCore.pri itself, which is part of what is being tested.
    !include($$QWK_QMAKE_DIR/QWKWidgets.pri): error("cannot include $$QWK_QMAKE_DIR/QWKWidgets.pri")
} else {
    !include($$QWK_QMAKE_DIR/QWKCore.pri): error("cannot include $$QWK_QMAKE_DIR/QWKCore.pri")
}

SOURCES += $$PWD/../consumer.cpp
