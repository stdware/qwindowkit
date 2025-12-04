#include <QGuiApplication>
#include <QWKCore/styleagent.h>
#include <QDebug>
#include <QTimer>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QWK::StyleAgent agent;
    QObject::connect(&agent, &QWK::StyleAgent::systemAccentColorChanged, [](){
        qDebug() << "Accent color changed!" << QWK::StyleAgent().systemAccentColor();
    });

    qDebug() << "Initial Accent Color:" << agent.systemAccentColor();

    return app.exec();
}
