// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <memory>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtTest/QTest>
#include <QtCore/qt_windows.h>
#include <QWKQuick/qwkquickglobal.h>
#include "../unit/systembuttoncases.h"

class SystemButtonsTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void numericRoles_data() {
        QTest::addColumn<int>("value");
        QTest::addColumn<bool>("valid");
        for (const auto &row : QwkTest::systemButtonCases)
            QTest::newRow(row.name) << row.value << row.valid;
    }

    void numericRoles() {
        QFETCH(int, value);
        QFETCH(bool, valid);
        QQmlEngine engine;
        engine.addImportPath(QString::fromUtf8(QWK_TEST_QML_IMPORT_PATH));
        QWK::registerTypes(&engine);
        QStringList warnings;
        connect(&engine, &QQmlEngine::warnings, this, [&](const QList<QQmlError> &errors) {
            for (const auto &error : errors)
                warnings.append(error.toString());
        });
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick 2.15
            import QtQuick.Window 2.15
            import QWindowKit 1.0
            Window {
                id: root
                property int changes: 0
                property bool initialized: false
                Item { id: original }
                Item { id: replacement }
                WindowAgent {
                    id: agent
                    onSystemButtonChanged: { root.changes += 1 }
                }
                Component.onCompleted: { initialized = agent.setup(root) }
                function exercise(role, valid) {
                    const roles = [WindowAgent.WindowIcon, WindowAgent.Help,
                        WindowAgent.Minimize, WindowAgent.Maximize, WindowAgent.Close]
                    if (WindowAgent.Unknown !== 0 || roles.join(",") !== "1,2,3,4,5")
                        return "enum metadata changed"
                    for (let i = 0; i < roles.length; ++i)
                        agent.setSystemButton(roles[i], original)
                    changes = 0
                    if (agent.systemButton(role) !== (valid ? original : null))
                        return "initial query"
                    agent.setSystemButton(role, replacement)
                    if (agent.systemButton(role) !== (valid ? replacement : null))
                        return "replacement query"
                    agent.setSystemButton(role, replacement)
                    if (changes !== (valid ? 1 : 0))
                        return "replacement signals"
                    for (let i = 0; i < roles.length; ++i) {
                        if (agent.systemButton(roles[i]) !== (roles[i] === role ? replacement : original))
                            return "another registration changed"
                    }
                    agent.setSystemButton(role, null)
                    agent.setSystemButton(role, null)
                    if (agent.systemButton(role) !== null || changes !== (valid ? 2 : 0))
                        return "unregistration"
                    for (let i = 0; i < roles.length; ++i) {
                        if (agent.systemButton(roles[i]) !== (roles[i] === role ? null : original))
                            return "another registration removed"
                    }
                    return ""
                }
            }
        )", QUrl("qrc:/systembuttons.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        QVERIFY(window->property("initialized").toBool());
        QVariant result;
        // Enter an actual QML function: it passes JavaScript numbers through Qt's
        // enum conversion to the production QuickWindowAgent invokable methods.
        QVERIFY(QMetaObject::invokeMethod(window.get(), "exercise", Q_RETURN_ARG(QVariant, result),
            Q_ARG(QVariant, QVariant(value)), Q_ARG(QVariant, QVariant(valid))));
        QVERIFY(result.isValid());
        QCOMPARE(result.toString(), QString{});
        QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join('\n')));
    }
};

int main(int argc, char **argv) {
    ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    QGuiApplication app(argc, argv);
    SystemButtonsTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_systembuttons.moc"
