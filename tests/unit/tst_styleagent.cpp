// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>
#include <QtCore/qscopeguard.h>
#include <QWKCore/private/styleagentregistry_p.h>
#include <array>
#include <new>

namespace {
    int setupCalls = 0;
    int removalCalls = 0;
    QWK::StyleAgentRegistry registry;

    class Agent : public QWK::StyleAgent {
    public:
        QWK::StyleAgentPrivate &state() { return *d_ptr; }
    };

    void publish(Agent::SystemTheme theme, const QColor &color) {
        registry.notify([&] { return QWK::StyleAgentRegistry::Appearance{theme, color}; });
    }
}

// Only the OS subscription boundary is replaced. Theme/color setters, getters, signals,
// constructors and destructors are compiled from the unchanged production styleagent.cpp.
// This test does not modify or depend on the user's actual theme or registry.
namespace QWK {
    void StyleAgentPrivate::setupSystemThemeHook() { ++setupCalls; registry.insert(this); }
    void StyleAgentPrivate::removeSystemThemeHook() { ++removalCalls; registry.remove(this); }
}

class StyleAgentTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void init() { QVERIFY(registry.isEmpty()); setupCalls = removalCalls = 0; }
    void cleanup() { QVERIFY(registry.isEmpty()); }

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

    void pairedBroadcastAndDeduplication() {
        std::array<Agent, 3> agents;
        std::array<QString, 3> calls;
        for (size_t i = 0; i < agents.size(); ++i) {
            auto &agent = agents[i];
            connect(&agent, &Agent::systemThemeChanged, this, [&, i] {
                QCOMPARE(agents[i].systemTheme(), Agent::Light);
                QCOMPARE(agents[i].systemAccentColor(), QColor(Qt::red));
                calls[i] += 'T';
            });
            connect(&agent, &Agent::systemAccentColorChanged, this, [&, i] {
                QCOMPARE(agents[i].systemTheme(), Agent::Light);
                QCOMPARE(agents[i].systemAccentColor(), QColor(Qt::red));
                calls[i] += 'C';
            });
        }
        publish(Agent::Light, Qt::red);
        publish(Agent::Light, Qt::red);
        for (const auto &call : calls)
            QCOMPARE(call, QStringLiteral("TC"));
    }

    void creationWaitsForNextSnapshot() {
        Agent a, b;
        std::unique_ptr<Agent> added;
        int addedCalls = 0;
        auto create = [&] {
            if (!added) {
                added = std::make_unique<Agent>();
                connect(added.get(), &Agent::systemThemeChanged, this, [&] { ++addedCalls; });
            }
        };
        connect(&a, &Agent::systemThemeChanged, this, create);
        connect(&b, &Agent::systemThemeChanged, this, create);
        publish(Agent::Light, Qt::red);
        QVERIFY(added);
        QCOMPARE(added->systemTheme(), Agent::Unknown);
        QCOMPARE(addedCalls, 0);
        publish(Agent::Dark, Qt::blue);
        QCOMPARE(added->systemTheme(), Agent::Dark);
        QCOMPARE(added->systemAccentColor(), QColor(Qt::blue));
        QCOMPARE(addedCalls, 1);
    }

    void destroysRemainingSnapshot() {
        std::array<std::unique_ptr<Agent>, 3> agents;
        int themes = 0, colors = 0;
        for (auto &agent : agents)
            agent = std::make_unique<Agent>();
        for (size_t i = 0; i < agents.size(); ++i) {
            connect(agents[i].get(), &Agent::systemThemeChanged, this, [&, i] {
                ++themes;
                for (size_t j = 0; j < agents.size(); ++j) {
                    if (j != i)
                        agents[j].reset();
                }
            });
            connect(agents[i].get(), &Agent::systemAccentColorChanged, this, [&] { ++colors; });
        }
        publish(Agent::Light, Qt::red);
        QCOMPARE(themes, 1);
        QCOMPARE(colors, 1);
        QCOMPARE(removalCalls, 2);
    }

    void destroysLastAgent() {
        auto agent = std::make_unique<Agent>();
        int themes = 0, colors = 0;
        connect(agent.get(), &Agent::systemThemeChanged, this, [&] { ++themes; agent.reset(); });
        connect(agent.get(), &Agent::systemAccentColorChanged, this, [&] { ++colors; });
        publish(Agent::Light, Qt::red);
        QVERIFY(!agent);
        QVERIFY(registry.isEmpty());
        QCOMPARE(themes, 1);
        QCOMPARE(colors, 0);
        Agent replacement;
        publish(Agent::Dark, Qt::blue);
        QCOMPARE(replacement.systemTheme(), Agent::Dark);
    }

    void ownerAddressReuseWaitsForNextSnapshot() {
        alignas(Agent) unsigned char storage[2][sizeof(Agent)];
        Agent *agents[] = {new (storage[0]) Agent, new (storage[1]) Agent};
        const auto cleanup = qScopeGuard([&] { for (auto agent : agents) agent->~Agent(); });
        int replaced = -1;
        int replacementCalls = 0;
        for (int i = 0; i < 2; ++i) {
            connect(agents[i], &Agent::systemThemeChanged, this, [&, i] {
                if (replaced >= 0)
                    return;
                replaced = 1 - i; // QSet order is intentionally unspecified.
                QPointer<Agent> oldOwner(agents[replaced]);
                agents[replaced]->~Agent();
                agents[replaced] = new (storage[replaced]) Agent;
                QVERIFY(!oldOwner);
                connect(agents[replaced], &Agent::systemThemeChanged, this,
                        [&] { ++replacementCalls; });
            });
        }
        publish(Agent::Light, Qt::red);
        QVERIFY(replaced >= 0);
        QCOMPARE(agents[replaced]->systemTheme(), Agent::Unknown);
        QCOMPARE(replacementCalls, 0);
        publish(Agent::Dark, Qt::blue);
        QCOMPARE(agents[replaced]->systemTheme(), Agent::Dark);
        QCOMPARE(replacementCalls, 1);
    }

    void unregisteredLiveOwnerIsNotNotified() {
        Agent agents[2];
        int removed = -1;
        int calls = 0;
        for (int i = 0; i < 2; ++i) {
            connect(&agents[i], &Agent::systemThemeChanged, this, [&, i] {
                ++calls;
                removed = 1 - i;
                agents[removed].state().removeSystemThemeHook();
            });
        }
        publish(Agent::Light, Qt::red);
        QCOMPARE(calls, 1);
        QVERIFY(removed >= 0);
        QCOMPARE(agents[removed].systemTheme(), Agent::Unknown);
    }

    void nestedSnapshotSupersedesOuter() {
        Agent agents[2];
        int first = -1;
        std::array<QString, 2> calls;
        for (int i = 0; i < 2; ++i) {
            connect(&agents[i], &Agent::systemThemeChanged, this, [&, i] {
                const auto theme = agents[i].systemTheme();
                if (theme == Agent::Light) {
                    QCOMPARE(agents[i].systemAccentColor(), QColor(Qt::red));
                    calls[i] += 'L';
                    first = i;
                    publish(Agent::Dark, Qt::blue);
                } else {
                    QCOMPARE(theme, Agent::Dark);
                    QCOMPARE(agents[i].systemAccentColor(), QColor(Qt::blue));
                    calls[i] += 'D';
                }
            });
            connect(&agents[i], &Agent::systemAccentColorChanged, this, [&, i] {
                QCOMPARE(agents[i].systemTheme(), Agent::Dark);
                QCOMPARE(agents[i].systemAccentColor(), QColor(Qt::blue));
                calls[i] += 'C';
            });
        }
        publish(Agent::Light, Qt::red);
        QVERIFY(first >= 0);
        QCOMPARE(calls[first], QStringLiteral("LDC"));
        QCOMPARE(calls[1 - first], QStringLiteral("DC"));
        for (auto &agent : agents) {
            QCOMPARE(agent.systemTheme(), Agent::Dark);
            QCOMPARE(agent.systemAccentColor(), QColor(Qt::blue));
        }
    }

    void nestedThemeOnlyKeepsPendingAccent() {
        Agent agent;
        QString calls;
        connect(&agent, &Agent::systemThemeChanged, this, [&] {
            if (agent.systemTheme() == Agent::Light) {
                calls += 'L';
                publish(Agent::Dark, Qt::red);
            } else {
                calls += 'D';
            }
        });
        connect(&agent, &Agent::systemAccentColorChanged, this, [&] {
            QCOMPARE(agent.systemTheme(), Agent::Dark);
            QCOMPARE(agent.systemAccentColor(), QColor(Qt::red));
            calls += 'C';
        });
        publish(Agent::Light, Qt::red);
        QCOMPARE(calls, QStringLiteral("LDC"));
    }

    void samplingReentrySupersedesOuter() {
        Agent a, b;
        QSignalSpy first(&a, &Agent::systemThemeChanged);
        QSignalSpy second(&b, &Agent::systemThemeChanged);
        registry.notify([&] {
            publish(Agent::Dark, Qt::blue);
            return QWK::StyleAgentRegistry::Appearance{Agent::Light, Qt::red};
        });
        QCOMPARE(first.count(), 1);
        QCOMPARE(second.count(), 1);
        for (const auto agent : {&a, &b}) {
            QCOMPARE(agent->systemTheme(), Agent::Dark);
            QCOMPARE(agent->systemAccentColor(), QColor(Qt::blue));
        }
    }
};

QTEST_GUILESS_MAIN(StyleAgentTest)
#include "tst_styleagent.moc"
