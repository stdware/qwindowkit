#include "mainwindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QGuiApplication>

int main(int argc, char *argv[])
{
    /*
        QWindowKit recommends setting this before QApplication is created.

         Reason:
         Some native-child-widget scenarios can make Qt promote sibling widgets
         to native widgets. In a frameless/native-hit-test window, unexpected
         native siblings can cause z-order, mouse event, and painting problems.
     */
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}