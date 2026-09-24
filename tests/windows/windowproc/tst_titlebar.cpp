// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <memory>
#include <type_traits>
#include <QtTest/QTest>
#include <QtTest/QSignalSpy>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtCore/qt_windows.h>
#include <QWKCore/private/windowagentbase_p.h>
#include <QWKWidgets/widgetwindowagent.h>
#ifdef TEST_QUICK
#  include <QWKQuick/quickwindowagent.h>
#endif

using Button = QWK::WindowAgentBase;

namespace {
    template <class Agent> class InspectableAgent : public Agent {
    public:
        QString contextKey() const { return this->d_ptr->context->key(); }
    };
    auto titleSignal(const QWK::WidgetWindowAgent &) { return &QWK::WidgetWindowAgent::titleBarChanged; }
    QWidget *root(QWidget &window) { return &window; }
    QWindow *handle(QWidget &window) { return window.windowHandle(); }
    void place(QWidget *item, QWidget *parent, const QRect &rect) {
        item->setParent(parent);
        item->setGeometry(rect);
        item->show();
    }
#ifdef TEST_QUICK
    auto titleSignal(const QWK::QuickWindowAgent &) { return &QWK::QuickWindowAgent::titleBarWidgetChanged; }
    QQuickItem *root(QQuickWindow &window) { return window.contentItem(); }
    QWindow *handle(QQuickWindow &window) { return &window; }
    void place(QQuickItem *item, QQuickItem *parent, const QRect &rect) {
        item->setParent(parent); // QObject ownership must also leave the old title.
        item->setParentItem(parent);
        item->setPosition(rect.topLeft());
        item->setSize(rect.size());
        item->setVisible(true);
    }
#endif

    void checkHit(QWindow *window, const QPoint &position, LRESULT expected) {
        const auto dpr = window->devicePixelRatio();
        const auto hwnd = reinterpret_cast<HWND>(window->winId());
        // Use client origin plus scaled client coordinates, avoiding mixed-DPI global mapping.
        POINT native{qRound(position.x() * dpr), qRound(position.y() * dpr)};
        QVERIFY(::ClientToScreen(hwnd, &native));
        DWORD_PTR actual = 0;
        QVERIFY(::SendMessageTimeoutW(hwnd, WM_NCHITTEST, 0, MAKELPARAM(native.x, native.y),
            SMTO_ABORTIFHUNG | SMTO_BLOCK, 1000, &actual));
        QCOMPARE(LRESULT(actual), expected);
    }

    template <class Window, class Agent>
    void lifecycle(const QString &scenario) {
        Window window;
        window.setGeometry(200, 200, 640, 400);
        InspectableAgent<Agent> agent;
        QVERIFY(agent.setup(&window));
        QCOMPARE(agent.contextKey(), QString("win32"));
        using Item = std::remove_pointer_t<decltype(agent.titleBar())>;
        auto title = std::make_unique<Item>();
        auto button = std::make_unique<Item>();
        auto excluded = std::make_unique<Item>();
        place(title.get(), root(window), QRect(40, 20, 500, 60));
        place(button.get(), title.get(), QRect(20, 10, 80, 40));
        place(excluded.get(), title.get(), QRect(160, 10, 80, 40));
        QSignalSpy titles(&agent, titleSignal(agent));
        QSignalSpy buttons(&agent, &Agent::systemButtonChanged);
        agent.setTitleBar(title.get());
        agent.setSystemButton(Button::Close, button.get());
        agent.setHitTestVisible(excluded.get());
        window.show();
        ::ShowWindow(reinterpret_cast<HWND>(handle(window)->winId()), SW_SHOWNOACTIVATE);
        QVERIFY(QTest::qWaitForWindowExposed(handle(window), 3000));
        QVERIFY(::IsWindow(reinterpret_cast<HWND>(handle(window)->winId())));
        checkHit(handle(window), QPoint(100, 50), HTCLOSE);
        checkHit(handle(window), QPoint(230, 50), HTCLIENT);
        checkHit(handle(window), QPoint(350, 50), HTCAPTION);
        if (QTest::currentTestFailed()) return;

        // Move the registered objects out of the title while keeping their scene positions.
        place(button.get(), root(window), QRect(60, 30, 80, 40));
        place(excluded.get(), root(window), QRect(200, 30, 80, 40));
        QPointer<Item> survivingButton(button.get()), survivingExcluded(excluded.get());
        if (scenario == "objects-first") {
            button.reset();
            excluded.reset();
            QVERIFY(!agent.systemButton(Button::Close));
        }
        if (scenario != "live-title") {
            title.reset();
            QVERIFY(!agent.titleBar());
        }
        QCOMPARE(titles.count(), 1);
        QCOMPARE(buttons.count(), 1);

        Item replacement, third;
        place(&replacement, root(window), QRect(40, 20, 500, 60));
        bool cleanAtSignal = false;
        QObject::connect(&agent, titleSignal(agent), &agent, [&](Item *item) {
            cleanAtSignal = agent.titleBar() == item && !agent.systemButton(Button::Close) &&
                (!survivingExcluded || !agent.isHitTestVisible(survivingExcluded.data()));
        });
        agent.setTitleBar(&replacement);
        checkHit(handle(window), QPoint(100, 50), HTCAPTION);
        checkHit(handle(window), QPoint(230, 50), HTCAPTION);
        QCOMPARE(titles.count(), 2);
        QCOMPARE(buttons.count(), 1); // Clearing title registrations does not emit button setters.
        QVERIFY(cleanAtSignal);
        QVERIFY(!agent.systemButton(Button::Close));
        if (scenario != "objects-first") {
            QVERIFY(survivingButton);
            QVERIFY(survivingExcluded);
            QVERIFY(!agent.isHitTestVisible(excluded.get()));
        }
        if (QTest::currentTestFailed()) return;

        // Legal registrations on the new title survive a duplicate setter.
        if (!button) button = std::make_unique<Item>();
        if (!excluded) excluded = std::make_unique<Item>();
        place(button.get(), root(window), QRect(60, 30, 80, 40));
        place(excluded.get(), root(window), QRect(200, 30, 80, 40));
        agent.setSystemButton(Button::Close, button.get());
        agent.setHitTestVisible(excluded.get());
        agent.setTitleBar(&replacement);
        QCOMPARE(titles.count(), 2);
        QCOMPARE(buttons.count(), 2);
        QCOMPARE(agent.systemButton(Button::Close), button.get());
        QVERIFY(agent.isHitTestVisible(excluded.get()));
        checkHit(handle(window), QPoint(100, 50), HTCLOSE);
        checkHit(handle(window), QPoint(230, 50), HTCLIENT);
        if (QTest::currentTestFailed()) return;

        // A replaced title's later destruction must not disturb the current registration.
        title.reset();
        QCOMPARE(agent.systemButton(Button::Close), button.get());
        place(&third, root(window), QRect(40, 20, 500, 60));
        agent.setTitleBar(&third);
        QCOMPARE(titles.count(), 3);
        QVERIFY(!agent.systemButton(Button::Close));
        QVERIFY(!agent.isHitTestVisible(excluded.get()));
        checkHit(handle(window), QPoint(100, 50), HTCAPTION);
        checkHit(handle(window), QPoint(230, 50), HTCAPTION);
    }
}

class TitleBarTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void initTestCase() {
        qRegisterMetaType<Button::SystemButton>("SystemButton");
        qRegisterMetaType<Button::SystemButton>("QWK::WindowAgentBase::SystemButton");
        QCOMPARE(QGuiApplication::platformName(), QString("windows"));
    }
    void registrations_data() {
        QTest::addColumn<bool>("quick");
        QTest::addColumn<QString>("scenario");
        for (const QString scenario : {"destroyed-title", "live-title", "objects-first"}) {
            QTest::newRow(qPrintable("widgets-" + scenario)) << false << scenario;
#ifdef TEST_QUICK
            QTest::newRow(qPrintable("quick-" + scenario)) << true << scenario;
#endif
        }
    }
    void registrations() {
        QFETCH(bool, quick); QFETCH(QString, scenario);
#ifdef TEST_QUICK
        if (quick) {
            lifecycle<QQuickWindow, QWK::QuickWindowAgent>(scenario);
            return;
        }
#endif
        Q_UNUSED(quick)
        lifecycle<QWidget, QWK::WidgetWindowAgent>(scenario);
    }
};

QTEST_MAIN(TitleBarTest)
#include "tst_titlebar.moc"
