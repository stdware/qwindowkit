// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <iostream>
#include <QWKQuick/qwkquickglobal.h>
#include <QWKCore/windowagentbase.h>

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    if (argc != 3)
        return 1;

    // Exercise the public registration entry point repeatedly and across engines.
    // This also catches static archives dropping Qt's generated registration object.
    int registeredType = -1;
    for (int i = 0; i < 2; ++i) {
        bool warnings = false;
        QQmlApplicationEngine engine;
        engine.addImportPath(QString::fromLocal8Bit(argv[2]));
        QWK::registerTypes(&engine);
        QWK::registerTypes(&engine);
        const int type = qmlTypeId("QWindowKit", 1, 0, "WindowAgent");
        if (type < 0 || (i > 0 && registeredType != type))
            return 4;
        registeredType = type;
        QObject::connect(&engine, &QQmlEngine::warnings, [&warnings](const QList<QQmlError> &errors) {
            warnings = true;
            for (const auto &error : errors)
                std::cerr << error.toString().toStdString() << '\n';
        });
        engine.load(QUrl::fromLocalFile(QString::fromLocal8Bit(argv[1])));
        if (warnings || engine.rootObjects().size() != 1)
            return 2;
        if (qmlTypeId("QWindowKit", 1, 0, "WindowAgent") != registeredType)
            return 5;
        const auto window = engine.rootObjects().first();
        if (!window->property("initialized").toBool()
            || window->property("minimizeRole").toInt() != QWK::WindowAgentBase::Minimize)
            return 3;
    }
    return 0;
}
