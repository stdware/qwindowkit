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
#  include <QtQuick/QQuickWindow>
#endif
#include "win32windowcontext_p.h"
#include "qwkglobal_p.h"

using namespace QWK;

namespace {
    class Context : public Win32WindowContext {
    public:
        QMargins appliedMargins;
        int applications = 0;
        int creations = 0;
        bool rejectMargins = false;
        std::function<void(const QString &)> onChange;
        QVariant windowAttribute(const QString &key) const override {
            // Exercise the real decoration and native attribute implementation on
            // Windows 11 too. This is NOT Windows 10 DWM seam/appearance acceptance.
            if (key == QStringLiteral("win10-border-needed"))
                return true;
            return Win32WindowContext::windowAttribute(key);
        }
    protected:
        void winIdChanged(WId id, WId old) override {
            if (id)
                ++creations;
            Win32WindowContext::winIdChanged(id, old);
        }
        bool extendFrameMargins(const QMargins &margins) override {
            if (rejectMargins)
                return false;
            appliedMargins = margins;
            ++applications;
            return Win32WindowContext::extendFrameMargins(margins);
        }
        bool windowAttributeChanged(const QString &key, const QVariant &value,
                                    const QVariant &old) override {
            const auto callback = onChange;
            const bool accepted = Win32WindowContext::windowAttributeChanged(key, value, old);
            if (callback)
                callback(key);
            return accepted;
        }
    };
    QMap<QString, QVariant> preload;
    Context *context = nullptr;
    AbstractWindowContext *createContext() {
        context = new Context;
        for (auto it = preload.cbegin(); it != preload.cend(); ++it)
            context->setWindowAttribute(it.key(), it.value());
        return context;
    }
    class Widget : public QWidget {
    public:
        void recreate() { destroy(); create(); }
    };
    struct Host {
        explicit Host(bool quick) {
#ifdef TEST_QUICK
            if (quick) {
                window = std::make_unique<QQuickWindow>();
                window->resize(240, 120);
                return;
            }
#else
            Q_UNUSED(quick)
#endif
            widget = std::make_unique<Widget>();
            widget->resize(240, 120);
        }
        void create() {
            if (widget)
                widget->winId();
#ifdef TEST_QUICK
            else
                window->create();
#endif
        }
        bool setup() {
            if (widget) {
                auto a = std::make_unique<WidgetWindowAgent>();
                const bool result = a->setup(widget.get());
                agent = std::move(a);
                return result;
            }
#ifdef TEST_QUICK
            auto a = std::make_unique<QuickWindowAgent>();
            const bool result = a->setup(window.get());
            agent = std::move(a);
            return result;
#else
            return false;
#endif
        }
        void recreate() {
            if (widget)
                widget->recreate();
#ifdef TEST_QUICK
            else { window->destroy(); window->create(); }
#endif
        }
        void closeAndShow() {
            if (widget) { widget->close(); widget->show(); }
#ifdef TEST_QUICK
            else { window->close(); window->show(); }
#endif
            QCoreApplication::processEvents();
        }
        void changeFlags() {
            if (widget) {
                widget->setWindowFlag(Qt::WindowStaysOnTopHint, true);
                widget->show();
            }
#ifdef TEST_QUICK
            else {
                window->setFlag(Qt::WindowStaysOnTopHint, true);
                window->show();
            }
#endif
            QCoreApplication::processEvents();
        }
        std::unique_ptr<Widget> widget;
#ifdef TEST_QUICK
        std::unique_ptr<QQuickWindow> window;
#endif
        std::unique_ptr<WindowAgentBase> agent;
    };
    void activate(bool active) {
        MSG message{};
        message.message = WM_ACTIVATE;
        message.wParam = active ? WA_ACTIVE : WA_INACTIVE;
        QT_NATIVE_EVENT_RESULT_TYPE result = 0;
        context->nativeDispatch({}, &message, &result);
    }
}

class BorderAttributesTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void init() {
        previousFactory = WindowAgentBasePrivate::windowContextFactoryMethod;
        WindowAgentBasePrivate::windowContextFactoryMethod = createContext;
        preload.clear();
    }
    void cleanup() {
        WindowAgentBasePrivate::windowContextFactoryMethod = previousFactory;
    }
    void configurations_data() {
        QTest::addColumn<bool>("quick");
        QTest::addColumn<int>("dark");
        QTest::addColumn<bool>("custom");
        QTest::addColumn<int>("stage");
        for (bool quick : {
                 false,
#ifdef TEST_QUICK
                 true,
#endif
             }) {
            for (int dark : {-1, 0, 1}) {
                for (bool custom : {false, true}) {
                    for (int stage : {0, 1, 2}) {
                        const auto name = QString("%1-dark%2-custom%3-stage%4")
                            .arg(quick ? "quick" : "widget").arg(dark).arg(custom).arg(stage).toLatin1();
                        QTest::newRow(name.constData()) << quick << dark << custom << stage;
                    }
                }
            }
        }
    }
    void configurations() {
        QFETCH(bool, quick);
        QFETCH(int, dark);
        QFETCH(bool, custom);
        QFETCH(int, stage);
        const QMargins margins = custom ? QMargins(3, 7, 5, 9) : QMargins(0, 1, 0, 0);
        QMap<QString, QVariant> configured;
        if (dark >= 0)
            configured.insert("dark-mode", bool(dark));
        if (custom)
            configured.insert("extra-margins", QVariant::fromValue(margins));
        // 0: attributes cached before context setup; 1: application sets them
        // after setup, before the first surface; 2: decoration attaches to a surface.
        if (stage != 1)
            preload = configured;
        Host host(quick);
        if (stage == 2)
            host.create();
        QVERIFY(host.setup());
        if (stage != 2)
            QCOMPARE(context->windowId(), WId(0));
        if (stage == 1) {
            for (auto it = configured.cbegin(); it != configured.cend(); ++it)
                QVERIFY(host.agent->setWindowAttribute(it.key(), it.value()));
        }
        host.create();
        QVERIFY(context->windowId());
        auto check = [&] {
            QCOMPARE(host.agent->windowAttribute("dark-mode"), QVariant(dark != 0));
            QCOMPARE(host.agent->windowAttribute("extra-margins").value<QMargins>(), margins);
            // Query the real current HWND's DWM dark-mode setting, not the cache.
            using GetAttribute = HRESULT (WINAPI *)(HWND, DWORD, PVOID, DWORD);
            const auto getAttribute = reinterpret_cast<GetAttribute>(
                ::GetProcAddress(::GetModuleHandleW(L"dwmapi.dll"), "DwmGetWindowAttribute"));
            QVERIFY(getAttribute);
            BOOL enabled = FALSE;
            QVERIFY(SUCCEEDED(getAttribute(reinterpret_cast<HWND>(context->windowId()),
                isWin1020H1OrGreater() ? 20 : 19, &enabled, sizeof(enabled))));
            QCOMPARE(bool(enabled), dark != 0);
            const int before = context->applications;
            activate(false);
            QVERIFY(context->applications > before);
            const int top = -context->windowAttribute("window-rect").toRect().top();
            QCOMPARE(context->appliedMargins, QMargins(margins.left(), qMax(margins.top(), top),
                                                       margins.right(), margins.bottom()));
            QCOMPARE(host.agent->windowAttribute("extra-margins").value<QMargins>(), margins);
            activate(true);
            QCOMPARE(context->appliedMargins, margins);
        };
        check();
        for (int i = 0; i < 2; ++i) {
            activate(false);
            const int creations = context->creations;
            host.recreate();
            QVERIFY(context->creations > creations);
            check();
        }
        host.closeAndShow();
        check();
        host.changeFlags();
        check();
    }
    void updatesWhileInactive_data() {
        QTest::addColumn<bool>("quick");
        QTest::newRow("widget") << false;
#ifdef TEST_QUICK
        QTest::newRow("quick") << true;
#endif
    }
    void updatesWhileInactive() {
        QFETCH(bool, quick);
        Host host(quick);
        QVERIFY(host.setup());
        host.create();
        activate(false);
        for (const QMargins &margins : {QMargins(2, 256, 4, 6), QMargins(-1, -1, -1, -1),
                                        QMargins(2, 3, 4, 6)}) {
            QVERIFY(host.agent->setWindowAttribute("extra-margins", QVariant::fromValue(margins)));
            QCOMPARE(host.agent->windowAttribute("extra-margins").value<QMargins>(), margins);
            const int top = -context->windowAttribute("window-rect").toRect().top();
            QCOMPARE(context->appliedMargins, margins.left() < 0 ? margins :
                QMargins(2, qMax(margins.top(), top), 4, 6));
            activate(true);
            QCOMPARE(context->appliedMargins, margins);
            activate(false);
        }
        // A successful inner update during default initialization wins. This also
        // covers replay: later defaults must not reinsert an obsolete outer value.
        QVERIFY(host.agent->setWindowAttribute("dark-mode", {}));
        context->onChange = [](const QString &key) {
            if (key == QStringLiteral("dark-mode")) {
                context->onChange = {};
                QVERIFY(context->setWindowAttribute(key, false));
            }
        };
        host.recreate();
        QCOMPARE(host.agent->windowAttribute("dark-mode"), QVariant(false));
        host.recreate();
        QCOMPARE(host.agent->windowAttribute("dark-mode"), QVariant(false));
        QVERIFY(host.agent->setWindowAttribute("extra-margins", {}));
        const QMargins innerMargins(4, 8, 12, 16);
        context->onChange = [innerMargins](const QString &key) {
            if (key == QStringLiteral("extra-margins")) {
                context->onChange = {};
                QVERIFY(context->setWindowAttribute(key, QVariant::fromValue(innerMargins)));
            }
        };
        host.recreate();
        QCOMPARE(host.agent->windowAttribute("extra-margins").value<QMargins>(), innerMargins);
        activate(true);
        QCOMPARE(context->appliedMargins, innerMargins);
        const auto cached = host.agent->windowAttribute("extra-margins");
        const auto applied = context->appliedMargins;
        context->rejectMargins = true;
        QVERIFY(!host.agent->setWindowAttribute("extra-margins", QVariant::fromValue(QMargins(9, 9, 9, 9))));
        QCOMPARE(host.agent->windowAttribute("extra-margins"), cached);
        QCOMPARE(context->appliedMargins, applied);
    }
    void destructionDuringDefaults_data() { updatesWhileInactive_data(); }
    void destructionDuringDefaults() {
        QFETCH(bool, quick);
        Host host(quick);
        QVERIFY(host.setup());
        host.create();
        QVERIFY(host.agent->setWindowAttribute("dark-mode", {}));
        const QPointer<Context> guard(context);
        context->onChange = [&](const QString &key) {
            if (key == QStringLiteral("dark-mode"))
                host.agent.reset();
        };
        host.recreate();
        QVERIFY(guard.isNull());
        QVERIFY(!host.agent);
    }
private:
    WindowAgentBasePrivate::WindowContextFactoryMethod previousFactory = nullptr;
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    BorderAttributesTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_borderattributes.moc"
