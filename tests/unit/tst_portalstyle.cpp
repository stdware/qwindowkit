// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <QtCore/QPointer>
#include <QtCore/QThread>
#include <QtGui/QGuiApplication>
#include <QtGui/QPalette>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>
#include "portalsettings_p.h"
#include <limits>

namespace {
    QVariantMap settings(uint scheme, const QVariantList &accent = {1.0, 0.0, 0.0}) {
        return {{"color-scheme", scheme}, {"accent-color", accent}};
    }
    class Source : public QWK::PortalSettingsSource {
    public:
        QVariantMap values = settings(1);
        bool available = true, hold = false;
        int requests = 0, completions = 0;
        quint64 generation = 0;
        QVariantMap held;
        void read() override {
            ++requests;
            if (hold) {
                held = values;
                return;
            }
            const auto copy = values;
            const auto valid = available;
            const auto token = generation;
            QTimer::singleShot(0, this, [this, copy, valid, token] {
                if (token != generation) return;
                ++completions;
                Q_EMIT snapshot(copy, valid);
            });
        }
        void change(const QVariantMap &value) { values = value; Q_EMIT changed(); }
        void loseService() { ++generation; available = false; Q_EMIT unavailable(); }
        void completeHeld() { ++completions; Q_EMIT snapshot(held, true); }
    };
    Source *lastSource = nullptr;
    class Agent : public QWK::StyleAgent {
    public:
        QWK::StyleAgentPrivate &state() { return *d_ptr; }
    };
}

namespace QWK {
    // Exactly the Linux hookup, with only the D-Bus transport replaced. All refresh,
    // subscription, parsing, cache, getter and notification code is production code.
    void StyleAgentPrivate::setupSystemThemeHook() {
        auto source = new Source;
        lastSource = source;
        systemThemeHook.reset(new PortalStyleObserver(this, source));
    }
    void StyleAgentPrivate::removeSystemThemeHook() { systemThemeHook.reset(); }
}

class PortalStyleTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void initialSnapshotAndPaletteIndependence() {
        const auto oldPalette = QGuiApplication::palette();
        QPalette applicationPalette;
        applicationPalette.setColor(QPalette::Highlight, Qt::green);
        QGuiApplication::setPalette(applicationPalette);
        Agent agent;
        QGuiApplication::setPalette(oldPalette);
        QCOMPARE(agent.systemTheme(), Agent::Unknown); // Asynchronous construction.
        QVERIFY(!agent.systemAccentColor().isValid());
        QSignalSpy theme(&agent, &Agent::systemThemeChanged);
        QSignalSpy accent(&agent, &Agent::systemAccentColorChanged);
        QColor colorInThemeSlot;
        Agent::SystemTheme themeInColorSlot = Agent::Unknown;
        connect(&agent, &Agent::systemThemeChanged, &agent,
                [&] { colorInThemeSlot = agent.systemAccentColor(); });
        connect(&agent, &Agent::systemAccentColorChanged, &agent,
                [&] { themeInColorSlot = agent.systemTheme(); });
        QTRY_COMPARE_WITH_TIMEOUT(theme.count(), 1, 1000);
        QCOMPARE(accent.count(), 1);
        QCOMPARE(agent.systemTheme(), Agent::Dark);
        QCOMPARE(colorInThemeSlot, QColor(Qt::red));
        QCOMPARE(themeInColorSlot, Agent::Dark);
    }

    void changesAndDeduplication() {
        Agent agent;
        auto source = lastSource;
        QTRY_COMPARE_WITH_TIMEOUT(agent.systemTheme(), Agent::Dark, 1000);
        QSignalSpy theme(&agent, &Agent::systemThemeChanged);
        QSignalSpy accent(&agent, &Agent::systemAccentColorChanged);
        source->change(settings(2));
        QTRY_COMPARE_WITH_TIMEOUT(theme.count(), 1, 1000);
        QCOMPARE(accent.count(), 0);
        source->change(settings(2, {0.0, 0.0, 1.0}));
        QTRY_COMPARE_WITH_TIMEOUT(accent.count(), 1, 1000);
        QCOMPARE(theme.count(), 1);
        const int completed = source->completions;
        source->change(source->values);
        QTRY_VERIFY_WITH_TIMEOUT(source->completions > completed, 1000);
        QCOMPARE(accent.count(), 1);
        QCOMPARE(theme.count(), 1);
        source->change(settings(1, {0.0, 1.0, 0.0}));
        QTRY_COMPARE_WITH_TIMEOUT(theme.count(), 2, 1000);
        QCOMPARE(accent.count(), 2);
        QCOMPARE(agent.systemAccentColor(), QColor(Qt::green));
    }

    void missingAndMalformedValues_data() {
        QTest::addColumn<QVariantMap>("value");
        QTest::addColumn<int>("theme");
        QTest::addColumn<bool>("validColor");
        QTest::newRow("missing") << QVariantMap{} << int(Agent::Unknown) << false;
        QTest::newRow("no-preference") << settings(0) << int(Agent::Unknown) << true;
        QTest::newRow("unknown-scheme") << settings(9) << int(Agent::Unknown) << true;
        auto high = settings(1); high.insert("contrast", 1u);
        QTest::newRow("high-contrast") << high << int(Agent::HighContrast) << true;
        QTest::newRow("negative-rgb") << settings(1, {-0.1, 0.0, 1.0}) << int(Agent::Dark) << false;
        QTest::newRow("large-rgb") << settings(1, {2.0, 0.0, 1.0}) << int(Agent::Dark) << false;
        QTest::newRow("nan-rgb") << settings(1, {std::numeric_limits<double>::quiet_NaN(), 0.0, 1.0})
                                 << int(Agent::Dark) << false;
        QTest::newRow("short-rgb") << settings(1, {0.0, 1.0}) << int(Agent::Dark) << false;
        QTest::newRow("wrong-rgb-type") << settings(1, {0, 0, 1}) << int(Agent::Dark) << false;
        QVariantMap wrong = settings(1); wrong.insert("color-scheme", "1");
        QTest::newRow("wrong-scheme-type") << wrong << int(Agent::Unknown) << true;
    }
    void missingAndMalformedValues() {
        QFETCH(QVariantMap, value);
        QFETCH(int, theme);
        QFETCH(bool, validColor);
        Agent agent;
        auto source = lastSource;
        source->values = value;
        QTRY_COMPARE_WITH_TIMEOUT(source->completions, 1, 1000);
        QCOMPARE(int(agent.systemTheme()), theme);
        QCOMPARE(agent.systemAccentColor().isValid(), validColor);
    }

    void changesBeforeInitialRead() {
        Agent agent;
        lastSource->change(settings(2, {0.0, 0.0, 1.0}));
        QTRY_COMPARE_WITH_TIMEOUT(agent.systemTheme(), Agent::Light, 1000);
        QCOMPARE(agent.systemAccentColor(), QColor(Qt::blue));
    }

    void staleReadIsDiscarded() {
        Agent agent;
        auto source = lastSource;
        source->hold = true;
        QSignalSpy theme(&agent, &Agent::systemThemeChanged);
        QTRY_COMPARE_WITH_TIMEOUT(source->requests, 1, 1000);
        source->change(settings(2, {0.0, 0.0, 1.0}));
        QCoreApplication::processEvents();
        source->hold = false;
        source->completeHeld();
        QCOMPARE(theme.count(), 0); // Never publish the older dark/red snapshot.
        QTRY_COMPARE_WITH_TIMEOUT(agent.systemTheme(), Agent::Light, 1000);
        QCOMPARE(agent.systemAccentColor(), QColor(Qt::blue));
        QCOMPARE(theme.count(), 1);
        QCOMPARE(source->requests, 2);
    }

    void serviceLossAndRecovery() {
        Agent agent;
        auto source = lastSource;
        QTRY_COMPARE_WITH_TIMEOUT(agent.systemTheme(), Agent::Dark, 1000);
        source->loseService();
        QCOMPARE(agent.systemTheme(), Agent::Unknown);
        QVERIFY(!agent.systemAccentColor().isValid());
        QTRY_VERIFY_WITH_TIMEOUT(source->requests >= 2, 1000);
        source->available = true;
        source->change(settings(2, {0.0, 0.0, 1.0}));
        QTRY_COMPARE_WITH_TIMEOUT(agent.systemTheme(), Agent::Light, 1000);
        QCOMPARE(agent.systemAccentColor(), QColor(Qt::blue));
    }

    void failureRetriesWithoutChangeSignal() {
        Agent agent;
        auto source = lastSource;
        source->available = false;
        QTRY_COMPARE_WITH_TIMEOUT(source->completions, 1, 1000);
        QCOMPARE(agent.systemTheme(), Agent::Unknown);
        source->available = true;
        QTRY_COMPARE_WITH_TIMEOUT(agent.systemTheme(), Agent::Dark, 6000);
        QCOMPARE(source->requests, 2);
    }

    void deleteAgentInThemeSlot() {
        auto agent = new Agent;
        QPointer<Agent> guard(agent);
        QPointer<Source> source(lastSource);
        int colorSignals = 0;
        connect(agent, &Agent::systemAccentColorChanged, this, [&] { ++colorSignals; });
        connect(agent, &Agent::systemThemeChanged, this, [agent] { delete agent; });
        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 1000);
        QVERIFY(source.isNull());
        QCOMPARE(colorSignals, 0);
    }

    void createAndDeleteOtherAgentInSlot() {
        auto victim = new Agent;
        QPointer<Agent> victimGuard(victim);
        Agent first;
        Agent *replacement = nullptr;
        connect(&first, &Agent::systemThemeChanged, &first, [&] {
            delete victimGuard.data();
            if (!replacement) replacement = new Agent;
        });
        QTRY_COMPARE_WITH_TIMEOUT(first.systemTheme(), Agent::Dark, 1000);
        QVERIFY(victimGuard.isNull());
        QVERIFY(replacement);
        QTRY_COMPARE_WITH_TIMEOUT(replacement->systemTheme(), Agent::Dark, 1000);
        delete replacement;
    }

    void pendingReadDiesWithAgent() {
        auto agent = new Agent;
        QPointer<Source> source(lastSource);
        source->hold = true;
        QTRY_COMPARE_WITH_TIMEOUT(source->requests, 1, 1000);
        delete agent;
        QVERIFY(source.isNull());
        QCoreApplication::processEvents();
    }

    void reentrantSnapshotSupersedesOlderColorSignal() {
        Agent agent;
        int colors = 0;
        connect(&agent, &Agent::systemAccentColorChanged, &agent, [&] { ++colors; });
        connect(&agent, &Agent::systemThemeChanged, &agent, [&] {
            if (agent.systemTheme() == Agent::Dark)
                agent.state().notifyAppearanceChanged(Agent::Light, QColor(Qt::blue));
        });
        QTRY_COMPARE_WITH_TIMEOUT(agent.systemTheme(), Agent::Light, 1000);
        QCOMPARE(agent.systemAccentColor(), QColor(Qt::blue));
        QCOMPARE(colors, 1);
    }

    void queuedSourceNotificationFromWorker() {
        Agent agent;
        auto source = lastSource;
        QTRY_COMPARE_WITH_TIMEOUT(agent.systemTheme(), Agent::Dark, 1000);
        QThread worker;
        bool correctThread = false;
        connect(&worker, &QThread::started, source, [source] {
            source->change(settings(2));
        }, Qt::DirectConnection); // Emit from the worker; the observer must queue delivery.
        connect(&agent, &Agent::systemThemeChanged, &agent,
                [&] { correctThread = QThread::currentThread() == agent.thread(); });
        worker.start();
        const bool updated = QTest::qWaitFor([&] { return agent.systemTheme() == Agent::Light; }, 1000);
        worker.quit();
        QVERIFY(worker.wait(1000));
        QVERIFY(updated);
        QVERIFY(correctThread);
    }

    void reentrantThemePreservesPendingAccent() {
        Agent agent;
        QSignalSpy accent(&agent, &Agent::systemAccentColorChanged);
        connect(&agent, &Agent::systemThemeChanged, &agent, [&] {
            if (agent.systemTheme() == Agent::Dark)
                agent.state().notifyAppearanceChanged(Agent::Light, agent.systemAccentColor());
        });
        QTRY_COMPARE_WITH_TIMEOUT(agent.systemTheme(), Agent::Light, 1000);
        QCOMPARE(agent.systemAccentColor(), QColor(Qt::red));
        QCOMPARE(accent.count(), 1);
    }

    void failedRefreshInvalidatesPreviousSnapshot() {
        Agent agent;
        auto source = lastSource;
        QTRY_COMPARE_WITH_TIMEOUT(agent.systemTheme(), Agent::Dark, 1000);
        QSignalSpy theme(&agent, &Agent::systemThemeChanged);
        QSignalSpy accent(&agent, &Agent::systemAccentColorChanged);
        source->available = false;
        source->change(settings(2));
        QTRY_COMPARE_WITH_TIMEOUT(agent.systemTheme(), Agent::Unknown, 1000);
        QVERIFY(!agent.systemAccentColor().isValid());
        QCOMPARE(theme.count(), 1);
        QCOMPARE(accent.count(), 1);
    }
};

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    PortalStyleTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_portalstyle.moc"
