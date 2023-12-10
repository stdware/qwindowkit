#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
    qputenv("QT_WIN_DEBUG_CONSOLE", "1");
    qputenv("QSG_INFO", "1");
    qputenv("QT_WIDGETS_HIGHDPI_DOWNSCALE", "1");
#if 0
    qputenv("QT_WIDGETS_RHI", "1");
    qputenv("QSG_RHI_BACKEND", "d3d12");
    qputenv("QSG_RHI_HDR", "scrgb");
    qputenv("QT_QPA_DISABLE_REDIRECTION_SURFACE", "1");
#endif
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}