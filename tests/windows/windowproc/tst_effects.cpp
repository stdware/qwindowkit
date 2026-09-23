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
    const QMargins custom(3, 7, 5, 9), extended(65536, 0, 0, 0);
    const QStringList effects{"mica", "mica-alt", "legacy-mica", "dwm-blur", "dark-mode", "dwm-border-color"};
    QString keyFor(const QString &effect) { return effect == "legacy-mica" ? QString("mica") : effect; }
    bool hasMargins(const QString &effect) { return effect != "dark-mode" && effect != "dwm-border-color"; }
    QVariant valueFor(const QString &effect, bool enable) {
        return effect == "dwm-border-color" ? QVariant(QColor(enable ? "#224466" : "#6688aa")) : QVariant(enable);
    }
    DWORD idFor(const QString &effect) {
        if (effect == "legacy-mica") return 1029;
        if (effect == "dark-mode") return 20;
        if (effect == "dwm-border-color") return 34;
        if (effect == "dwm-blur") return 0; // Controlled blur state, not a DWM attribute.
        return 38;
    }
    DWORD expectedValue(const QString &effect, bool enable) {
        if (effect == "dwm-border-color") return enable ? RGB(0x22, 0x44, 0x66) : RGB(0x66, 0x88, 0xaa);
        if (effect == "mica") return enable ? 2 : 0;
        if (effect == "mica-alt") return enable ? 4 : 0;
        return enable;
    }
    class Context : public Win32WindowContext {
    public:
        bool native = false, legacy = false, supported = true;
        bool rejectSet = false, rejectMargins = false, rejectRollback = false;
        int sets = 0, marginCalls = 0;
        QMap<DWORD, DWORD> values;
        QMargins margins;
        ACCENT_POLICY policy{};
        std::function<void(const QString &)> callback;
        HRESULT read(DWORD id, DWORD *value) const {
            return DynamicApis::instance().pDwmGetWindowAttribute(
                reinterpret_cast<HWND>(windowId()), id, value, sizeof(*value));
        }
        void reset() { sets = marginCalls = 0; }
    protected:
        bool supportsSystemBackdrop() const override {
            return !legacy && (native ? Win32WindowContext::supportsSystemBackdrop() : supported);
        }
        bool supportsLegacyMica() const override { return supported; }
        HRESULT querySystemBackdrop(int *value) const override {
            if (native) return Win32WindowContext::querySystemBackdrop(value);
            *value = int(values.value(38));
            return S_OK;
        }
        HRESULT setWindowDwmAttribute(DWORD id, const void *value, DWORD size) override {
            ++sets;
            const auto result = native ? Win32WindowContext::setWindowDwmAttribute(id, value, size)
                                       : rejectSet ? E_FAIL : S_OK;
            if (SUCCEEDED(result)) {
                if (size != sizeof(DWORD)) qFatal("Unexpected DWM payload size");
                DWORD copy;
                memcpy(&copy, value, sizeof(copy));
                values[id] = copy;
            }
            const auto action = callback;
            if (action) action("set");
            return result;
        }
        bool setAccentPolicy(const ACCENT_POLICY &value) override {
            const bool accepted = native ? Win32WindowContext::setAccentPolicy(value) : !rejectSet;
            if (accepted) policy = value;
            return accepted;
        }
        bool setBlurBehind(bool enable) override {
            ++sets;
            const bool accepted = native ? Win32WindowContext::setBlurBehind(enable) : !rejectSet;
            if (accepted) values[0] = enable;
            const auto action = callback;
            if (action) action("set");
            return accepted;
        }
        bool extendFrameMargins(const QMargins &value) override {
            ++marginCalls;
            const bool accepted = native ? Win32WindowContext::extendFrameMargins(value)
                : !rejectMargins && !(rejectRollback && marginCalls == 2);
            if (accepted) margins = value;
            const auto action = callback;
            if (action) action("margins");
            return accepted;
        }
    };
    Context *context = nullptr;
    bool nativeMode = false;
    AbstractWindowContext *createContext() {
        context = new Context;
        context->native = nativeMode;
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
                if (!a->setup(window.get())) qFatal("Quick agent setup failed");
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
                if (!a->setup(widget.get())) qFatal("Widget agent setup failed");
                agent = std::move(a);
                widget->show();
            }
            QCoreApplication::processEvents();
            if (!::IsWindow(reinterpret_cast<HWND>(context->windowId()))) qFatal("Expected a native HWND");
        }
        void recreate() {
            if (widget) widget->recreate();
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

class EffectsTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void init() {
        previousFactory = WindowAgentBasePrivate::windowContextFactoryMethod;
        WindowAgentBasePrivate::windowContextFactoryMethod = createContext;
        nativeMode = false;
    }
    void cleanup() { WindowAgentBasePrivate::windowContextFactoryMethod = previousFactory; }
    void failures_data() {
        QTest::addColumn<QString>("effect");
        QTest::addColumn<bool>("initial");
        QTest::addColumn<int>("operation");
        QTest::addColumn<QString>("failure");
        for (const auto &effect : effects)
            for (bool initial : {false, true})
                for (int op = 0; op < 3; ++op)
                    for (const QString failure : {"none", "set", "margins", "rollback"}) {
                        if (!hasMargins(effect) && (failure == "margins" || failure == "rollback")) continue;
                        if (effect == "dwm-border-color" && op == 2) continue; // Color removal is unsupported.
                        QTest::newRow(qPrintable(QString("%1-%2-%3-%4").arg(effect).arg(initial).arg(op).arg(failure)))
                            << effect << initial << op << failure;
                    }
    }
    void failures() {
        QFETCH(QString, effect); QFETCH(bool, initial); QFETCH(int, operation); QFETCH(QString, failure);
        Host host;
        context->legacy = effect == "legacy-mica";
        const auto key = keyFor(effect);
        QVERIFY(host.agent->setWindowAttribute("extra-margins", QVariant::fromValue(custom)));
        QVERIFY(host.agent->setWindowAttribute(key, valueFor(effect, initial)));
        // The current successful margins may differ from the effect's cached value.
        const QMargins previous(-1, -1, -1, -1);
        QVERIFY(host.agent->setWindowAttribute("extra-margins", QVariant::fromValue(previous)));
        const auto before = context->values;
        context->reset();
        context->rejectSet = failure == "set" || failure == "rollback";
        context->rejectMargins = failure == "margins";
        context->rejectRollback = failure == "rollback";
        if (failure == "rollback")
            QTest::ignoreMessage(QtWarningMsg, "QWindowKit: Effect margins rollback failed; retry the effect update.");
        const QVariant request = operation == 2 ? QVariant() : valueFor(effect, operation == 1);
        QCOMPARE(host.agent->setWindowAttribute(key, request), failure == "none");
        QCOMPARE(host.agent->windowAttribute(key), failure == "none" ? request : valueFor(effect, initial));
        const auto requestedMargins = effect != "dwm-blur" && operation == 1 ? extended : previous;
        QCOMPARE(context->margins, hasMargins(effect) && (failure == "none" || failure == "rollback")
            ? requestedMargins : previous);
        if (failure == "none")
            QCOMPARE(context->values.value(idFor(effect)), expectedValue(effect, operation == 1));
        else
            QCOMPARE(context->values, before);
        QCOMPARE(context->sets, failure == "margins" ? 0 : 1);
        if (hasMargins(effect))
            QCOMPARE(context->marginCalls, failure == "set" || failure == "rollback" ? 2 : 1);
        if (failure != "none") {
            context->rejectSet = context->rejectMargins = context->rejectRollback = false;
            QVERIFY(host.agent->setWindowAttribute(key, request));
            QCOMPARE(host.agent->windowAttribute(key), request);
        }
    }
    void replay_data() {
        QTest::addColumn<QString>("effect"); QTest::addColumn<bool>("reject");
        for (const auto &effect : effects)
            for (bool reject : {false, true})
                QTest::newRow(qPrintable(effect + QString::number(reject))) << effect << reject;
    }
    void replay() {
        QFETCH(QString, effect); QFETCH(bool, reject);
        Host host;
        context->legacy = effect == "legacy-mica";
        const auto key = keyFor(effect);
        QVERIFY(host.agent->setWindowAttribute("extra-margins", QVariant::fromValue(custom)));
        QVERIFY(host.agent->setWindowAttribute(key, valueFor(effect, true)));
        context->rejectSet = reject;
        context->reset();
        host.recreate();
        QVERIFY(context->windowId());
        QCOMPARE(host.agent->windowAttribute(key), reject ? QVariant() : valueFor(effect, true));
        QVERIFY(context->sets > 0);
        QCOMPARE(context->margins, !reject && hasMargins(effect) && effect != "dwm-blur" ? extended : custom);
    }
    void reentrant_data() {
        QTest::addColumn<QString>("effect"); QTest::addColumn<QString>("stage"); QTest::addColumn<QString>("action");
        for (const auto &effect : effects)
            for (const QString stage : {"set", "margins"})
                for (const QString action : {"write", "recreate", "delete"}) {
                    if (stage == "margins" && !hasMargins(effect)) continue;
                    QTest::newRow(qPrintable(effect + stage + action)) << effect << stage << action;
                }
    }
    void reentrant() {
        QFETCH(QString, effect); QFETCH(QString, stage); QFETCH(QString, action);
        Host host;
        context->legacy = effect == "legacy-mica";
        QVERIFY(host.agent->setWindowAttribute("extra-margins", QVariant::fromValue(custom)));
        QPointer<Context> alive(context);
        const auto key = keyFor(effect);
        bool invoked = false, accepted = false;
        context->callback = [&](const QString &current) {
            if (invoked || current != stage) return;
            invoked = true;
            if (action == "write") accepted = host.agent->setWindowAttribute(key, valueFor(effect, false));
            else if (action == "recreate") host.recreate();
            else host.agent.reset();
        };
        QVERIFY(!alive->setWindowAttribute(key, valueFor(effect, true)));
        QVERIFY(invoked);
        if (action == "delete") { QVERIFY(!alive); return; }
        if (action == "write") {
            QVERIFY(accepted);
            QCOMPARE(host.agent->windowAttribute(key), valueFor(effect, false));
            QCOMPARE(context->values.value(idFor(effect)), expectedValue(effect, false));
        } else if (key != "dark-mode") { // The border handler can reinstall its dark-mode default.
            QVERIFY(!host.agent->windowAttribute(key).isValid());
        }
        QCOMPARE(context->margins, custom);
    }
    void relatedWrites_data() {
        QTest::addColumn<QString>("outer"); QTest::addColumn<QString>("stage"); QTest::addColumn<QString>("inner");
        for (const QString outer : {"mica", "mica-alt", "dwm-blur", "acrylic-material"})
            for (const QString stage : {"set", "margins"})
                for (const QString inner : {"mica", "mica-alt", "acrylic-material"})
                    if (outer != inner)
                        QTest::newRow(qPrintable(outer + stage + inner)) << outer << stage << inner;
    }
    void relatedWrites() {
        QFETCH(QString, outer); QFETCH(QString, stage); QFETCH(QString, inner);
        Host host;
        QVERIFY(host.agent->setWindowAttribute("extra-margins", QVariant::fromValue(custom)));
        bool invoked = false, accepted = false;
        context->callback = [&](const QString &current) {
            if (invoked || current != stage) return;
            invoked = true;
            accepted = host.agent->setWindowAttribute(inner, true);
        };
        QVERIFY(!host.agent->setWindowAttribute(outer, true));
        QVERIFY(invoked); QVERIFY(accepted);
        QCOMPARE(host.agent->windowAttribute(inner), QVariant(true));
        QVERIFY(!host.agent->windowAttribute(outer).isValid());
        QCOMPARE(context->values.value(38), DWORD(inner == "mica" ? 2 : inner == "mica-alt" ? 4 : 3));
        QCOMPARE(context->margins, extended);
    }
    void failurePreservesInnerMargins_data() {
        QTest::addColumn<QString>("effect");
        for (const QString effect : {"mica", "mica-alt", "legacy-mica", "dwm-blur"})
            QTest::newRow(qPrintable(effect)) << effect;
    }
    void failurePreservesInnerMargins() {
        QFETCH(QString, effect);
        Host host;
        context->legacy = effect == "legacy-mica";
        const QMargins inner(11, 12, 13, 14);
        context->rejectSet = true;
        bool invoked = false, accepted = false;
        context->callback = [&](const QString &stage) {
            if (invoked || stage != "set") return;
            invoked = true;
            accepted = host.agent->setWindowAttribute("extra-margins", QVariant::fromValue(inner));
        };
        QVERIFY(!host.agent->setWindowAttribute(keyFor(effect), true));
        QVERIFY(invoked); QVERIFY(accepted);
        QCOMPARE(context->margins, inner);
        QCOMPARE(host.agent->windowAttribute("extra-margins").value<QMargins>(), inner);
    }
    void unsupported_data() {
        QTest::addColumn<QString>("effect");
        for (const QString effect : {"mica", "mica-alt", "legacy-mica"})
            QTest::newRow(qPrintable(effect)) << effect;
    }
    void unsupported() {
        QFETCH(QString, effect);
        Host host;
        context->legacy = effect == "legacy-mica";
        context->supported = false;
        context->reset();
        QVERIFY(!host.agent->setWindowAttribute(keyFor(effect), true));
        QVERIFY(!host.agent->windowAttribute(keyFor(effect)).isValid());
        QCOMPARE(context->sets, 0);
        QCOMPARE(context->marginCalls, 0);
    }
    void firstFailure_data() {
        QTest::addColumn<QString>("effect");
        for (const auto &effect : effects) QTest::newRow(qPrintable(effect)) << effect;
    }
    void firstFailure() {
        QFETCH(QString, effect);
        Host host;
        context->legacy = effect == "legacy-mica";
        const auto key = keyFor(effect);
        // Remove the border handler's optional default before asserting an absent cache.
        if (host.agent->windowAttribute(key).isValid())
            QVERIFY(host.agent->setWindowAttribute(key, QVariant()));
        context->reset();
        context->rejectSet = true;
        QVERIFY(!host.agent->setWindowAttribute(key, valueFor(effect, true)));
        QVERIFY(!host.agent->windowAttribute(key).isValid());
        QCOMPARE(context->sets, 1);
    }
    void failedInnerRequest_data() {
        QTest::addColumn<QString>("outer"); QTest::addColumn<QString>("stage"); QTest::addColumn<QString>("failure");
        for (const QString outer : {"mica", "acrylic-material"})
            for (const QString stage : {"set", "margins"})
                for (const QString failure : {"set", "margins", "rollback"})
                    QTest::newRow(qPrintable(outer + stage + failure)) << outer << stage << failure;
    }
    void failedInnerRequest() {
        QFETCH(QString, outer); QFETCH(QString, stage); QFETCH(QString, failure);
        Host host;
        QVERIFY(host.agent->setWindowAttribute("extra-margins", QVariant::fromValue(custom)));
        bool invoked = false, accepted = true;
        context->callback = [&](const QString &current) {
            if (invoked || current != stage) return;
            invoked = true;
            context->reset();
            context->rejectSet = failure != "margins";
            context->rejectMargins = failure == "margins";
            context->rejectRollback = failure == "rollback";
            if (failure == "rollback")
                QTest::ignoreMessage(QtWarningMsg, "QWindowKit: Effect margins rollback failed; retry the effect update.");
            accepted = host.agent->setWindowAttribute("mica-alt", true);
            context->rejectSet = context->rejectMargins = context->rejectRollback = false;
        };
        const bool completes = failure != "rollback" && (stage == "set" || failure == "margins");
        QCOMPARE(host.agent->setWindowAttribute(outer, true), completes);
        QVERIFY(invoked); QVERIFY(!accepted);
        QCOMPARE(host.agent->windowAttribute(outer), completes ? QVariant(true) : QVariant());
        QVERIFY(!host.agent->windowAttribute("mica-alt").isValid());
        // Failed inner calls remain eligible when restored. A failed restoration
        // invalidates the outer; a newer margin write can also interrupt its API call.
        const bool nativeEffect = completes || (failure == "rollback" &&
            (stage == "set" || outer == "acrylic-material"));
        QCOMPARE(context->values.value(38), DWORD(nativeEffect ? outer == "mica" ? 2 : 3 : 0));
        context->callback = {};
        context->rejectSet = context->rejectMargins = context->rejectRollback = false;
        QVERIFY(host.agent->setWindowAttribute(outer, false)); // Explicit retry repairs state.
        QCOMPARE(context->values.value(38), DWORD(0));
        QCOMPARE(context->margins, custom);
    }
    void nativeWindows_data() {
        QTest::addColumn<bool>("quick");
        QTest::newRow("widgets") << false;
#ifdef TEST_QUICK
        QTest::newRow("quick") << true;
#endif
    }
    void nativeWindows() {
        QFETCH(bool, quick);
        nativeMode = true;
        Host host(quick);
        QVERIFY(host.agent->setWindowAttribute("extra-margins", QVariant::fromValue(custom)));
        for (const QString effect : {"mica", "mica-alt", "dwm-blur", "dark-mode", "dwm-border-color"}) {
            for (bool enable : {true, false}) {
                QVERIFY(host.agent->setWindowAttribute(effect, valueFor(effect, enable)));
                DWORD actual = 0;
                if (effect != "dwm-blur" && effect != "dwm-border-color") {
                    const HRESULT result = context->read(idFor(effect), &actual);
                    QVERIFY2(SUCCEEDED(result), qPrintable(QString("%1 read failed: 0x%2")
                        .arg(effect).arg(quint32(result), 8, 16, QLatin1Char('0'))));
                    QCOMPARE(actual, expectedValue(effect, enable));
                } else if (effect == "dwm-blur") {
                    QCOMPARE(context->policy.dwAccentState, DWORD(enable ? ACCENT_ENABLE_BLURBEHIND : ACCENT_DISABLED));
                } else {
                    // The local native getter rejects border-color reads with
                    // E_INVALIDARG; assert the payload accepted by the real setter.
                    QCOMPARE(context->values.value(34), expectedValue(effect, enable));
                }
                host.recreate();
                QCOMPARE(host.agent->windowAttribute(effect), valueFor(effect, enable));
                if (effect != "dwm-blur" && effect != "dwm-border-color") {
                    const HRESULT result = context->read(idFor(effect), &actual);
                    QVERIFY2(SUCCEEDED(result), qPrintable(QString("%1 replay read failed: 0x%2")
                        .arg(effect).arg(quint32(result), 8, 16, QLatin1Char('0'))));
                    QCOMPARE(actual, expectedValue(effect, enable));
                } else if (effect == "dwm-border-color") {
                    QCOMPARE(context->values.value(34), expectedValue(effect, enable));
                } else {
                    QCOMPARE(context->policy.dwAccentState, DWORD(enable ? ACCENT_ENABLE_BLURBEHIND : ACCENT_DISABLED));
                }
                QCOMPARE(context->margins, hasMargins(effect) && effect != "dwm-blur" && enable ? extended : custom);
            }
        }
        for (const QString effect : {"mica", "mica-alt", "acrylic-material", "mica"}) {
            QVERIFY(host.agent->setWindowAttribute(effect, true));
            DWORD actual = 0;
            QVERIFY(SUCCEEDED(context->read(38, &actual)));
            QCOMPARE(actual, DWORD(effect == "mica" ? 2 : effect == "mica-alt" ? 4 : 3));
            QCOMPARE(context->margins, extended);
        }
    }
private:
    WindowAgentBasePrivate::WindowContextFactoryMethod previousFactory = nullptr;
};

QTEST_MAIN(EffectsTest)
#include "tst_effects.moc"
