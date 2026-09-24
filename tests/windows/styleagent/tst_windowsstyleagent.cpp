// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <QtCore/qt_windows.h>
#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>
#include <QWKCore/private/styleagent_p.h>

namespace {
    class Agent : public QWK::StyleAgent {
    public:
        // Make a refresh observable without changing the user's Windows settings.
        // Production platform reads, native subscriptions and notification code are unchanged.
        void invalidateCache() {
            d_ptr->systemTheme = Unknown;
            d_ptr->systemAccentColor = {};
        }
    };
}

class WindowsStyleAgentTest : public QObject {
    Q_OBJECT
    std::unique_ptr<QWindow> window;
    HWND hwnd = nullptr;

    void send(UINT message = WM_THEMECHANGED) {
        static const wchar_t immersive[] = L"ImmersiveColorSet";
        SendMessageW(hwnd, message, 0,
                     message == WM_SETTINGCHANGE ? reinterpret_cast<LPARAM>(immersive) : 0);
    }

private Q_SLOTS:
    void init() {
        QCOMPARE(QGuiApplication::platformName(), QStringLiteral("windows"));
        window = std::make_unique<QWindow>();
        window->create();
        hwnd = reinterpret_cast<HWND>(window->winId());
        QVERIFY(IsWindow(hwnd));
    }
    void cleanup() { window.reset(); hwnd = nullptr; }

    void messages_data() {
        QTest::addColumn<uint>("message");
        QTest::newRow("theme") << uint(WM_THEMECHANGED);
        QTest::newRow("system-color") << uint(WM_SYSCOLORCHANGE);
        QTest::newRow("dwm-color") << uint(WM_DWMCOLORIZATIONCOLORCHANGED);
        QTest::newRow("immersive-setting") << uint(WM_SETTINGCHANGE);
    }
    void messages() {
        QFETCH(uint, message);
        Agent a, b;
        const auto theme = a.systemTheme();
        const auto color = a.systemAccentColor();
        QVERIFY(theme != Agent::Unknown);
        QSignalSpy aTheme(&a, &Agent::systemThemeChanged), bTheme(&b, &Agent::systemThemeChanged);
        QSignalSpy aColor(&a, &Agent::systemAccentColorChanged), bColor(&b, &Agent::systemAccentColorChanged);
        for (auto agent : {&a, &b}) {
            agent->invalidateCache();
            auto checkPair = [agent, theme, color] {
                QCOMPARE(agent->systemTheme(), theme);
                QCOMPARE(agent->systemAccentColor(), color);
            };
            connect(agent, &Agent::systemThemeChanged, this, checkPair);
            connect(agent, &Agent::systemAccentColorChanged, this, checkPair);
        }
        send(message);
        QCOMPARE(aTheme.count(), 1);
        QCOMPARE(bTheme.count(), 1);
        QCOMPARE(aColor.count(), color.isValid() ? 1 : 0);
        QCOMPARE(bColor.count(), color.isValid() ? 1 : 0);
        send(message);
        QCOMPARE(aTheme.count(), 1);
        QCOMPARE(bTheme.count(), 1);
        QCOMPARE(aColor.count(), color.isValid() ? 1 : 0);
        QCOMPARE(bColor.count(), color.isValid() ? 1 : 0);
    }

    void lastSubscriberDestroyed_data() {
        QTest::addColumn<bool>("nested");
        QTest::newRow("direct") << false;
        QTest::newRow("nested") << true;
    }
    void lastSubscriberDestroyed() {
        QFETCH(bool, nested);
        auto agent = std::make_unique<Agent>();
        int calls = 0;
        agent->invalidateCache();
        connect(agent.get(), &Agent::systemThemeChanged, this, [&] {
            ++calls;
            if (nested && calls == 1) {
                agent->invalidateCache();
                send();
            } else {
                agent.reset();
            }
        });
        send();
        QVERIFY(!agent);
        QCOMPARE(calls, nested ? 2 : 1);
        send(); // Empty native master must be safe after the setting filter unwinds.
        Agent replacement;
        const auto theme = replacement.systemTheme();
        replacement.invalidateCache();
        QSignalSpy changed(&replacement, &Agent::systemThemeChanged);
        send();
        QCOMPARE(changed.count(), 1);
        QCOMPARE(replacement.systemTheme(), theme);
    }

    void replacementDuringPendingUninstall() {
        auto agent = std::make_unique<Agent>();
        std::unique_ptr<Agent> replacement;
        int calls = 0;
        const auto theme = agent->systemTheme();
        agent->invalidateCache();
        connect(agent.get(), &Agent::systemThemeChanged, this, [&] {
            ++calls;
            agent.reset();
            replacement = std::make_unique<Agent>();
            replacement->invalidateCache();
        });
        send();
        QVERIFY(!agent);
        QVERIFY(replacement);
        QCOMPARE(calls, 1);
        QCOMPARE(replacement->systemTheme(), Agent::Unknown);
        QSignalSpy changed(replacement.get(), &Agent::systemThemeChanged);
        send();
        QCOMPARE(changed.count(), 1);
        QCOMPARE(replacement->systemTheme(), theme);
    }
};

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    WindowsStyleAgentTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_windowsstyleagent.moc"
