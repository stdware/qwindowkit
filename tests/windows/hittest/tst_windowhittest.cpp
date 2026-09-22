// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <cstdio>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtCore/qt_windows.h>
#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <QtTest/QTest>

#include <QWKCore/qwkconfig.h>
#include <QWKCore/private/windowagentbase_p.h>
#ifdef TEST_WIDGETS
#  include <QtWidgets/QApplication>
#  include <QWKWidgets/widgetwindowagent.h>
#endif
#ifdef TEST_QUICK
#  include <QWKQuick/quickwindowagent.h>
#endif

namespace {
    // Observe the real factory result without replacing the production context.
    template <class Agent> class InspectableAgent : public Agent {
    public:
        QString contextKey() const { return this->d_ptr->context->key(); }
    };

    void reportReady(QWindow *window, const QString &context, bool borders) {
        const QJsonObject info{
            {QStringLiteral("hwnd"), QString::number(quintptr(window->winId()))},
            {QStringLiteral("context"), context},
            {QStringLiteral("borders"), borders},
            {QStringLiteral("dpr"), window->devicePixelRatio()},
            {QStringLiteral("fixedWidth"), window->minimumWidth() == window->maximumWidth()},
            {QStringLiteral("fixedHeight"), window->minimumHeight() == window->maximumHeight()}};
        const auto json = QJsonDocument(info).toJson(QJsonDocument::Compact);
        std::printf("%s\n", json.constData());
        std::fflush(stdout);
    }

    int runFixture(const QString &kind, const QString &constraint) {
        const bool fixedWidth = constraint == QStringLiteral("width") || constraint == QStringLiteral("both");
        const bool fixedHeight = constraint == QStringLiteral("height") || constraint == QStringLiteral("both");
        // Also clean up if the test driver is interrupted before its normal cleanup.
        QTimer::singleShot(20000, qApp, [] { QCoreApplication::exit(3); });
#ifdef TEST_WIDGETS
        if (kind == QStringLiteral("widgets")) {
            QWidget window;
            window.setGeometry(200, 200, 640, 400);
            if (fixedWidth)
                window.setFixedWidth(640);
            if (fixedHeight)
                window.setFixedHeight(400);
            QWidget title(&window);
            title.setGeometry(40, 20, 500, 60);
            QWidget excluded(&title);
            excluded.setGeometry(160, 10, 80, 40);
            InspectableAgent<QWK::WidgetWindowAgent> agent;
            if (!agent.setup(&window))
                return 2;
            agent.setTitleBar(&title);
            agent.setHitTestVisible(&excluded);
            window.show();
            ::ShowWindow(reinterpret_cast<HWND>(window.winId()), SW_SHOWNOACTIVATE);
            if (!QTest::qWaitForWindowExposed(window.windowHandle()))
                return 2;
            reportReady(window.windowHandle(), agent.contextKey(),
                        agent.windowAttribute(QStringLiteral("windows-system-border-enabled")).toBool());
            return QCoreApplication::exec();
        }
#endif
#ifdef TEST_QUICK
        if (kind == QStringLiteral("quick")) {
            QQuickWindow window;
            window.setGeometry(200, 200, 640, 400);
            if (fixedWidth) {
                window.setMinimumWidth(640);
                window.setMaximumWidth(640);
            }
            if (fixedHeight) {
                window.setMinimumHeight(400);
                window.setMaximumHeight(400);
            }
            QQuickItem title(window.contentItem());
            title.setPosition(QPointF(40, 20));
            title.setSize(QSizeF(500, 60));
            QQuickItem excluded(&title);
            excluded.setPosition(QPointF(160, 10));
            excluded.setSize(QSizeF(80, 40));
            InspectableAgent<QWK::QuickWindowAgent> agent;
            if (!agent.setup(&window))
                return 2;
            agent.setTitleBar(&title);
            agent.setHitTestVisible(&excluded);
            window.show();
            ::ShowWindow(reinterpret_cast<HWND>(window.winId()), SW_SHOWNOACTIVATE);
            if (!QTest::qWaitForWindowExposed(&window))
                return 2;
            reportReady(&window, agent.contextKey(),
                        agent.windowAttribute(QStringLiteral("windows-system-border-enabled")).toBool());
            return QCoreApplication::exec();
        }
#endif
        return 2;
    }
}

class WindowHitTest : public QObject {
    Q_OBJECT

private:
    QProcess fixture;
    HWND hwnd = nullptr;
    qreal dpr = 1;
    bool borders = false;

    void addRows(bool onlySingleAxis) {
        QTest::addColumn<QString>("kind");
        QTest::addColumn<QString>("constraint");
        QStringList kinds;
#ifdef TEST_WIDGETS
        kinds << QStringLiteral("widgets");
#endif
#ifdef TEST_QUICK
        kinds << QStringLiteral("quick");
#endif
        QStringList constraints{QStringLiteral("width"), QStringLiteral("height")};
        if (!onlySingleAxis)
            constraints << QStringLiteral("both") << QStringLiteral("none");
        for (const auto &kind : kinds) {
            for (const auto &constraint : constraints) {
                const auto name = (kind + QLatin1Char('-') + constraint).toLatin1();
                QTest::newRow(name.constData()) << kind << constraint;
            }
        }
    }

    void startFixture(const QString &kind, const QString &constraint) {
        fixture.start(QCoreApplication::applicationFilePath(),
                      {QStringLiteral("--fixture"), kind, constraint});
        QVERIFY2(fixture.waitForStarted(5000), qPrintable(fixture.errorString()));
        QTRY_VERIFY_WITH_TIMEOUT(fixture.canReadLine(), 7000);
        const auto info = QJsonDocument::fromJson(fixture.readLine()).object();
        QCOMPARE(info.value(QStringLiteral("context")).toString(), QStringLiteral("win32"));
        QCOMPARE(info.value(QStringLiteral("fixedWidth")).toBool(),
                 constraint == QStringLiteral("width") || constraint == QStringLiteral("both"));
        QCOMPARE(info.value(QStringLiteral("fixedHeight")).toBool(),
                 constraint == QStringLiteral("height") || constraint == QStringLiteral("both"));
        borders = info.value(QStringLiteral("borders")).toBool();
        QCOMPARE(borders, bool(QWINDOWKIT_CONFIG(ENABLE_WINDOWS_SYSTEM_BORDERS)));
        dpr = info.value(QStringLiteral("dpr")).toDouble();
        QVERIFY(dpr > 0);
        hwnd = reinterpret_cast<HWND>(info.value(QStringLiteral("hwnd")).toString().toULongLong());
        QVERIFY(::IsWindow(hwnd));
        QVERIFY(::IsWindowVisible(hwnd));
        DWORD pid = 0;
        QVERIFY(::GetWindowThreadProcessId(hwnd, &pid));
        QCOMPARE(quint64(pid), quint64(fixture.processId()));
    }

    void checkHit(POINT screenPoint, LRESULT expected, const char *location) {
        QVERIFY2(screenPoint.x >= -32768 && screenPoint.x <= 32767 &&
                     screenPoint.y >= -32768 && screenPoint.y <= 32767,
                 "Screen coordinates must fit WM_NCHITTEST's signed 16-bit fields");
        DWORD_PTR result = 0;
        QVERIFY2(::SendMessageTimeoutW(hwnd, WM_NCHITTEST, 0,
                                      MAKELPARAM(screenPoint.x, screenPoint.y),
                                      SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT, 1000, &result),
                 "WM_NCHITTEST failed or timed out");
        const auto message = QStringLiteral("%1: expected %2, got %3")
                                 .arg(QString::fromLatin1(location)).arg(expected).arg(LRESULT(result));
        QVERIFY2(LRESULT(result) == expected, qPrintable(message));
    }

    void checkClientPoint(int x, int y, LRESULT expected, const char *location) {
        POINT point{qRound(x * dpr), qRound(y * dpr)};
        QVERIFY(::ClientToScreen(hwnd, &point));
        checkHit(point, expected, location);
    }

private Q_SLOTS:
    void initTestCase() {
        QCOMPARE(QGuiApplication::platformName(), QStringLiteral("windows"));
        qInfo() << "Qt" << qVersion() << "system borders"
                << bool(QWINDOWKIT_CONFIG(ENABLE_WINDOWS_SYSTEM_BORDERS));
    }

    void cleanup() {
        bool closedNormally = true;
        bool stopped = true;
        if (fixture.state() != QProcess::NotRunning) {
            if (hwnd)
                ::PostMessageW(hwnd, WM_CLOSE, 0, 0);
            if (!fixture.waitForFinished(3000)) {
                closedNormally = false;
                fixture.kill();
                stopped = fixture.waitForFinished(3000);
            }
        }
        hwnd = nullptr;
        const auto errors = fixture.readAllStandardError();
        if (!errors.isEmpty())
            qInfo().noquote() << errors;
        QVERIFY(stopped);
        QVERIFY2(closedNormally, "Fixture did not close normally");
        QCOMPARE(fixture.exitStatus(), QProcess::NormalExit);
        QCOMPARE(fixture.exitCode(), 0);
    }

    void titleBarAndClient_data() { addRows(false); }
    void titleBarAndClient() {
        QFETCH(QString, kind);
        QFETCH(QString, constraint);
        startFixture(kind, constraint);
        if (QTest::currentTestFailed())
            return;
        // These points are defined by the fixture layout, not production hit-test helpers.
        checkClientPoint(100, 50, HTCAPTION, "title bar");
        checkClientPoint(230, 50, HTCLIENT, "excluded control");
        checkClientPoint(300, 200, HTCLIENT, "content");
    }

    void resizeEdges_data() { addRows(true); }
    void resizeEdges() {
        QFETCH(QString, kind);
        QFETCH(QString, constraint);
        startFixture(kind, constraint);
        if (QTest::currentTestFailed())
            return;
        RECT frame{};
        RECT client{};
        POINT origin{0, 0};
        QVERIFY(::GetWindowRect(hwnd, &frame));
        QVERIFY(::GetClientRect(hwnd, &client));
        QVERIFY(::ClientToScreen(hwnd, &origin));
        const LONG left = frame.left + 1;
        const LONG right = frame.right - 1;
        const LONG top = origin.y + 1;
        const LONG bottom = frame.bottom - 1;
        const LONG midX = origin.x + client.right / 2;
        const LONG midY = origin.y + client.bottom / 2;
        const bool fixedWidth = constraint == QStringLiteral("width");
        const auto fixedBorder = borders ? HTBORDER : HTCLIENT;
        checkHit({left, midY}, fixedWidth ? fixedBorder : HTLEFT, "left");
        checkHit({right, midY}, fixedWidth ? fixedBorder : HTRIGHT, "right");
        checkHit({midX, top}, fixedWidth ? HTTOP : HTCLIENT, "top");
        checkHit({midX, bottom}, fixedWidth ? HTBOTTOM : fixedBorder, "bottom");
        checkHit({left, top}, fixedWidth ? HTTOP : HTLEFT, "top left");
        checkHit({right, top}, fixedWidth ? HTTOP : HTRIGHT, "top right");
        checkHit({left, bottom}, fixedWidth ? HTBOTTOM : HTLEFT, "bottom left");
        checkHit({right, bottom}, fixedWidth ? HTBOTTOM : HTRIGHT, "bottom right");
    }
};

int main(int argc, char **argv) {
#ifdef TEST_WIDGETS
    QApplication app(argc, argv);
#else
    QGuiApplication app(argc, argv);
#endif
    const auto args = app.arguments();
    if (args.size() == 4 && args.at(1) == QStringLiteral("--fixture"))
        return runFixture(args.at(2), args.at(3));
    WindowHitTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_windowhittest.moc"
