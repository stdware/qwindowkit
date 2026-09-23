// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <functional>
#include <QtTest/QTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QWKCore/private/windowagentbase_p.h>
#include <QWKWidgets/widgetwindowagent.h>
#ifdef TEST_QUICK
#  include <QWKQuick/quickwindowagent.h>
#endif
#include "win32windowcontext_p.h"
#include "qwkwindowsextra_p.h"

using namespace QWK;

namespace {
    const QString acrylic = QStringLiteral("acrylic-material");
    const QString extra = QStringLiteral("extra-margins");
    const QMargins custom(3, 7, 5, 9), extended(65536, 0, 0, 0);
    constexpr int automatic = 0, mica = 2, transient = 3, tabbed = 4;

    class Context : public Win32WindowContext {
    public:
        bool controlled = true, supported = true, legacy = false;
        bool rejectQuery = false, rejectSet = false, rejectMargins = false, rejectRollback = false;
        int backdrop = automatic, sets = 0, marginCalls = 0;
        mutable int queries = 0;
        QMargins appliedMargins;
        ACCENT_POLICY policy{};
        QStringList calls;
        std::function<void(const QString &)> callback;
        bool realSupport() const { return Win32WindowContext::supportsSystemBackdrop(); }
        HRESULT realQuery(int *value) const { return Win32WindowContext::querySystemBackdrop(value); }
        bool realLegacySupport() const { return Win32WindowContext::supportsLegacyAcrylic(); }
        void resetCalls() { calls.clear(); sets = queries = marginCalls = 0; }
    protected:
        bool supportsSystemBackdrop() const override {
            return !legacy && (controlled ? supported : Win32WindowContext::supportsSystemBackdrop());
        }
        bool supportsLegacyAcrylic() const override {
            return controlled ? legacy && supported : Win32WindowContext::supportsLegacyAcrylic();
        }
        bool setAccentPolicy(const ACCENT_POLICY &value) override {
            ++sets;
            const int type = value.dwAccentState == ACCENT_ENABLE_ACRYLICBLURBEHIND ? transient : automatic;
            calls.append(QString("backdrop:%1").arg(type));
            bool result = true;
            if (controlled) {
                if (rejectSet || rejectRollback)
                    result = false;
                else {
                    policy = value;
                    backdrop = type;
                }
            } else {
                result = Win32WindowContext::setAccentPolicy(value);
                if (result)
                    policy = value;
            }
            const auto action = callback;
            if (action)
                action("backdrop");
            return result;
        }
        HRESULT querySystemBackdrop(int *value) const override {
            ++queries;
            if (!controlled)
                return Win32WindowContext::querySystemBackdrop(value);
            if (rejectQuery)
                return E_FAIL;
            *value = backdrop;
            return S_OK;
        }
        HRESULT setSystemBackdrop(int value) override {
            ++sets;
            calls.append(QString("backdrop:%1").arg(value));
            HRESULT result = S_OK;
            if (controlled) {
                if (rejectSet || (rejectRollback && sets == 2))
                    result = E_FAIL;
                else
                    backdrop = value;
            } else {
                result = Win32WindowContext::setSystemBackdrop(value);
            }
            const auto action = callback;
            if (action)
                action("backdrop"); // May delete this context.
            return result;
        }
        bool extendFrameMargins(const QMargins &margins) override {
            ++marginCalls;
            calls.append("margins");
            const bool result = controlled ? !rejectMargins && !(legacy && rejectRollback && marginCalls == 2)
                                          : Win32WindowContext::extendFrameMargins(margins);
            if (result)
                appliedMargins = margins;
            const auto action = callback;
            if (action)
                action("margins");
            return result;
        }
    };

    Context *context = nullptr;
    bool nativeMode = false;
    bool legacyMode = false;
    AbstractWindowContext *createContext() {
        context = new Context;
        context->controlled = !nativeMode;
        context->legacy = legacyMode;
        return context;
    }
    class Widget : public QWidget {
    public:
        void recreate() { destroy(); create(); }
    };
    struct Host {
        explicit Host(bool quick = false) {
#ifdef TEST_QUICK
            if (quick) {
                window = std::make_unique<QQuickWindow>();
                window->resize(320, 240);
                auto a = std::make_unique<QuickWindowAgent>();
                if (!a->setup(window.get()))
                    qFatal("Quick agent setup failed");
                agent = std::move(a);
                window->show();
            } else
#else
            Q_UNUSED(quick)
#endif
            {
                widget = std::make_unique<Widget>();
                widget->resize(320, 240);
                auto a = std::make_unique<WidgetWindowAgent>();
                if (!a->setup(widget.get()))
                    qFatal("Widget agent setup failed");
                agent = std::move(a);
                widget->show();
            }
            QCoreApplication::processEvents();
            if (!context->windowId() || !::IsWindow(reinterpret_cast<HWND>(context->windowId())))
                qFatal("Expected a real HWND");
        }
        void recreate() {
            if (widget)
                widget->recreate();
#ifdef TEST_QUICK
            else { window->destroy(); window->create(); }
#endif
        }
        std::unique_ptr<Widget> widget;
#ifdef TEST_QUICK
        std::unique_ptr<QQuickWindow> window;
#endif
        std::unique_ptr<WindowAgentBase> agent;
    };
}

class AcrylicTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void init() {
        previousFactory = WindowAgentBasePrivate::windowContextFactoryMethod;
        WindowAgentBasePrivate::windowContextFactoryMethod = createContext;
        nativeMode = false;
        legacyMode = false;
    }
    void cleanup() { WindowAgentBasePrivate::windowContextFactoryMethod = previousFactory; }

    void failures_data() {
        QTest::addColumn<bool>("initial");
        QTest::addColumn<int>("operation");
        QTest::addColumn<QString>("failure");
        QTest::addColumn<bool>("legacy");
        for (bool legacy : {false, true})
        for (bool initial : {false, true})
            for (int operation = 0; operation < 3; ++operation)
                for (const auto failure : {"none", "unsupported", "query", "set", "margins", "rollback"}) {
                    if (legacy && QString(failure) == "query")
                        continue; // Legacy policy is intentionally not queried.
                    const auto row = QString("%1-%2-%3-%4").arg(legacy ? "legacy" : "modern").arg(initial).arg(operation).arg(failure).toLatin1();
                    QTest::newRow(row.constData()) << initial << operation << QString(failure) << legacy;
                }
    }
    void failures() {
        QFETCH(bool, initial);
        QFETCH(int, operation);
        QFETCH(QString, failure);
        QFETCH(bool, legacy);
        legacyMode = legacy;
        Host host;
        QVERIFY(host.agent->setWindowAttribute(extra, QVariant::fromValue(custom)));
        QVERIFY(host.agent->setWindowAttribute(acrylic, initial));
        const auto previousMargins = context->appliedMargins;
        const int previousBackdrop = context->backdrop;
        context->resetCalls();
        context->supported = failure != "unsupported";
        context->rejectQuery = failure == "query";
        context->rejectSet = failure == "set";
        context->rejectMargins = failure == "margins" || (!legacy && failure == "rollback");
        context->rejectRollback = failure == "rollback";
        const QVariant request = operation == 2 ? QVariant() : QVariant(operation == 1);
        if (failure == "rollback")
            QTest::ignoreMessage(QtWarningMsg, legacy
                ? "QWindowKit: Acrylic margins rollback failed; retry the effect update."
                : "QWindowKit: Acrylic backdrop rollback failed; retry the effect update.");
        QCOMPARE(host.agent->setWindowAttribute(acrylic, request), failure == "none");
        QCOMPARE(host.agent->windowAttribute(acrylic), failure == "none" ? request : QVariant(initial));
        QCOMPARE(host.agent->windowAttribute(extra).value<QMargins>(), custom);
        QCOMPARE(context->appliedMargins, failure == "none" || (legacy && failure == "rollback")
            ? (operation == 1 ? extended : custom) : previousMargins);
        QCOMPARE(context->backdrop, failure == "none" || (!legacy && failure == "rollback")
            ? (operation == 1 ? transient : automatic) : previousBackdrop);
        QCOMPARE(context->queries, legacy || failure == "unsupported" ? 0 : 1);
        QStringList expected;
        if (legacy && failure != "unsupported") {
            expected << "margins";
            if (failure != "margins") {
                expected << QString("backdrop:%1").arg(operation == 1 ? transient : automatic);
                if (failure == "set" || failure == "rollback")
                    expected << "margins";
            }
        } else if (!legacy && failure != "unsupported" && failure != "query") {
            expected << QString("backdrop:%1").arg(operation == 1 ? transient : automatic);
            if (failure != "set") {
                expected << "margins";
                if (failure == "margins" || failure == "rollback")
                    expected << QString("backdrop:%1").arg(previousBackdrop);
            }
        }
        QCOMPARE(context->calls, expected);
        if (legacy && failure == "none") {
            QCOMPARE(context->policy.dwAccentState, DWORD(operation == 1 ? 4 : 0));
            QCOMPARE(context->policy.dwAccentFlags, DWORD(operation == 1 ? 482 : 0));
            QCOMPARE(context->policy.dwGradientColor, DWORD(0));
        }
        if (failure == "rollback") {
            context->rejectMargins = context->rejectRollback = false;
            QVERIFY(host.agent->setWindowAttribute(acrylic, request));
            QCOMPARE(host.agent->windowAttribute(acrylic), request);
            QCOMPARE(context->appliedMargins, operation == 1 ? extended : custom);
        }
    }

    void firstFailureAndForeignBackdrop() {
        Host host;
        QVERIFY(host.agent->setWindowAttribute(extra, QVariant::fromValue(custom)));
        context->backdrop = tabbed; // The actual prior effect is not described by acrylic's cache.
        context->rejectMargins = true;
        QVERIFY(!host.agent->setWindowAttribute(acrylic, true));
        QVERIFY(!host.agent->windowAttribute(acrylic).isValid());
        QCOMPARE(context->backdrop, tabbed);
        QCOMPARE(context->appliedMargins, custom);
    }

    void legacyRollbackPreservesCompletePolicy() {
        legacyMode = true;
        Host host;
        QVERIFY(host.agent->setWindowAttribute(extra, QVariant::fromValue(custom)));
        context->policy = {ACCENT_ENABLE_BLURBEHIND, 7, 0x80442211, 9};
        context->rejectMargins = true;
        QVERIFY(!host.agent->setWindowAttribute(acrylic, true));
        QVERIFY(!host.agent->windowAttribute(acrylic).isValid());
        QCOMPARE(context->policy.dwAccentState, DWORD(ACCENT_ENABLE_BLURBEHIND));
        QCOMPARE(context->policy.dwAccentFlags, DWORD(7));
        QCOMPARE(context->policy.dwGradientColor, DWORD(0x80442211));
        QCOMPARE(context->policy.dwAnimationId, DWORD(9));
        QCOMPARE(context->appliedMargins, custom);
    }

    void legacyRestoresLatestMargins() {
        legacyMode = true;
        Host host;
        QVERIFY(host.agent->setWindowAttribute(extra, QVariant::fromValue(custom)));
        context->legacy = false; // Set up the current-OS Mica path before testing legacy Acrylic.
        QVERIFY(host.agent->setWindowAttribute("mica", true));
        context->legacy = true;
        QCOMPARE(context->appliedMargins, extended);
        context->rejectSet = true;
        QVERIFY(!host.agent->setWindowAttribute(acrylic, false));
        QCOMPARE(context->appliedMargins, extended);
        const QMargins negative(-1, -1, -1, -1);
        QVERIFY(host.agent->setWindowAttribute(extra, QVariant::fromValue(negative)));
        QVERIFY(!host.agent->setWindowAttribute(acrylic, true));
        QCOMPARE(context->appliedMargins, negative);
        QCOMPARE(host.agent->windowAttribute(extra).value<QMargins>(), negative);
        QVERIFY(!host.agent->windowAttribute(acrylic).isValid());
    }

    void legacyFailureKeepsInnerMargins() {
        legacyMode = true;
        Host host;
        QVERIFY(host.agent->setWindowAttribute(extra, QVariant::fromValue(custom)));
        context->rejectSet = true;
        const QMargins inner(8, 9, 10, 11);
        bool invoked = false, accepted = false;
        context->callback = [&](const QString &stage) {
            if (stage != "backdrop" || invoked)
                return;
            invoked = true;
            accepted = host.agent->setWindowAttribute(extra, QVariant::fromValue(inner));
        };
        QVERIFY(!host.agent->setWindowAttribute(acrylic, true));
        QVERIFY(invoked);
        QVERIFY(accepted);
        QCOMPARE(context->appliedMargins, inner);
        QCOMPARE(host.agent->windowAttribute(extra).value<QMargins>(), inner);
        QVERIFY(!host.agent->windowAttribute(acrylic).isValid());
    }

    void replay_data() {
        QTest::addColumn<bool>("reject");
        QTest::addColumn<bool>("legacy");
        QTest::newRow("success") << false << false;
        QTest::newRow("failure") << true << false;
        QTest::newRow("legacy-success") << false << true;
        QTest::newRow("legacy-failure") << true << true;
    }
    void replay() {
        QFETCH(bool, reject);
        QFETCH(bool, legacy);
        legacyMode = legacy;
        Host host;
        QVERIFY(host.agent->setWindowAttribute(extra, QVariant::fromValue(custom)));
        QVERIFY(host.agent->setWindowAttribute(acrylic, true));
        context->rejectSet = reject;
        context->resetCalls();
        host.recreate();
        QVERIFY(context->windowId());
        QCOMPARE(host.agent->windowAttribute(extra).value<QMargins>(), custom);
        QCOMPARE(host.agent->windowAttribute(acrylic), reject ? QVariant() : QVariant(true));
        QCOMPARE(context->appliedMargins, reject ? custom : extended);
        QVERIFY(context->sets > 0);
    }

    void reentrant_data() {
        QTest::addColumn<QString>("stage");
        QTest::addColumn<QString>("action");
        QTest::addColumn<bool>("legacy");
        for (bool legacy : {false, true})
        for (const auto stage : {"backdrop", "margins"})
            for (const auto action : {"write", "related-write", "recreate", "delete"})
                QTest::newRow(qPrintable(QString("%1-%2-%3").arg(legacy).arg(stage, action))) << QString(stage) << QString(action) << legacy;
    }
    void reentrant() {
        QFETCH(QString, stage);
        QFETCH(QString, action);
        QFETCH(bool, legacy);
        legacyMode = legacy;
        Host host;
        QVERIFY(host.agent->setWindowAttribute(extra, QVariant::fromValue(custom)));
        QPointer<Context> alive(context);
        bool invoked = false, innerAccepted = false;
        context->callback = [&](const QString &current) {
            if (invoked || current != stage)
                return;
            invoked = true;
            if (action == "write")
                innerAccepted = host.agent->setWindowAttribute(acrylic, false);
            else if (action == "related-write") {
                // Exercise the shared material revision even when the outer call
                // uses ACCENT_POLICY and the inner one uses the modern backdrop.
                context->legacy = false;
                innerAccepted = host.agent->setWindowAttribute("mica", true);
                context->legacy = legacy;
            }
            else if (action == "recreate")
                host.recreate();
            else
                host.agent.reset();
        };
        // Use the context entry directly: the callback may delete the public agent.
        QVERIFY(!alive->setWindowAttribute(acrylic, true));
        QVERIFY(invoked);
        if (action == "delete") {
            QVERIFY(!alive);
        } else if (action == "write") {
            QVERIFY(innerAccepted);
            QCOMPARE(host.agent->windowAttribute(acrylic), QVariant(false));
            QCOMPARE(context->backdrop, automatic);
            QCOMPARE(context->appliedMargins, custom);
        } else if (action == "related-write") {
            QVERIFY(innerAccepted);
            QVERIFY(!host.agent->windowAttribute(acrylic).isValid());
            QCOMPARE(host.agent->windowAttribute("mica"), QVariant(true));
            QCOMPARE(context->backdrop, mica);
            QCOMPARE(context->appliedMargins, extended);
        } else {
            QVERIFY(alive);
            QVERIFY(!host.agent->windowAttribute(acrylic).isValid());
            QCOMPARE(context->appliedMargins, custom);
        }
    }

    void nativeWindows_data() {
        QTest::addColumn<bool>("quick");
        QTest::addColumn<bool>("legacy");
        QTest::newRow("widgets") << false << false;
        QTest::newRow("widgets-legacy-api") << false << true;
#ifdef TEST_QUICK
        QTest::newRow("quick") << true << false;
        QTest::newRow("quick-legacy-api") << true << true;
#endif
    }
    void nativeWindows() {
        QFETCH(bool, quick);
        QFETCH(bool, legacy);
        nativeMode = true;
        legacyMode = legacy;
        Host host(quick);
        QVERIFY(host.agent->setWindowAttribute(extra, QVariant::fromValue(custom)));
        if (legacy || !context->realSupport()) {
            // Forced legacy entry on a newer OS is not Windows 11 21H2 acceptance.
            QVERIFY(context->realLegacySupport());
            for (int i = 0; i < 2; ++i) {
                QVERIFY(host.agent->setWindowAttribute(acrylic, true));
                // No private getter requirement: assert the payload accepted by the
                // actual setter, not a fabricated policy readback or visual result.
                QCOMPARE(context->policy.dwAccentState, DWORD(ACCENT_ENABLE_ACRYLICBLURBEHIND));
                QCOMPARE(context->appliedMargins, extended);
                host.recreate();
                QCOMPARE(context->policy.dwAccentState, DWORD(ACCENT_ENABLE_ACRYLICBLURBEHIND));
                QVERIFY(host.agent->setWindowAttribute(acrylic, false));
                QCOMPARE(context->policy.dwAccentState, DWORD(ACCENT_DISABLED));
                QCOMPARE(context->appliedMargins, custom);
            }
            return;
        }
        for (const auto effect : {QString("mica"), QString("mica-alt")}) {
            QVERIFY(host.agent->setWindowAttribute(effect, true));
            int actual = -1;
            QVERIFY(SUCCEEDED(context->realQuery(&actual)));
            QCOMPARE(actual, effect == "mica" ? mica : tabbed);
            QVERIFY(host.agent->setWindowAttribute(acrylic, true));
            QVERIFY(SUCCEEDED(context->realQuery(&actual)));
            QCOMPARE(actual, transient);
            QCOMPARE(host.agent->windowAttribute(acrylic), QVariant(true));
            QCOMPARE(context->appliedMargins, extended);
            host.recreate();
            QVERIFY(SUCCEEDED(context->realQuery(&actual)));
            QCOMPARE(actual, transient);
            QVERIFY(host.agent->setWindowAttribute(acrylic, false));
            QVERIFY(SUCCEEDED(context->realQuery(&actual)));
            QCOMPARE(actual, automatic);
            QCOMPARE(context->appliedMargins, custom);
            QVERIFY(host.agent->setWindowAttribute(effect, QVariant()));
            QVERIFY(host.agent->setWindowAttribute(acrylic, QVariant()));
        }
    }
private:
    WindowAgentBasePrivate::WindowContextFactoryMethod previousFactory = nullptr;
};

QTEST_MAIN(AcrylicTest)
#include "tst_acrylic.moc"
