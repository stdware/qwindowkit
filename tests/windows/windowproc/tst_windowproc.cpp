// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <functional>
#include <memory>
#include <QtCore/QAbstractEventDispatcher>
#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryDir>
#include <QtCore/qt_windows.h>
#include <QtTest/QTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QWKCore/private/windowagentbase_p.h>
#include <QWKWidgets/widgetwindowagent.h>

namespace {
    bool childProcess = false;
    constexpr UINT probeMessage = WM_APP + 137;
    QString calls;
    struct Chain {
        WNDPROC original = nullptr;
        WNDPROC after = nullptr;
        int destroyed = 0;
        std::function<void()> action;
    } chainState[2];

    template <int Index>
    LRESULT CALLBACK before(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
        auto &chain = chainState[Index];
        if (message == probeMessage) {
            calls += Index == 0 ? 'A' : 'B';
            const auto action = chain.action;
            if (action)
                action();
            return 100 + Index + LRESULT(wParam) + lParam;
        }
        if (message == WM_NCDESTROY)
            ++chain.destroyed;
        return ::CallWindowProcW(chain.original, hwnd, message, wParam, lParam);
    }

    template <int Index>
    LRESULT CALLBACK after(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
        if (message == probeMessage)
            calls += 'L';
        return ::CallWindowProcW(chainState[Index].after, hwnd, message, wParam, lParam);
    }

    class Agent : public QWK::WidgetWindowAgent {
    public:
        QString contextKey() const { return d_ptr->context->key(); }
    };

    class ReenterFilter : public QAbstractNativeEventFilter {
    public:
        std::function<void(MSG *)> action;
        bool nativeEventFilter(const QByteArray &, void *message,
                               QT_NATIVE_EVENT_RESULT_TYPE *) override {
            auto msg = static_cast<MSG *>(message);
            if (msg && msg->message == WM_NCCALCSIZE && action)
                action(msg);
            return false;
        }
    };

    struct Calculation {
        RECT rect{200, 200, 840, 600};
        LRESULT result = 0;
        QRect geometry() const { return QRect(rect.left, rect.top,
                                             rect.right - rect.left, rect.bottom - rect.top); }
    };

    Calculation calculate(HWND hwnd) {
        Calculation value;
        value.result = ::SendMessageW(hwnd, WM_NCCALCSIZE, FALSE,
                                     reinterpret_cast<LPARAM>(&value.rect));
        return value;
    }

    WNDPROC procedure(HWND hwnd) {
        return reinterpret_cast<WNDPROC>(::GetWindowLongPtrW(hwnd, GWLP_WNDPROC));
    }

    WNDPROC replace(HWND hwnd, WNDPROC proc) {
        return reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(
            hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(proc)));
    }
}

class WindowProcTest : public QObject {
    Q_OBJECT
private:
    void runChild() {
        QTemporaryDir reports;
        QVERIFY(reports.isValid());
        const auto path = reports.filePath("child.txt");
        QProcess child;
        child.setProcessChannelMode(QProcess::MergedChannels);
        child.start(QCoreApplication::applicationFilePath(),
                    {"--child", QString("%1:%2").arg(QTest::currentTestFunction(), QTest::currentDataTag()),
                     "-o", path + ",txt"});
        QVERIFY(child.waitForStarted(2000));
        const bool finished = child.waitForFinished(5000);
        if (!finished) {
            child.kill();
            child.waitForFinished(1000);
        }
        QFile report(path);
        const auto output = child.readAll() + (report.open(QIODevice::ReadOnly)
            ? report.readAll() : report.errorString().toUtf8());
        QVERIFY2(finished, output.constData());
        QVERIFY2(child.exitStatus() == QProcess::NormalExit && child.exitCode() == 0,
                 output.constData());
    }

private Q_SLOTS:
    void chains_data() {
        QTest::addColumn<QString>("scenario");
        for (const auto name : {"distinct", "detach", "later-subclass", "destroy-detached",
                                "detach-in-callback"})
            QTest::newRow(name) << QString::fromLatin1(name);
    }

    void chains() {
        if (!childProcess) {
            runChild();
            return;
        }
        QFETCH(QString, scenario);
        QCOMPARE(QGuiApplication::platformName(), QStringLiteral("windows"));
        auto first = std::make_unique<QWidget>();
        auto second = std::make_unique<QWidget>();
        const auto hwndA = reinterpret_cast<HWND>(first->winId());
        const auto hwndB = reinterpret_cast<HWND>(second->winId());
        chainState[0].original = replace(hwndA, before<0>);
        chainState[1].original = replace(hwndB, before<1>);
        QVERIFY(chainState[0].original && chainState[1].original);
        auto agentA = std::make_unique<Agent>();
        auto agentB = std::make_unique<Agent>();
        QVERIFY(agentA->setup(first.get()));
        QVERIFY(agentB->setup(second.get()));
        QCOMPARE(agentA->contextKey(), QStringLiteral("win32"));
        QCOMPARE(agentB->contextKey(), QStringLiteral("win32"));
        QCOMPARE(reinterpret_cast<HWND>(first->winId()), hwndA);
        QCOMPARE(reinterpret_cast<HWND>(second->winId()), hwndB);

        if (scenario == "detach") {
            agentA.reset();
            agentB.reset();
            QCOMPARE(procedure(hwndA), before<0>);
            QCOMPARE(procedure(hwndB), before<1>);
        } else if (scenario == "later-subclass" || scenario == "destroy-detached") {
            chainState[0].after = replace(hwndA, after<0>);
            chainState[1].after = replace(hwndB, after<1>);
            QVERIFY(chainState[0].after && chainState[1].after);
            agentA.reset();
            agentB.reset();
            QCOMPARE(procedure(hwndA), after<0>);
            QCOMPARE(procedure(hwndB), after<1>);
            if (scenario == "later-subclass") {
                // Reuse the retained hook underneath the later subclass, without a cycle.
                agentA = std::make_unique<Agent>();
                agentB = std::make_unique<Agent>();
                QVERIFY(agentA->setup(first.get()));
                QVERIFY(agentB->setup(second.get()));
                QCOMPARE(procedure(hwndA), after<0>);
                QCOMPARE(procedure(hwndB), after<1>);
            }
        } else if (scenario == "detach-in-callback") {
            chainState[0].action = [&] { agentA.reset(); };
            chainState[1].action = [&] { agentB.reset(); };
        }

        calls.clear();
        QCOMPARE(::SendMessageW(hwndA, probeMessage, 7, 11), LRESULT(118));
        QCOMPARE(::SendMessageW(hwndB, probeMessage, 13, 17), LRESULT(131));
        const bool later = scenario == "later-subclass" || scenario == "destroy-detached";
        QCOMPARE(calls, later ? QStringLiteral("LALB") : QStringLiteral("AB"));

        // Teardown must reach both independent predecessors, including inactive hooks.
        first.reset();
        second.reset();
        QVERIFY(!::IsWindow(hwndA));
        QVERIFY(!::IsWindow(hwndB));
        QCOMPARE(chainState[0].destroyed, 1);
        QCOMPARE(chainState[1].destroyed, 1);
        agentA.reset();
        agentB.reset();

        // A fresh native registration after teardown must never forward to either old chain.
        QWidget fresh;
        const auto freshHwnd = reinterpret_cast<HWND>(fresh.winId());
        const auto freshProc = procedure(freshHwnd);
        Agent freshAgent;
        QVERIFY(freshAgent.setup(&fresh));
        calls.clear();
        QCOMPARE(::SendMessageW(freshHwnd, probeMessage, 0, 0),
                 ::CallWindowProcW(freshProc, freshHwnd, probeMessage, 0, 0));
        QVERIFY(calls.isEmpty());
    }

    void reentrancy_data() {
        QTest::addColumn<QString>("scenario");
        for (const auto name : {"same-window", "other-window", "unmanaged-window",
                               "inactive-window", "two-level", "delete-outer-agent",
                               "replace-outer-agent", "unrelated-message"})
            QTest::newRow(name) << QString::fromLatin1(name);
    }

    void reentrancy() {
        if (!childProcess) {
            runChild();
            return;
        }
        QFETCH(QString, scenario);
        QCOMPARE(QGuiApplication::platformName(), QStringLiteral("windows"));
        QWidget first, second, plain;
        const auto hwndA = reinterpret_cast<HWND>(first.winId());
        const auto hwndB = reinterpret_cast<HWND>(second.winId());
        const auto hwndPlain = reinterpret_cast<HWND>(plain.winId());
        auto agentA = std::make_unique<Agent>();
        Agent agentB;
        QVERIFY(agentA->setup(&first));
        QVERIFY(agentB.setup(&second));
        QCOMPARE(agentA->contextKey(), QStringLiteral("win32"));
        QCOMPARE(agentB.contextKey(), QStringLiteral("win32"));
        if (scenario == "inactive-window") {
            Agent temporary;
            QVERIFY(temporary.setup(&plain));
            chainState[0].after = replace(hwndPlain, after<0>);
            QVERIFY(chainState[0].after);
        }
        const auto baselineA = calculate(hwndA);
        const auto baselineB = calculate(hwndB);
        const auto baselinePlain = calculate(hwndPlain);
        int depth = 0;
        int visits = 0;
        int deepest = 0;
        bool enabled = true;
        bool replacementSetup = false;
        bool foreignConsumed = false;
        Calculation inner, innermost, foreign;
        QT_NATIVE_EVENT_RESULT_TYPE foreignResult = 12345;
        ReenterFilter filter;
        filter.action = [&](MSG *msg) {
            if (!enabled)
                return;
            ++visits;
            ++depth;
            deepest = qMax(deepest, depth);
            if (depth == 1) {
                if (scenario == "unrelated-message") {
                    // Explicit dispatcher probe: same HWND/type, different message payload.
                    MSG different = *msg;
                    different.lParam = reinterpret_cast<LPARAM>(&foreign.rect);
                    enabled = false;
                    foreignConsumed = QAbstractEventDispatcher::instance()->filterNativeEvent(
                        "windows_generic_MSG", &different, &foreignResult);
                    enabled = true;
                } else {
                    const auto target = scenario == "same-window" ? hwndA
                        : (scenario == "unmanaged-window" || scenario == "inactive-window")
                            ? hwndPlain : hwndB;
                    inner = calculate(target);
                }
            } else if (depth == 2) {
                if (scenario == "two-level") {
                    innermost = calculate(hwndA);
                } else if (scenario == "delete-outer-agent" || scenario == "replace-outer-agent") {
                    enabled = false;
                    agentA.reset();
                    if (scenario == "replace-outer-agent") {
                        agentA = std::make_unique<Agent>();
                        replacementSetup = agentA->setup(&first);
                    }
                    enabled = true;
                }
            }
            --depth;
        };
        QCoreApplication::instance()->installNativeEventFilter(&filter);
        const auto outer = calculate(hwndA);
        enabled = false;
        QCOMPARE(depth, 0);
        if (scenario == "unrelated-message") {
            QCOMPARE(visits, 1);
            QVERIFY(!foreignConsumed);
            QCOMPARE(foreign.geometry(), Calculation{}.geometry());
            QCOMPARE(foreignResult, QT_NATIVE_EVENT_RESULT_TYPE(12345));
        } else {
            QCOMPARE(visits, scenario == "two-level" ? 3 : 2);
            QCOMPARE(deepest, scenario == "two-level" ? 3 : 2);
            const auto expected = scenario == "same-window" ? baselineA
                : (scenario == "unmanaged-window" || scenario == "inactive-window")
                    ? baselinePlain : baselineB;
            QCOMPARE(inner.geometry(), expected.geometry());
            QCOMPARE(inner.result, expected.result);
        }
        if (scenario == "two-level") {
            QCOMPARE(innermost.geometry(), baselineA.geometry());
            QCOMPARE(innermost.result, baselineA.result);
        }
        if (scenario == "delete-outer-agent" || scenario == "replace-outer-agent") {
            if (scenario == "replace-outer-agent")
                QVERIFY(replacementSetup);
            else
                QVERIFY(!agentA);
            agentA.reset();
            // The old frame must not use either a deleted or replacement context.
            const auto detached = calculate(hwndA);
            QCOMPARE(outer.geometry(), detached.geometry());
            QCOMPARE(outer.result, detached.result);
            QVERIFY(outer.geometry() != baselineA.geometry());
        } else {
            QCOMPARE(outer.geometry(), baselineA.geometry());
            QCOMPARE(outer.result, baselineA.result);
        }
        QCoreApplication::instance()->removeNativeEventFilter(&filter);
        // No stale frame may remain after the outermost dispatch.
        QCOMPARE(calculate(hwndPlain).geometry(), baselinePlain.geometry());
    }
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    auto arguments = app.arguments();
    childProcess = arguments.removeAll("--child") > 0;
    WindowProcTest test;
    return QTest::qExec(&test, arguments);
}

#include "tst_windowproc.moc"
