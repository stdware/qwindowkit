// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>
#include <QWKCore/private/styleagent_p.h>

namespace {
    int setupCalls = 0;
    int removalCalls = 0;

    class Agent : public QWK::StyleAgent {
    public:
        QWK::StyleAgentPrivate &state() { return *d_ptr; }
    };
}

// Only the OS subscription boundary is replaced. Theme/color setters, getters, signals,
// constructors and destructors are compiled from the unchanged production styleagent.cpp.
// This test does not modify or depend on the user's actual theme or registry.
namespace QWK {
    void StyleAgentPrivate::setupSystemThemeHook() { ++setupCalls; }
    void StyleAgentPrivate::removeSystemThemeHook() { ++removalCalls; }
}

class StyleAgentTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void init() { setupCalls = removalCalls = 0; }

    void initialState() {
        Agent agent;
        QCOMPARE(agent.systemTheme(), Agent::Unknown);
        QVERIFY(!agent.systemAccentColor().isValid());
        QSignalSpy theme(&agent, &Agent::systemThemeChanged);
        QSignalSpy color(&agent, &Agent::systemAccentColorChanged);
        QVERIFY(theme.isValid());
        QVERIFY(color.isValid());
        agent.state().notifyThemeChanged(Agent::Unknown);
        agent.state().notifyAccentColorChanged({});
        QCOMPARE(theme.count(), 0);
        QCOMPARE(color.count(), 0);
    }

    void themeChanges_data() {
        QTest::addColumn<int>("theme");
        QTest::newRow("light") << int(Agent::Light);
        QTest::newRow("dark") << int(Agent::Dark);
        QTest::newRow("high-contrast") << int(Agent::HighContrast);
        QTest::newRow("unknown") << int(Agent::Unknown);
    }

    void themeChanges() {
        QFETCH(int, theme);
        Agent agent;
        const auto target = static_cast<Agent::SystemTheme>(theme);
        agent.state().notifyThemeChanged(target == Agent::Unknown ? Agent::Light : Agent::Unknown);
        QSignalSpy changed(&agent, &Agent::systemThemeChanged);
        agent.state().notifyThemeChanged(target);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(agent.systemTheme(), target);
        agent.state().notifyThemeChanged(target);
        QCOMPARE(changed.count(), 1);
    }

    void colorChangesIncludingInvalid() {
        Agent agent;
        QSignalSpy changed(&agent, &Agent::systemAccentColorChanged);
        const QColor red(Qt::red), blue(Qt::blue);
        agent.state().notifyAccentColorChanged(red);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(agent.systemAccentColor(), red);
        agent.state().notifyAccentColorChanged(red);
        QCOMPARE(changed.count(), 1);
        agent.state().notifyAccentColorChanged(blue);
        QCOMPARE(changed.count(), 2);
        QCOMPARE(agent.systemAccentColor(), blue);
        agent.state().notifyAccentColorChanged({});
        QCOMPARE(changed.count(), 3);
        QVERIFY(!agent.systemAccentColor().isValid());
        agent.state().notifyAccentColorChanged({});
        QCOMPARE(changed.count(), 3);
    }

    void signalsObserveNewValues() {
        Agent agent;
        Agent::SystemTheme observedTheme = Agent::Unknown;
        QColor observedColor;
        connect(&agent, &Agent::systemThemeChanged, &agent,
                [&] { observedTheme = agent.systemTheme(); });
        connect(&agent, &Agent::systemAccentColorChanged, &agent,
                [&] { observedColor = agent.systemAccentColor(); });
        agent.state().notifyThemeChanged(Agent::Dark);
        QCOMPARE(observedTheme, Agent::Dark);
        agent.state().notifyAccentColorChanged(Qt::green);
        QCOMPARE(observedColor, QColor(Qt::green));
    }

    void reentrantNotifications() {
        Agent agent;
        QList<int> themes;
        connect(&agent, &Agent::systemThemeChanged, &agent, [&] {
            themes.append(int(agent.systemTheme()));
            if (agent.systemTheme() == Agent::Light)
                agent.state().notifyThemeChanged(Agent::Dark);
        });
        agent.state().notifyThemeChanged(Agent::Light);
        QCOMPARE(themes, QList<int>({int(Agent::Light), int(Agent::Dark)}));
        QCOMPARE(agent.systemTheme(), Agent::Dark);
        agent.state().notifyThemeChanged(Agent::Dark);
        QCOMPARE(themes.size(), 2);
    }

    void hookLifetime() {
        {
            Agent first;
            QCOMPARE(setupCalls, 1);
            QCOMPARE(removalCalls, 0);
            {
                Agent second;
                QCOMPARE(setupCalls, 2);
                QCOMPARE(removalCalls, 0);
            }
            QCOMPARE(removalCalls, 1);
        }
        QCOMPARE(removalCalls, 2);
    }
};

QTEST_GUILESS_MAIN(StyleAgentTest)
#include "tst_styleagent.moc"
