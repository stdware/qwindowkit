#include "mainwindow.h"

#include <QtCore/QDebug>

#include <QWKWidgets/widgetwindowagent.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    auto agent = new QWK::WidgetWindowAgent(this);
    if (!agent->setup(this)) {
        qDebug() << "Frameless handle failed to initialize.";
    }
}

MainWindow::~MainWindow() {
}
