// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <functional>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryDir>
#include <QtCore/qscopeguard.h>
#include <QtGui/QExposeEvent>
#include <QtTest/QTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QWKCore/private/windowagentbase_p.h>
#include <QWKWidgets/widgetwindowagent.h>

namespace {
    bool childProcess = false;
    int draws = 0;

    class Context : public QWK::AbstractWindowContext {
    public:
        QVariant windowAttribute(const QString &key) const override {
            if (key == QStringLiteral("win10-border-needed"))
                return true;
            return AbstractWindowContext::windowAttribute(key);
        }
        void drawWindows10Border() override { ++draws; }
    protected:
        void winIdChanged(WId, WId) override {}
        bool windowAttributeChanged(const QString &, const QVariant &) override {
            return true;
        }
    };

    QPointer<Context> context;
    QWK::AbstractWindowContext *createContext() {
        auto result = new Context;
        context = result;
        return result;
    }

    class Widget : public QWidget {
    public:
        std::function<void()> updateAction;
        int updates = 0;
        bool event(QEvent *event) override {
            if (event->type() == QEvent::UpdateRequest) {
                ++updates;
                const auto action = updateAction;
                if (action)
                    action();
                return true;
            }
            return QWidget::event(event);
        }
    };

    class Filter : public QObject {
    public:
        QEvent::Type type = QEvent::User;
        std::function<bool()> action;
        int calls = 0;
        bool eventFilter(QObject *, QEvent *event) override {
            if (event->type() != type || !action)
                return false;
            ++calls;
            const auto callback = action;
            return callback();
        }
    };

    class Tail : public QWK::SharedEventFilter {
    public:
        int calls = 0;
        bool sharedEventFilter(QObject *, QEvent *event) override {
            if (event->type() == QEvent::Expose)
                ++calls;
            return false;
        }
    };
}

class WidgetBorderTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void forwarding_data() {
        QTest::addColumn<bool>("expose");
        QTest::addColumn<QString>("action");
        for (bool expose : {false, true}) {
            for (const QString &action : {QString("normal"), QString("consume"),
                                         QString("delete-agent"), QString("delete-window"),
                                         QString("delete-window-keep-agent"), QString("nested-delete")}) {
                const auto tag = ((expose ? "expose-" : "update-") + action).toLatin1();
                QTest::newRow(tag.constData()) << expose << action;
            }
        }
        QTest::newRow("update-delete-agent-in-event") << false << QString("delete-agent-in-event");
    }

    void forwarding() {
        if (!childProcess) {
            QTemporaryDir reports;
            QVERIFY(reports.isValid());
            const auto reportPath = reports.filePath("child.txt");
            QProcess child;
            child.setProcessChannelMode(QProcess::MergedChannels);
            child.start(QCoreApplication::applicationFilePath(),
                        {"--child", QString("forwarding:%1").arg(QTest::currentDataTag()),
                         "-o", reportPath + ",txt"});
            QVERIFY(child.waitForStarted(2000));
            const bool finished = child.waitForFinished(5000);
            if (!finished) {
                child.kill();
                child.waitForFinished(1000);
            }
            QFile report(reportPath);
            const auto output = child.readAll() + (report.open(QIODevice::ReadOnly)
                ? report.readAll() : report.errorString().toUtf8());
            QVERIFY2(finished, output.constData());
            QVERIFY2(child.exitStatus() == QProcess::NormalExit && child.exitCode() == 0,
                     output.constData());
            return;
        }

        QFETCH(bool, expose);
        QFETCH(QString, action);
        QCOMPARE(QGuiApplication::platformName(), QStringLiteral("offscreen"));
        const auto previousFactory = QWK::WindowAgentBasePrivate::windowContextFactoryMethod;
        QWK::WindowAgentBasePrivate::windowContextFactoryMethod = createContext;
        const auto restoreFactory = qScopeGuard([&] {
            QWK::WindowAgentBasePrivate::windowContextFactoryMethod = previousFactory;
        });
        QPointer<Widget> widget = new Widget;
        QPointer<QWK::WidgetWindowAgent> agent;
        const auto cleanup = qScopeGuard([&] { delete agent.data(); delete widget.data(); });
        widget->resize(240, 120);
        widget->winId();
        auto window = widget->windowHandle();
        QVERIFY(window);
        if (expose) {
            widget->show(); // Offscreen component only; no native desktop or GDI rendering.
            QCoreApplication::processEvents();
            QVERIFY(window->isExposed());
        }

        Filter remaining;
        remaining.type = expose ? QEvent::Expose : QEvent::UpdateRequest;
        QObject *receiver = expose ? static_cast<QObject *>(window) : widget.data();
        receiver->installEventFilter(&remaining); // Runs after the production border/context.
        agent = new QWK::WidgetWindowAgent(action == "delete-window-keep-agent" ? nullptr : widget.data());
        QVERIFY(agent->setup(widget));
        QVERIFY(context);
        Tail tail;
        context->installSharedEventFilter(&tail);
        widget->updates = 0;
        draws = 0;
        bool nested = false;
        auto send = [&] {
            if (expose) {
                QExposeEvent event(QRegion(0, 0, 240, 120));
                return QCoreApplication::sendEvent(window, &event);
            }
            QEvent event(QEvent::UpdateRequest);
            return QCoreApplication::sendEvent(widget, &event);
        };
        remaining.action = [&] {
            if (action == "delete-agent") {
                delete agent.data();
                return false; // A surviving receiver must still receive its event once.
            }
            if (action.startsWith("delete-window")) {
                delete widget.data();
                return true;
            }
            if (action == "nested-delete") {
                if (!nested) {
                    nested = true;
                    send();
                } else {
                    delete widget.data();
                }
                return true;
            }
            return action == "consume";
        };
        if (action == "delete-agent-in-event")
            widget->updateAction = [&] { delete agent.data(); };

        QVERIFY(send());
        QCOMPARE(remaining.calls, action == "nested-delete" ? 2 : 1);
        QCOMPARE(draws, action == "normal" || action == "consume" ? 1 : 0);
        QCOMPARE(tail.calls, 0); // The forwarded expose must never continue down the shared list.
        if (action.contains("window") || action == "nested-delete")
            QVERIFY(!widget);
        else if (!expose)
            QCOMPARE(widget->updates, action == "consume" ? 0 : 1);
        if (action == "delete-window-keep-agent") {
            QVERIFY(agent);
            QVERIFY(context);
            QVERIFY(!context->window());
        } else if (action != "normal" && action != "consume") {
            QVERIFY(!agent);
            QVERIFY(!context);
        }
    }
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    auto arguments = app.arguments();
    childProcess = arguments.removeAll("--child") > 0;
    WidgetBorderTest test;
    return QTest::qExec(&test, arguments);
}

#include "tst_widgetborder.moc"
