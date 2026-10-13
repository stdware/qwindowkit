// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <functional>
#include <memory>
#include <new>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtGui/QCloseEvent>
#include <QtTest/QTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QWKCore/private/windowagentbase_p.h>
#include <QWKWidgets/widgetwindowagent.h>
#include "win32windowcontext_p.h"
#include "../../unit/windowcontextfixture.h"

namespace {
    bool childProcess = false;
    QList<void *> retiredPages;

    // Keep the address reserved after destruction. Any stale read/write faults in
    // the isolated child, independent of heap reuse or optional sanitizer tooling.
    void *allocatePage(size_t size) {
        auto page = ::VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!page)
            throw std::bad_alloc();
        return page;
    }
    void retirePage(void *page) {
        DWORD old = 0;
        if (!::VirtualProtect(page, 1, PAGE_NOACCESS, &old))
            qFatal("Cannot protect retired test allocation");
        retiredPages.append(page);
    }

    class Context : public QWK::Win32WindowContext {
    public:
        static void *operator new(size_t size) { return allocatePage(size); }
        static void operator delete(void *page) { retirePage(page); }
        bool iconStateClear() const { return !iconButtonClickTime && !iconButtonClickLevel; }
        void forgetRecordedId() { m_windowId = 0; }
    };
    class SurfaceFilter : public QWK::WindowWinIdChangeEventFilter {
    public:
        using WindowWinIdChangeEventFilter::WindowWinIdChangeEventFilter;
        static void *operator new(size_t size) { return allocatePage(size); }
        static void operator delete(void *page) { retirePage(page); }
    };
    class WindowDelegate : public QwkTest::Delegate {
    public:
        using Delegate::Delegate;
        QWK::WinIdChangeEventFilter *createWinIdEventFilter(
            QObject *host, QWK::AbstractWindowContext *context) const override {
            return new SurfaceFilter(static_cast<QWindow *>(host), context);
        }
    };
    class Observer : public QWK::SharedEventFilter {
    public:
        std::function<void()> action;
        bool sharedEventFilter(QObject *, QEvent *event) override {
            if (event->type() == QEvent::WinIdChange) {
                const auto callback = action;
                callback();
            }
            return false;
        }
    };
    class Agent : public QWK::WidgetWindowAgent {
    public:
        Context *context() const { return static_cast<Context *>(d_ptr->context.get()); }
    };
    class MenuWindow : public QWidget {
    public:
        std::function<void(HWND)> menuAction;
        int closeRequests = 0;
        void recreate() { destroy(); create(); }
    protected:
        bool nativeEvent(const QByteArray &type, void *message,
                         QT_NATIVE_EVENT_RESULT_TYPE *result) override {
            const auto msg = static_cast<MSG *>(message);
            if (msg->message == WM_ENTERIDLE && msg->wParam == MSGF_MENU && menuAction) {
                const auto action = std::exchange(menuAction, {});
                const auto popup = reinterpret_cast<HWND>(msg->lParam);
                action(popup); // May delete this window.
                *result = 0;
                return true;
            }
            Q_UNUSED(type);
            return false;
        }
        void closeEvent(QCloseEvent *event) override {
            ++closeRequests;
            event->ignore();
        }
    };

    class AttributeWindow : public QWindow {
    public:
        std::function<void()> action;
        std::function<bool()> ready;
        int moves = 0;
        UINT trigger = WM_WINDOWPOSCHANGING;
        bool consumeTrigger = false;
    protected:
        bool nativeEvent(const QByteArray &, void *message,
                         QT_NATIVE_EVENT_RESULT_TYPE *result) override {
            const auto msg = static_cast<MSG *>(message);
            if (msg->message == WM_WINDOWPOSCHANGING) {
                ++moves;
            }
            if (msg->message == trigger && action && ready()) {
                const auto callback = std::exchange(action, {});
                callback();
                if (consumeTrigger) {
                    // The old QPA window was destroyed. Do not let Qt continue
                    // delivering this native message to that obsolete receiver.
                    *result = 0;
                    return true;
                }
            }
            return false;
        }
    };
}

class WindowLifetimeTest : public QObject {
    Q_OBJECT
private:
    void runChild() {
        QTemporaryDir reports;
        QVERIFY(reports.isValid());
        const auto path = reports.filePath("child.txt");
        QProcess child;
        child.setProcessChannelMode(QProcess::MergedChannels);
        child.start(QCoreApplication::applicationFilePath(),
                    {"--child", QString("%1:%2").arg(QTest::currentTestFunction(), QTest::currentDataTag()),
                     "-o", path + ",txt"});
        QVERIFY(child.waitForStarted(2000));
        const bool finished = child.waitForFinished(5000);
        if (!finished) {
            child.kill();
            child.waitForFinished(1000);
        }
        QFile report(path);
        const auto output = QByteArray("Child exit: ") + QByteArray::number(child.exitCode()) + '\n' +
            child.readAll() + (report.open(QIODevice::ReadOnly)
            ? report.readAll() : report.errorString().toUtf8());
        QVERIFY2(finished, output.constData());
        QVERIFY2(child.exitStatus() == QProcess::NormalExit && child.exitCode() == 0,
                 output.constData());
    }
private Q_SLOTS:
    void attributes_data() {
        QTest::addColumn<QString>("scenario");
        for (const auto name : {"delete-context", "replace", "remove", "other-keys",
                               "recreate", "replay-delete", "replay-replace", "handle-delete"})
            QTest::newRow(name) << QString::fromLatin1(name);
    }

    void attributes() {
        if (!childProcess) { runChild(); return; }
        QFETCH(QString, scenario);
        AttributeWindow window;
        window.setGeometry(100, 100, 320, 240);
        auto context = std::make_unique<Context>();
        context->setup(&window, new WindowDelegate(&window));
        window.create();
        QVERIFY(context->windowId());
        context->setProperty("_qwk_effectBugWorkaround1", true);
        QVERIFY(context->setWindowAttribute("dwm-blur", false));
        context->setProperty("_qwk_effectBugWorkaround1", false);
        bool visited = false;
        bool nestedResult = false;
        window.ready = [&] {
            return context && (scenario == "handle-delete" ? context->windowId() != 0
                : context->property("_qwk_effectBugWorkaround1").toBool());
        };
        if (scenario == "handle-delete")
            window.trigger = WM_STYLECHANGING;
        window.consumeTrigger = scenario == "recreate";
        window.action = [&] {
            visited = true;
            if (scenario == "delete-context" || scenario == "replay-delete" || scenario == "handle-delete") {
                context.reset();
            } else if (scenario == "recreate") {
                window.destroy();
                window.create();
            } else if (scenario == "other-keys") {
                // Write other supported keys while the outer platform operation is active.
                nestedResult = context->setWindowAttribute("no-system-menu", true) &&
                    context->setWindowAttribute("dark-mode", true) &&
                    context->setWindowAttribute("extra-margins", QVariant::fromValue(QMargins(1, 1, 1, 1)));
            } else {
                nestedResult = context->setWindowAttribute("dwm-blur",
                    scenario == "remove" ? QVariant{} : QVariant(true));
            }
        };
        window.moves = 0;
        RECT before{};
        QVERIFY(::GetWindowRect(reinterpret_cast<HWND>(window.winId()), &before));
        bool accepted = false;
        if (scenario.startsWith("replay-") || scenario == "handle-delete") {
            window.destroy();
            window.create();
        } else {
            accepted = context->setWindowAttribute("dwm-blur", false);
        }
        QVERIFY(visited); // Must be reached through a real synchronous Windows call.
        if (!context) {
            QVERIFY(!accepted);
            if (scenario == "delete-context")
                QCOMPARE(window.moves, 1); // No later movement after deleting the context.
            return;
        }
        if (scenario != "recreate")
            QVERIFY(nestedResult);
        QCOMPARE(accepted, scenario == "other-keys");
        QCOMPARE(context->windowAttribute("dwm-blur"), scenario == "remove" ? QVariant{}
            : scenario == "other-keys" || scenario == "recreate" ? QVariant(false) : QVariant(true));
        if (!scenario.startsWith("replay-") && scenario != "recreate") {
            RECT after{};
            QVERIFY(::GetWindowRect(reinterpret_cast<HWND>(window.winId()), &after));
            QCOMPARE(after.left, before.left);
            QCOMPARE(after.top, before.top);
            QCOMPARE(after.right, before.right);
            QCOMPARE(after.bottom, before.bottom);
        }
        QVERIFY(context->setWindowAttribute("no-system-menu", false));
        QCOMPARE(context->windowId(), window.winId());
    }

    void menus_data() {
        QTest::addColumn<QString>("scenario");
        for (const auto name : {"cancel", "select-close", "double-click", "delete-agent",
                               "delete-window", "recreate-window", "replace-agent", "nested"})
            QTest::newRow(name) << QString::fromLatin1(name);
    }
    void menus() {
        if (!childProcess) { runChild(); return; }
        QFETCH(QString, scenario);
        auto window = std::make_unique<MenuWindow>();
        window->resize(400, 240);
        window->move(150, 150);
        QWidget *icon = new QWidget(window.get());
        icon->setGeometry(20, 10, 30, 30);
        const auto hwnd = reinterpret_cast<HWND>(window->winId());
        auto agent = std::make_unique<Agent>();
        QVERIFY(agent->setup(window.get()));
        agent->setSystemButton(QWK::WindowAgentBase::WindowIcon, icon);
        window->show();
        QVERIFY(QTest::qWaitForWindowExposed(window.get(), 1000));
        QCOMPARE(reinterpret_cast<HWND>(window->winId()), hwnd);
        QCOMPARE(agent->context()->windowId(), reinterpret_cast<WId>(hwnd));
        QVERIFY(::IsWindow(hwnd));
        const auto hit = [&] {
            const QPoint local = icon->mapTo(window.get(), QPoint(15, 15));
            const auto dpr = window->devicePixelRatio();
            POINT point{qRound(local.x() * dpr), qRound(local.y() * dpr)};
            ::ClientToScreen(hwnd, &point);
            const auto pos = MAKELPARAM(point.x, point.y);
            return qMakePair(::SendMessageW(hwnd, WM_NCHITTEST, 0, pos), pos);
        };
        auto target = hit();
        QCOMPARE(target.first, LRESULT(HTSYSMENU));
        if (scenario == "double-click") {
            ::SendMessageW(hwnd, WM_NCLBUTTONDBLCLK, HTSYSMENU, target.second);
            QCoreApplication::processEvents();
            QCOMPARE(window->closeRequests, 1);
            return;
        }

        int visits = 0;
        if (scenario == "select-close") {
            QVERIFY(::ModifyMenuW(::GetSystemMenu(hwnd, FALSE), SC_CLOSE,
                                   MF_BYCOMMAND | MF_STRING, SC_CLOSE, L"&Z"));
        }
        bool replacementReady = false;
        bool nestedVisited = false;
        window->menuAction = [&](HWND) {
            ++visits;
            if (scenario == "select-close") {
                ::PostMessageW(hwnd, WM_CHAR, L'z', 0);
                return;
            }
            if (scenario == "delete-agent") {
                agent.reset();
            } else if (scenario == "replace-agent") {
                agent.reset();
                agent = std::make_unique<Agent>();
                replacementReady = agent->setup(window.get());
                agent->setSystemButton(QWK::WindowAgentBase::WindowIcon, icon);
            } else if (scenario == "delete-window") {
                window.reset();
            } else if (scenario == "recreate-window") {
                window->recreate();
            } else if (scenario == "nested") {
                // A second menu request inside the first must not own/clear its hook.
                ::SendMessageW(hwnd, WM_SYSCOMMAND, SC_KEYMENU, VK_SPACE);
                nestedVisited = true;
            }
            ::EndMenu();
        };
        // Bound the native loop even if the expected idle callback is not delivered.
        const auto timer = ::SetTimer(nullptr, 0, 1500,
            [](HWND, UINT, UINT_PTR, DWORD) { ::EndMenu(); });
        QVERIFY(timer);
        const auto stopTimer = qScopeGuard([timer] { ::KillTimer(nullptr, timer); });
        ::SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTSYSMENU, target.second);
        QCOMPARE(visits, 1);
        QCoreApplication::processEvents();
        if (scenario == "select-close")
            QTRY_COMPARE_WITH_TIMEOUT(window->closeRequests, 1, 500);
        if (scenario == "replace-agent")
            QVERIFY(replacementReady);
        if (scenario == "nested")
            QVERIFY(nestedVisited);
        if (window && agent) {
            QVERIFY(agent->context()->iconStateClear());
            // A later independent menu still works after cleanup/recreation.
            int later = 0;
            window->menuAction = [&](HWND) { ++later; ::EndMenu(); };
            agent->showSystemMenu(window->mapToGlobal(QPoint(30, 30)));
            QCOMPARE(later, 1);
        }
    }

    void surfaces_data() {
        QTest::addColumn<QString>("scenario");
        for (const auto name : {"delete-context", "delete-receiver", "nested", "native-recreate",
                               "widget-receiver"})
            QTest::newRow(name) << QString::fromLatin1(name);
    }
    void surfaces() {
        if (!childProcess) { runChild(); return; }
        QFETCH(QString, scenario);
        if (scenario == "widget-receiver") {
            auto widget = std::make_unique<QWidget>();
            widget->winId();
            auto agent = std::make_unique<Agent>();
            QVERIFY(agent->setup(widget.get()));
            Observer observer;
            int notifications = 0;
            observer.action = [&] {
                ++notifications;
                agent.reset();
                widget.reset();
            };
            agent->context()->installSharedEventFilter(&observer);
            // Exercise Qt's receiver-deletion contract with an explicit WinIdChange.
            // Destroying QWidget from inside QWidget::destroy() itself is unsupported.
            agent->context()->forgetRecordedId();
            QEvent event(QEvent::WinIdChange);
            QVERIFY(QCoreApplication::sendEvent(widget.get(), &event));
            QVERIFY(!widget);
            QCOMPARE(notifications, 1);
            return;
        }
        auto window = std::make_unique<QWindow>();
        window->resize(300, 200);
        auto context = std::make_unique<Context>();
        context->setup(window.get(), new WindowDelegate(window.get()));
        window->create();
        QVERIFY(context->windowId());
        Observer observer;
        int notifications = 0;
        QList<WId> ids;
        observer.action = [&] {
            ++notifications;
            ids.append(context->windowId());
            if (scenario == "delete-context" || scenario == "delete-receiver") {
                context.reset();
                if (scenario == "delete-receiver")
                    window.reset();
            } else if (scenario == "nested" && notifications == 1) {
                QPlatformSurfaceEvent nested(QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed);
                QCoreApplication::sendEvent(window.get(), &nested);
                // Still in the outer destruction notification: the native surface
                // exists, but must remain logically unavailable throughout this frame.
                context->notifyWinIdChange();
            }
        };
        context->installSharedEventFilter(&observer);
        if (scenario == "delete-receiver") {
            // QObject allows deletion from a filter, provided delivery is consumed.
            // Do not delete QWindow from inside its own destroy() implementation.
            QPlatformSurfaceEvent event(QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed);
            QVERIFY(QCoreApplication::sendEvent(window.get(), &event));
            QVERIFY(!window);
        } else {
            window->destroy(); // Real Windows QPA surface teardown.
        }
        QCOMPARE(notifications, 1);
        QCOMPARE(ids, QList<WId>({0}));
        if (scenario == "native-recreate" || scenario == "nested") {
            QCOMPARE(context->windowId(), WId(0));
            window->create();
            QCOMPARE(notifications, 2);
            QVERIFY(ids.last());
            QCOMPARE(context->windowId(), window->winId());
        }
        observer.action = [] {};
    }
};

int main(int argc, char **argv) {
    ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    auto arguments = app.arguments();
    childProcess = arguments.removeAll("--child") > 0;
    const auto previous = QWK::WindowAgentBasePrivate::windowContextFactoryMethod;
    QWK::WindowAgentBasePrivate::windowContextFactoryMethod = []() -> QWK::AbstractWindowContext * {
        return new Context;
    };
    WindowLifetimeTest test;
    const int result = QTest::qExec(&test, arguments);
    QWK::WindowAgentBasePrivate::windowContextFactoryMethod = previous;
    for (auto page : std::as_const(retiredPages))
        ::VirtualFree(page, 0, MEM_RELEASE);
    return result;
}

#include "tst_windowlifetime.moc"
