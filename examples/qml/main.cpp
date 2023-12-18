#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>

#include <QWKQuick/qwkquickglobal.h>

int main(int argc, char *argv[]) {
    qputenv("QT_WIN_DEBUG_CONSOLE", "1");
    qputenv("QSG_INFO", "1");
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
#if 1
    qputenv("QSG_RHI_BACKEND", "d3d12");
    qputenv("QSG_RHI_HDR", "scrgb");
    qputenv("QT_QPA_DISABLE_REDIRECTION_SURFACE", "1");
#endif
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QGuiApplication application(argc, argv);
    QQmlApplicationEngine engine;
    QWK::registerTypes(&engine);
    engine.load(QUrl(QStringLiteral("qrc:///main.qml")));
    return application.exec();
}