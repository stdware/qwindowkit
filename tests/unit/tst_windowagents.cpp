// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <memory>
#include <type_traits>

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>
#include <QWKCore/private/windowagentbase_p.h>
#include "systembuttoncases.h"
#ifdef Q_OS_WIN
#  include <QtCore/qt_windows.h>
#endif
#ifdef TEST_WIDGETS
#  include <QtWidgets/QApplication>
#  include <QWKWidgets/widgetwindowagent.h>
#endif
#ifdef TEST_QUICK
#  include <QtGui/QGuiApplication>
#  include <QWKQuick/quickwindowagent.h>
#endif

namespace {
    using Button = QWK::WindowAgentBase;

    // Exercise real public agents, delegates and base context state without installing any
    // native hooks, showing a window, or asking the OS to draw a border.
    class UnitContext : public QWK::AbstractWindowContext {
    protected:
        void winIdChanged(WId, WId) override {}
    };

    class FactoryGuard {
    public:
        FactoryGuard() : previous(QWK::WindowAgentBasePrivate::windowContextFactoryMethod) {
            QWK::WindowAgentBasePrivate::windowContextFactoryMethod = []() -> QWK::AbstractWindowContext * {
                return new UnitContext;
            };
        }
        ~FactoryGuard() { QWK::WindowAgentBasePrivate::windowContextFactoryMethod = previous; }
    private:
        QWK::WindowAgentBasePrivate::WindowContextFactoryMethod previous;
    };

#ifdef TEST_WIDGETS
    auto titleSignal(const QWK::WidgetWindowAgent &) { return &QWK::WidgetWindowAgent::titleBarChanged; }
#endif
#ifdef TEST_QUICK
    auto titleSignal(const QWK::QuickWindowAgent &) { return &QWK::QuickWindowAgent::titleBarWidgetChanged; }
#endif

    void agentRows() {
        QTest::addColumn<bool>("quick");
#ifdef TEST_WIDGETS
        QTest::newRow("widgets") << false;
#endif
#ifdef TEST_QUICK
        QTest::newRow("quick") << true;
#endif
    }

    template <typename Callback>
    void withAgent(bool quick, Callback callback) {
        Q_UNUSED(quick)
#ifdef TEST_WIDGETS
        if (!quick) {
            QWidget window;
            QWK::WidgetWindowAgent agent;
            callback(window, agent);
        }
#endif
#ifdef TEST_QUICK
        if (quick) {
            QQuickWindow window;
            QWK::QuickWindowAgent agent;
            callback(window, agent);
        }
#endif
    }
}

class WindowAgentsTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void initTestCase() {
        qRegisterMetaType<Button::SystemButton>("SystemButton");
        qRegisterMetaType<Button::SystemButton>("QWK::WindowAgentBase::SystemButton");
        QCOMPARE(QGuiApplication::platformName(), QStringLiteral("offscreen"));
    }

    void setupIsSingleUse_data() { agentRows(); }
    void setupIsSingleUse() {
        QFETCH(bool, quick);
        withAgent(quick, [](auto &window, auto &agent) {
            using Window = std::decay_t<decltype(window)>;
            using Item = std::remove_pointer_t<decltype(agent.titleBar())>;
            Window other;
            Item title;
            QVERIFY(agent.setup(&window));
            QVERIFY(!agent.titleBar());
            agent.setTitleBar(&title);
            QVERIFY(!agent.setup(&window));
            QVERIFY(!agent.setup(&other));
            QCOMPARE(agent.titleBar(), &title);
            QVERIFY(!window.isVisible());
            QVERIFY(!other.isVisible());
        });
    }

    void titleSignalsAndReset_data() { agentRows(); }
    void titleSignalsAndReset() {
        QFETCH(bool, quick);
        withAgent(quick, [](auto &window, auto &agent) {
            using Item = std::remove_pointer_t<decltype(agent.titleBar())>;
            QVERIFY(agent.setup(&window));
            Item first, second, button, excluded;
            QSignalSpy changed(&agent, titleSignal(agent));
            QVERIFY(changed.isValid());
            Item *lastSignalItem = nullptr;
            bool stateReadyAtSignal = true;
            QObject::connect(&agent, titleSignal(agent), &agent, [&](Item *item) {
                lastSignalItem = item;
                stateReadyAtSignal = stateReadyAtSignal && agent.titleBar() == item;
            });
            agent.setTitleBar(&first);
            QCOMPARE(changed.count(), 1);
            QCOMPARE(lastSignalItem, &first);
            agent.setSystemButton(Button::Close, &button);
            agent.setHitTestVisible(&excluded);
            agent.setTitleBar(&first);
            QCOMPARE(changed.count(), 1);
            QCOMPARE(agent.systemButton(Button::Close), &button);
            QVERIFY(agent.isHitTestVisible(&excluded));
            agent.setTitleBar(&second);
            QCOMPARE(changed.count(), 2);
            QCOMPARE(lastSignalItem, &second);
            QVERIFY(stateReadyAtSignal);
            QCOMPARE(agent.titleBar(), &second);
            QVERIFY(!agent.systemButton(Button::Close));
            QVERIFY(!agent.isHitTestVisible(&excluded));
        });
    }

    void systemButtonSignals_data() { agentRows(); }
    void systemButtonSignals() {
        QFETCH(bool, quick);
        withAgent(quick, [](auto &window, auto &agent) {
            using Agent = std::decay_t<decltype(agent)>;
            using Item = std::remove_pointer_t<decltype(agent.titleBar())>;
            QVERIFY(agent.setup(&window));
            Item first, second;
            QSignalSpy changed(&agent, &Agent::systemButtonChanged);
            QVERIFY(changed.isValid());
            Button::SystemButton lastRole = Button::Unknown;
            Item *lastItem = nullptr;
            bool stateReadyAtSignal = true;
            QObject::connect(&agent, &Agent::systemButtonChanged, &agent,
                             [&](Button::SystemButton role, Item *item) {
                lastRole = role;
                lastItem = item;
                stateReadyAtSignal = stateReadyAtSignal && agent.systemButton(role) == item;
            });
            for (auto role : {Button::WindowIcon, Button::Help, Button::Minimize,
                              Button::Maximize, Button::Close}) {
                const int before = changed.count();
                agent.setSystemButton(role, &first);
                QCOMPARE(lastRole, role);
                QCOMPARE(lastItem, &first);
                agent.setSystemButton(role, &first);
                QCOMPARE(changed.count(), before + 1);
                agent.setSystemButton(role, &second);
                QCOMPARE(agent.systemButton(role), &second);
                QCOMPARE(lastItem, &second);
                QCOMPARE(changed.count(), before + 2);
                agent.setSystemButton(role, nullptr);
                QVERIFY(!agent.systemButton(role));
                QVERIFY(!lastItem);
                QCOMPARE(changed.count(), before + 3);
                agent.setSystemButton(role, nullptr);
                QCOMPARE(changed.count(), before + 3);
            }
            QVERIFY(stateReadyAtSignal);
        });
    }

    void systemButtonBoundaries_data() {
        QTest::addColumn<bool>("quick");
        QTest::addColumn<int>("value");
        QTest::addColumn<bool>("valid");
        for (const auto &row : QwkTest::systemButtonCases) {
#ifdef TEST_WIDGETS
            QTest::newRow(qPrintable(QString("widgets-%1").arg(row.name))) << false << row.value << row.valid;
#endif
#ifdef TEST_QUICK
            QTest::newRow(qPrintable(QString("quick-%1").arg(row.name))) << true << row.value << row.valid;
#endif
        }
    }

    void systemButtonBoundaries() {
        QFETCH(bool, quick);
        QFETCH(int, value);
        QFETCH(bool, valid);
        withAgent(quick, [=](auto &window, auto &agent) {
            using Agent = std::decay_t<decltype(agent)>;
            using Item = std::remove_pointer_t<decltype(agent.titleBar())>;
            QVERIFY(agent.setup(&window));
            Item original, replacement;
            for (auto role : {Button::WindowIcon, Button::Help, Button::Minimize,
                              Button::Maximize, Button::Close})
                agent.setSystemButton(role, &original);
            QSignalSpy changed(&agent, &Agent::systemButtonChanged);
            QVERIFY(changed.isValid());
            const auto role = static_cast<Button::SystemButton>(value);
            QCOMPARE(agent.systemButton(role), valid ? &original : nullptr);
            agent.setSystemButton(role, &replacement);
            QCOMPARE(agent.systemButton(role), valid ? &replacement : nullptr);
            agent.setSystemButton(role, &replacement);
            QCOMPARE(changed.count(), valid ? 1 : 0);
            for (auto other : {Button::WindowIcon, Button::Help, Button::Minimize,
                               Button::Maximize, Button::Close})
                QCOMPARE(agent.systemButton(other), other == role ? &replacement : &original);
            agent.setSystemButton(role, nullptr);
            QVERIFY(!agent.systemButton(role));
            agent.setSystemButton(role, nullptr);
            QCOMPARE(changed.count(), valid ? 2 : 0);
            for (auto other : {Button::WindowIcon, Button::Help, Button::Minimize,
                               Button::Maximize, Button::Close})
                QCOMPARE(agent.systemButton(other), other == role ? nullptr : &original);
        });
    }

    void hitTestVisibilityToggles_data() { agentRows(); }
    void hitTestVisibilityToggles() {
        QFETCH(bool, quick);
        withAgent(quick, [](auto &window, auto &agent) {
            using Item = std::remove_pointer_t<decltype(agent.titleBar())>;
            QVERIFY(agent.setup(&window));
            Item first, second;
            QVERIFY(!agent.isHitTestVisible(&first));
            agent.setHitTestVisible(&first);
            agent.setHitTestVisible(&first);
            agent.setHitTestVisible(&second);
            QVERIFY(agent.isHitTestVisible(&first));
            agent.setHitTestVisible(&first, false);
            QVERIFY(!agent.isHitTestVisible(&first));
            QVERIFY(agent.isHitTestVisible(&second));
            agent.setHitTestVisible(&first, false);
            agent.setHitTestVisible(&first, true);
            QVERIFY(agent.isHitTestVisible(&first));
        });
    }

    void destroyedRegistrations_data() { agentRows(); }
    void destroyedRegistrations() {
        QFETCH(bool, quick);
        withAgent(quick, [](auto &window, auto &agent) {
            using Item = std::remove_pointer_t<decltype(agent.titleBar())>;
            QVERIFY(agent.setup(&window));
            auto title = std::make_unique<Item>();
            auto button = std::make_unique<Item>();
            auto excluded = std::make_unique<Item>();
            agent.setTitleBar(title.get());
            agent.setSystemButton(Button::Close, button.get());
            agent.setHitTestVisible(excluded.get());
            button.reset();
            QVERIFY(!agent.systemButton(Button::Close));
            excluded.reset();
            title.reset();
            QVERIFY(!agent.titleBar());
            Item replacement;
            agent.setTitleBar(&replacement);
            QCOMPARE(agent.titleBar(), &replacement);
            QVERIFY(!agent.isHitTestVisible(&replacement));
            agent.setHitTestVisible(&replacement);
            QVERIFY(agent.isHitTestVisible(&replacement));
            agent.setHitTestVisible(&replacement, false);
            QVERIFY(!agent.isHitTestVisible(&replacement));
        });
    }
};

int main(int argc, char **argv) {
#ifdef Q_OS_WIN
    ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
#ifdef TEST_WIDGETS
    QApplication app(argc, argv);
#else
    QGuiApplication app(argc, argv);
#endif
    FactoryGuard guard;
    WindowAgentsTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_windowagents.moc"
