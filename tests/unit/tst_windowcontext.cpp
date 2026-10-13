// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <functional>
#include <memory>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryDir>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtTest/QTest>
#include "windowcontextfixture.h"
#include "systembuttoncases.h"
#ifdef Q_OS_WIN
#  include <QtCore/qt_windows.h>
#endif

namespace {
    bool childProcess = false;
    using Button = QWK::WindowAgentBase;

    using QwkTest::Item;
    using QwkTest::Delegate;

    struct AttributeCall {
        QString key;
        QVariant value;
    };

    class Context : public QWK::AbstractWindowContext {
    public:
        QList<AttributeCall> calls;
        QStringList rejectedKeys;
        QList<QPair<WId, WId>> handles;
        QStringList sequence;
        std::function<bool(const QString &, const QVariant &)> onAttribute;
        std::function<void()> onHandle;
        bool storageHasSize(int size) const {
            return int(m_windowAttributesOrder.size()) == size && m_windowAttributes.size() == size;
        }
        QStringList keys() const {
            QStringList result;
            for (const auto &call : calls)
                result.append(call.key);
            return result;
        }
    protected:
        void winIdChanged(WId id, WId oldId) override {
            handles.append(qMakePair(id, oldId));
            sequence.append("handle");
            const auto callback = onHandle;
            if (callback)
                callback();
        }
        bool windowAttributeChanged(const QString &key, const QVariant &value) override {
            calls.append({key, value});
            sequence.append("attribute:" + key);
            const auto callback = onAttribute;
            if (callback)
                return callback(key, value);
            return !rejectedKeys.contains(key);
        }
    };

    struct Fixture {
        QWindow window;
        Delegate *delegate = new Delegate(&window); // Owned by context after setup.
        Context context;
        Fixture() {
            window.resize(300, 200);
            context.setup(&window, delegate);
        }
        void changeHandle(WId id) {
            delegate->id = id;
            context.notifyWinIdChange();
        }
    };

    class Observer : public QWK::SharedEventFilter {
    public:
        std::function<bool(QObject *, QEvent *)> callback;
        bool sharedEventFilter(QObject *object, QEvent *event) override {
            return callback(object, event);
        }
        bool attached() const { return m_sharedDispatcher != nullptr; }
    };
}

class WindowContextTest : public QObject {
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
    void systemButtonBoundaries_data() {
        QTest::addColumn<int>("value");
        QTest::addColumn<bool>("valid");
        for (const auto &row : QwkTest::systemButtonCases)
            QTest::newRow(row.name) << row.value << row.valid;
    }

    void systemButtonBoundaries() {
        if (!childProcess) { runChild(); return; }
        QFETCH(int, value);
        QFETCH(bool, valid);
        Fixture f;
        Item original, replacement, title, excluded;
        QVERIFY(f.context.setTitleBar(&title));
        QVERIFY(f.context.setHitTestVisible(&excluded, true));
        for (auto role : {Button::WindowIcon, Button::Help, Button::Minimize,
                          Button::Maximize, Button::Close})
            QVERIFY(f.context.setSystemButton(role, &original));
        // SystemButton has a fixed int underlying type, so these conversions are
        // defined even for negative values and the complete int range.
        const auto role = static_cast<Button::SystemButton>(value);
        QCOMPARE(f.context.systemButton(role), valid ? &original : nullptr);
        QCOMPARE(f.context.setSystemButton(role, &replacement), valid);
        QCOMPARE(f.context.systemButton(role), valid ? &replacement : nullptr);
        QVERIFY(!f.context.setSystemButton(role, &replacement));
        for (auto other : {Button::WindowIcon, Button::Help, Button::Minimize,
                           Button::Maximize, Button::Close})
            QCOMPARE(f.context.systemButton(other), other == role ? &replacement : &original);
        QCOMPARE(f.context.setSystemButton(role, nullptr), valid);
        QVERIFY(!f.context.systemButton(role));
        QVERIFY(!f.context.setSystemButton(role, nullptr));
        for (auto other : {Button::WindowIcon, Button::Help, Button::Minimize,
                           Button::Maximize, Button::Close})
            QCOMPARE(f.context.systemButton(other), other == role ? nullptr : &original);
        QCOMPARE(f.context.titleBar(), &title);
        QVERIFY(f.context.isHitTestVisible(&excluded));
    }

    void reentrantWrites_data() {
        QTest::addColumn<QString>("scenario");
        for (const auto name : {"insert-replace", "insert-remove", "update-replace",
                               "update-remove", "remove-reinsert", "failed-inner",
                               "failed-outer", "grow-hash", "recreate"})
            QTest::newRow(name) << QString::fromLatin1(name);
    }

    void reentrantWrites() {
        if (!childProcess) { runChild(); return; }
        QFETCH(QString, scenario);
        Fixture f;
        const bool initiallyAbsent = scenario.startsWith("insert-");
        if (!initiallyAbsent)
            QVERIFY(f.context.setWindowAttribute("alpha", 1));
        f.changeHandle(1);
        bool inside = false;
        bool nestedResult = false;
        bool argumentsStable = false;
        f.context.onAttribute = [&](const QString &key, const QVariant &value) {
            if (inside)
                return scenario != "failed-inner";
            inside = true;
            if (scenario == "grow-hash") {
                for (int i = 0; i < 300; ++i)
                    nestedResult = f.context.setWindowAttribute(QString::number(i), i);
            } else if (scenario == "recreate") {
                f.changeHandle(0);
                f.changeHandle(1); // Numeric reuse must still invalidate the outer operation.
            } else {
                nestedResult = f.context.setWindowAttribute("alpha",
                    scenario.endsWith("remove") ? QVariant{} : QVariant(3));
            }
            // Callback arguments remain valid across erasure and hash growth.
            argumentsStable = key == "alpha" &&
                value == (scenario == "remove-reinsert" ? QVariant{} : QVariant(2));
            return scenario != "failed-outer";
        };
        const bool result = f.context.setWindowAttribute("alpha",
            scenario == "remove-reinsert" ? QVariant{} : QVariant(2));
        f.context.onAttribute = {};
        QVERIFY(argumentsStable);
        const bool outerWins = scenario == "failed-inner" || scenario == "grow-hash";
        QCOMPARE(result, outerWins);
        if (scenario != "recreate")
            QCOMPARE(nestedResult, scenario != "failed-inner");
        const QVariant expected = outerWins ? QVariant(2) : scenario.endsWith("remove")
            ? QVariant{} : scenario == "recreate" ? QVariant(1) : QVariant(3);
        QCOMPARE(f.context.windowAttribute("alpha"), expected);
        const int count = scenario == "grow-hash" ? 301 : expected.isValid() ? 1 : 0;
        QVERIFY(f.context.storageHasSize(count));
        f.context.calls.clear();
        f.changeHandle(2);
        QCOMPARE(f.context.calls.size(), count);
        if (expected.isValid()) {
            QCOMPARE(f.context.calls.last().key, QString("alpha"));
            QCOMPARE(f.context.calls.last().value, expected);
        }
    }

    void reentrantReplay_data() {
        QTest::addColumn<QString>("scenario");
        for (const auto name : {"update-current", "remove-current", "update-next", "remove-next",
                               "remove-reinsert-next", "add-new", "reject-replaced", "recreate"})
            QTest::newRow(name) << QString::fromLatin1(name);
    }

    void reentrantReplay() {
        if (!childProcess) { runChild(); return; }
        QFETCH(QString, scenario);
        Fixture f;
        QVERIFY(f.context.setWindowAttribute("alpha", 1));
        QVERIFY(f.context.setWindowAttribute("beta", 2));
        bool first = true;
        bool nestedResult = true;
        Observer observer;
        QList<WId> notifications;
        observer.callback = [&](QObject *, QEvent *event) {
            if (event->type() == QEvent::WinIdChange)
                notifications.append(f.context.windowId());
            return false;
        };
        f.context.installSharedEventFilter(&observer);
        f.context.onAttribute = [&](const QString &, const QVariant &) {
            if (!std::exchange(first, false))
                return true;
            if (scenario == "recreate") {
                f.changeHandle(0);
                f.changeHandle(1);
            } else {
                const QString key = scenario.endsWith("next") ? "beta"
                    : scenario == "add-new" ? "gamma" : "alpha";
                if (scenario == "remove-reinsert-next")
                    nestedResult = f.context.setWindowAttribute(key, {});
                nestedResult = nestedResult && f.context.setWindowAttribute(key,
                    scenario.startsWith("remove-") && scenario != "remove-reinsert-next"
                        ? QVariant{} : QVariant(3));
            }
            return scenario != "reject-replaced";
        };
        f.changeHandle(1);
        QVERIFY(nestedResult);
        QCOMPARE(notifications, scenario == "recreate" ? QList<WId>({0, 1}) : QList<WId>({1}));
        f.context.onAttribute = {};
        const QStringList expected = scenario == "add-new" ? QStringList{"alpha", "gamma", "beta"}
            : scenario == "recreate" ? QStringList{"alpha", "alpha", "beta"}
            : scenario == "remove-reinsert-next" ? QStringList{"alpha", "beta", "beta"}
            : scenario.endsWith("next") ? QStringList{"alpha", "beta"}
            : QStringList{"alpha", "alpha", "beta"};
        QCOMPARE(f.context.keys(), expected);
        const int count = scenario.startsWith("remove-") && scenario != "remove-reinsert-next" ? 1
            : scenario == "add-new" ? 3 : 2;
        QVERIFY(f.context.storageHasSize(count));
        QCOMPARE(f.context.windowAttribute("alpha"), scenario == "remove-current" ? QVariant{}
            : scenario == "update-current" || scenario == "reject-replaced" ? QVariant(3) : QVariant(1));
        QCOMPARE(f.context.windowAttribute("beta"), scenario == "remove-next" ? QVariant{}
            : scenario == "update-next" || scenario == "remove-reinsert-next" ? QVariant(3) : QVariant(2));
        f.context.calls.clear();
        f.changeHandle(2);
        QCOMPARE(f.context.calls.size(), count);
    }

    void callbackDeletion_data() {
        QTest::addColumn<QString>("scenario");
        for (const auto name : {"insert", "update", "replay", "handle"})
            QTest::newRow(name) << QString::fromLatin1(name);
    }

    void callbackDeletion() {
        if (!childProcess) { runChild(); return; }
        QFETCH(QString, scenario);
        QWindow window;
        auto context = std::make_unique<Context>();
        auto delegate = new Delegate(&window);
        context->setup(&window, delegate);
        if (scenario != "insert")
            QVERIFY(context->setWindowAttribute("alpha", 1));
        if (scenario == "insert" || scenario == "update") {
            delegate->id = 1;
            context->notifyWinIdChange();
        }
        context->onAttribute = [&](const QString &, const QVariant &) {
            context.reset();
            return true;
        };
        if (scenario == "handle")
            context->onHandle = [&] { context.reset(); };
        if (scenario == "insert" || scenario == "update") {
            QVERIFY(!context->setWindowAttribute("alpha", 2));
        } else {
            delegate->id = 1;
            context->notifyWinIdChange();
        }
        QVERIFY(!context);
    }

    void setupRejectsInvalidOrRepeatedHosts() {
        QWindow first, second;
        Context context;
        Delegate rejected(&second);
        context.setup(nullptr, &rejected);
        context.setup(&first, nullptr);
        QVERIFY(!context.host());
        QVERIFY(!context.delegate());
        auto delegate = new Delegate(&first);
        context.setup(&first, delegate);
        context.setup(&second, &rejected);
        QCOMPARE(context.host(), &first);
        QCOMPARE(context.window(), &first);
        QCOMPARE(context.delegate(), delegate);
    }

    void raisePreservesOtherStateBits_data() {
        QTest::addColumn<int>("initial");
        QTest::addColumn<int>("expected");
        QTest::addColumn<bool>("restored");
        QTest::newRow("normal") << int(Qt::WindowNoState) << int(Qt::WindowNoState) << false;
        QTest::newRow("minimized") << int(Qt::WindowMinimized) << int(Qt::WindowNoState) << true;
        QTest::newRow("maximized") << int(Qt::WindowMaximized) << int(Qt::WindowMaximized) << false;
        QTest::newRow("minimized-maximized-active")
            << int(Qt::WindowMinimized | Qt::WindowMaximized | Qt::WindowActive)
            << int(Qt::WindowMaximized | Qt::WindowActive) << true;
        QTest::newRow("fullscreen") << int(Qt::WindowFullScreen) << int(Qt::WindowFullScreen) << false;
    }

    void raisePreservesOtherStateBits() {
        QFETCH(int, initial);
        QFETCH(int, expected);
        QFETCH(bool, restored);
        Fixture f;
        f.changeHandle(1);
        f.delegate->state = Qt::WindowStates(initial);
        f.context.virtual_hook(Context::RaiseWindowHook, nullptr);
        QCOMPARE(int(f.delegate->state), expected);
        QCOMPARE(f.delegate->operations, restored ? QStringList({"show", "state", "raise"})
                                                 : QStringList({"show", "raise"}));
        QVERIFY(!f.window.isVisible()); // Only recorded delegate calls, no desktop actions.
    }

    void windowActionsWithoutHandleAreNoOps() {
        Fixture f;
        f.delegate->state = Qt::WindowMinimized;
        const auto geometry = f.delegate->geometry;
        f.context.virtual_hook(Context::RaiseWindowHook, nullptr);
        f.context.virtual_hook(Context::CentralizeHook, nullptr);
        QVERIFY(f.delegate->operations.isEmpty());
        QCOMPARE(f.delegate->state, Qt::WindowStates(Qt::WindowMinimized));
        QCOMPARE(f.delegate->geometry, geometry);
    }

    void centralizeKeepsWindowSize() {
        Fixture f;
        f.changeHandle(1);
        QVERIFY(f.window.screen());
        auto expected = f.delegate->geometry;
        const auto screen = f.window.screen()->geometry();
        expected.moveTopLeft(screen.topLeft() + QPoint((screen.width() - expected.width()) / 2,
                                                      (screen.height() - expected.height()) / 2));
        f.context.virtual_hook(Context::CentralizeHook, nullptr);
        QCOMPARE(f.delegate->geometry, expected);
        QCOMPARE(f.delegate->operations, QStringList({"geometry"}));
        QVERIFY(!f.window.isVisible());
    }

    void handleNotificationFollowsAttributeReplay() {
        Fixture f;
        QVERIFY(f.context.setWindowAttribute("alpha", 4));
        Observer observer;
        QObject *notifiedHost = nullptr;
        observer.callback = [&](QObject *object, QEvent *event) {
            if (event->type() == QEvent::WinIdChange) {
                notifiedHost = object;
                f.context.sequence.append("notify");
            }
            return false;
        };
        f.context.installSharedEventFilter(&observer);
        f.changeHandle(1);
        QCOMPARE(notifiedHost, &f.window);
        QCOMPARE(f.context.sequence, QStringList({"handle", "attribute:alpha", "notify"}));
        f.context.sequence.clear();
        f.changeHandle(1);
        QVERIFY(f.context.sequence.isEmpty());
        f.changeHandle(0);
        QCOMPARE(f.context.sequence, QStringList({"handle", "notify"}));
    }

    void eventFilterFollowsHostWindowReplacement() {
        QWindow replacement;
        Fixture f;
        Observer observer;
        QList<QObject *> deliveries;
        observer.callback = [&](QObject *object, QEvent *event) {
            if (event->type() == QEvent::User) {
                deliveries.append(object);
                return true;
            }
            return false;
        };
        f.context.installSharedEventFilter(&observer);
        QEvent event(QEvent::User);
        QCoreApplication::sendEvent(&f.window, &event);
        QCOMPARE(deliveries, QList<QObject *>({&f.window}));
        deliveries.clear();
        f.delegate->host = &replacement;
        f.context.notifyWinIdChange(); // Even an unchanged ID must rebind the Qt event filter.
        QCoreApplication::sendEvent(&f.window, &event);
        QCoreApplication::sendEvent(&replacement, &event);
        QCOMPARE(deliveries, QList<QObject *>({&replacement}));
        deliveries.clear();
        f.delegate->host = nullptr;
        f.context.notifyWinIdChange();
        QCoreApplication::sendEvent(&replacement, &event);
        QVERIFY(deliveries.isEmpty());
    }

    void destructionUnregistersWindowObserver() {
        QWindow window;
        Observer observer;
        int deliveries = 0;
        observer.callback = [&](QObject *, QEvent *event) {
            if (event->type() == QEvent::User)
                ++deliveries;
            return false;
        };
        QEvent event(QEvent::User);
        {
            Context context;
            context.setup(&window, new Delegate(&window));
            context.installSharedEventFilter(&observer);
            QVERIFY(observer.attached());
            QCoreApplication::sendEvent(&window, &event);
            QCOMPARE(deliveries, 1);
        }
        QVERIFY(!observer.attached());
        QCoreApplication::sendEvent(&window, &event);
        QCOMPARE(deliveries, 1);
    }

    void attributesWithoutHandle() {
        Fixture f;
        QVERIFY(!f.context.windowAttribute("missing").isValid());
        QVERIFY(f.context.setWindowAttribute("missing", {}));
        QVERIFY(f.context.setWindowAttribute("alpha", 1));
        QCOMPARE(f.context.windowAttribute("alpha").toInt(), 1);
        QVERIFY(f.context.setWindowAttribute("alpha", 2));
        QCOMPARE(f.context.windowAttribute("alpha").toInt(), 2);
        QVERIFY(f.context.setWindowAttribute("alpha", {}));
        QVERIFY(!f.context.windowAttribute("alpha").isValid());
        QVERIFY(f.context.calls.isEmpty()); // No platform callback without a handle.
        f.changeHandle(1);
        QVERIFY(f.context.calls.isEmpty()); // Deleted attributes must not be replayed.
    }

    void rejectedChangesAreAtomic() {
        Fixture f;
        f.changeHandle(1);
        QVERIFY(f.context.setWindowAttribute("alpha", 7));
        QCOMPARE(f.context.calls.size(), 1);
        f.context.rejectedKeys << "alpha" << "beta";
        QVERIFY(!f.context.setWindowAttribute("alpha", 9));
        QCOMPARE(f.context.windowAttribute("alpha").toInt(), 7);
        QCOMPARE(f.context.calls.last().value.toInt(), 9);
        QVERIFY(!f.context.setWindowAttribute("alpha", {}));
        QCOMPARE(f.context.windowAttribute("alpha").toInt(), 7);
        QVERIFY(!f.context.calls.last().value.isValid());
        QVERIFY(!f.context.setWindowAttribute("beta", 11));
        QVERIFY(!f.context.windowAttribute("beta").isValid());
        f.context.rejectedKeys.clear();
        QVERIFY(f.context.setWindowAttribute("alpha", 13));
        QCOMPARE(f.context.windowAttribute("alpha").toInt(), 13);
        QVERIFY(f.context.setWindowAttribute("alpha", {}));
        QVERIFY(!f.context.windowAttribute("alpha").isValid());
    }

    void replayFollowsSuccessfulWriteOrder() {
        Fixture f;
        QVERIFY(f.context.setWindowAttribute("alpha", 1));
        QVERIFY(f.context.setWindowAttribute("beta", 2));
        QVERIFY(f.context.setWindowAttribute("gamma", 3));
        QVERIFY(f.context.setWindowAttribute("alpha", 4));
        f.changeHandle(1);
        QCOMPARE(f.context.keys(), QStringList({"beta", "gamma", "alpha"}));
        QCOMPARE(f.context.calls.last().value.toInt(), 4);

        f.context.rejectedKeys << "beta";
        QVERIFY(!f.context.setWindowAttribute("beta", 5));
        f.context.rejectedKeys.clear();
        QVERIFY(f.context.setWindowAttribute("gamma", {}));
        f.context.calls.clear();
        f.changeHandle(2);
        QCOMPARE(f.context.keys(), QStringList({"beta", "alpha"}));
        QCOMPARE(f.context.calls.first().value.toInt(), 2); // Rejected write did not reorder/update.
        QCOMPARE(f.context.calls.last().value.toInt(), 4);
    }

    void rejectedReplayRemovesOnlyFailedAttributes() {
        Fixture f;
        QVERIFY(f.context.setWindowAttribute("alpha", 1));
        QVERIFY(f.context.setWindowAttribute("beta", 2));
        QVERIFY(f.context.setWindowAttribute("gamma", 3));
        f.context.rejectedKeys << "alpha" << "beta"; // Adjacent failures exercise iterator removal.
        f.changeHandle(1);
        QCOMPARE(f.context.keys(), QStringList({"alpha", "beta", "gamma"}));
        QVERIFY(!f.context.windowAttribute("alpha").isValid());
        QVERIFY(!f.context.windowAttribute("beta").isValid());
        QCOMPARE(f.context.windowAttribute("gamma").toInt(), 3);
        f.context.calls.clear();
        f.changeHandle(2);
        QCOMPARE(f.context.keys(), QStringList({"gamma"}));
        f.context.rejectedKeys.clear();
        QVERIFY(f.context.setWindowAttribute("alpha", 4));
        QCOMPARE(f.context.windowAttribute("alpha").toInt(), 4);
        f.context.calls.clear();
        f.changeHandle(3);
        QCOMPARE(f.context.keys(), QStringList({"gamma", "alpha"}));
    }

    void handleLossAndUnchangedHandle() {
        Fixture f;
        QVERIFY(f.context.setWindowAttribute("alpha", 1));
        f.changeHandle(1);
        QCOMPARE(f.context.handles.size(), 1);
        QCOMPARE(f.context.handles.last(), qMakePair(WId(1), WId(0)));
        f.context.calls.clear();
        f.changeHandle(1);
        QVERIFY(f.context.calls.isEmpty());
        QCOMPARE(f.context.handles.size(), 1);
        f.changeHandle(0);
        QCOMPARE(f.context.handles.last(), qMakePair(WId(0), WId(1)));
        QVERIFY(f.context.calls.isEmpty());
        QVERIFY(f.context.setWindowAttribute("alpha", 2));
        QVERIFY(f.context.calls.isEmpty());
        f.changeHandle(1); // Handle values may be reused after destruction.
        QCOMPARE(f.context.handles.size(), 3);
        QCOMPARE(f.context.keys(), QStringList({"alpha"}));
        QCOMPARE(f.context.calls.first().value.toInt(), 2);
    }

    void replacingTitleClearsRegistrations() {
        Fixture f;
        Item title, replacement, button, excluded;
        QVERIFY(f.context.setTitleBar(&title));
        QVERIFY(f.context.setSystemButton(Button::Close, &button));
        QVERIFY(f.context.setHitTestVisible(&excluded, true));
        QVERIFY(!f.context.setTitleBar(&title));
        QCOMPARE(f.context.systemButton(Button::Close), &button);
        QVERIFY(f.context.isHitTestVisible(&excluded));
        QVERIFY(f.context.setTitleBar(&replacement));
        QCOMPARE(f.context.titleBar(), &replacement);
        QVERIFY(!f.context.systemButton(Button::Close));
        QVERIFY(!f.context.isHitTestVisible(&excluded));
    }

    void destroyedItemsStopAffectingHitTests() {
        Fixture f;
        auto title = std::make_unique<Item>();
        auto button = std::make_unique<Item>();
        auto excluded = std::make_unique<Item>();
        const QPoint point(10, 10);
        QVERIFY(f.context.setTitleBar(title.get()));
        QVERIFY(f.context.setSystemButton(Button::Close, button.get()));
        QVERIFY(!f.context.isInTitleBarDraggableArea(point));
        button.reset();
        QVERIFY(!f.context.systemButton(Button::Close));
        QVERIFY(f.context.isInTitleBarDraggableArea(point));
        QVERIFY(f.context.setHitTestVisible(excluded.get(), true));
        QVERIFY(!f.context.isInTitleBarDraggableArea(point));
        excluded.reset();
        QVERIFY(f.context.isInTitleBarDraggableArea(point));
        title.reset();
        QVERIFY(!f.context.titleBar());
        QVERIFY(!f.context.isInTitleBarDraggableArea(point));
    }

    void titleAndExclusionVisibility() {
        Fixture f;
        Item title, excluded;
        const QPoint point(10, 10);
        QVERIFY(!f.context.isInTitleBarDraggableArea(point));
        QVERIFY(f.context.setTitleBar(&title));
        QVERIFY(f.context.isInTitleBarDraggableArea(point));
        QVERIFY(!f.context.isInTitleBarDraggableArea(QPoint(210, 10)));
        title.visible = false;
        QVERIFY(!f.context.isInTitleBarDraggableArea(point));
        title.visible = true;
        title.enabled = false;
        QVERIFY(!f.context.isInTitleBarDraggableArea(point));
        title.enabled = true;
        title.rect.moveTopLeft(QPoint(400, 400));
        QVERIFY(!f.context.isInTitleBarDraggableArea(QPoint(410, 410)));
        title.rect.moveTopLeft(QPoint(0, 0));
        excluded.rect = QRect(0, 0, 20, 20);
        QVERIFY(f.context.setHitTestVisible(&excluded, true));
        QVERIFY(f.context.setHitTestVisible(&excluded, true));
        QVERIFY(!f.context.isInTitleBarDraggableArea(point));
        QVERIFY(f.context.isInTitleBarDraggableArea(QPoint(30, 10)));
        excluded.visible = false;
        QVERIFY(f.context.isInTitleBarDraggableArea(point));
        excluded.visible = true;
        QVERIFY(f.context.setHitTestVisible(&excluded, false));
        QVERIFY(!f.context.isHitTestVisible(&excluded));
        QVERIFY(f.context.isInTitleBarDraggableArea(point));
    }

    void systemButtonVisibilityAndPriority() {
        Fixture f;
        Item title, minimize, close;
        QVERIFY(f.context.setTitleBar(&title));
        QVERIFY(f.context.setSystemButton(Button::Close, &close));
        QVERIFY(f.context.setSystemButton(Button::Minimize, &minimize));
        Button::SystemButton role = Button::Unknown;
        QVERIFY(f.context.isInSystemButtons(QPoint(10, 10), &role));
        QCOMPARE(role, Button::Minimize); // Enum order wins for overlapping buttons.
        minimize.visible = false;
        QVERIFY(f.context.isInSystemButtons(QPoint(10, 10), &role));
        QCOMPARE(role, Button::Close);
        close.enabled = false;
        QVERIFY(!f.context.isInSystemButtons(QPoint(10, 10), &role));
        QCOMPARE(role, Button::Unknown);
        QVERIFY(f.context.isInTitleBarDraggableArea(QPoint(10, 10)));
        close.enabled = true;
        QVERIFY(!f.context.isInTitleBarDraggableArea(QPoint(10, 10)));
        QVERIFY(f.context.setSystemButton(Button::Close, nullptr));
        QVERIFY(!f.context.setSystemButton(Button::Close, nullptr));
        QVERIFY(f.context.isInTitleBarDraggableArea(QPoint(10, 10)));
    }

    void fixedSizeConstraints_data() {
        QTest::addColumn<QString>("mode");
        QTest::addColumn<bool>("widthFixed");
        QTest::addColumn<bool>("heightFixed");
        QTest::newRow("free") << QStringLiteral("free") << false << false;
        QTest::newRow("width") << QStringLiteral("width") << true << false;
        QTest::newRow("height") << QStringLiteral("height") << false << true;
        QTest::newRow("both") << QStringLiteral("both") << true << true;
        QTest::newRow("dialog-flag") << QStringLiteral("flag") << true << true;
        QTest::newRow("no-window") << QStringLiteral("no-window") << false << false;
    }

    void fixedSizeConstraints() {
        QFETCH(QString, mode);
        QFETCH(bool, widthFixed);
        QFETCH(bool, heightFixed);
        Fixture f;
        if (mode == "width" || mode == "both") {
            f.window.setMinimumWidth(300);
            f.window.setMaximumWidth(300);
        }
        if (mode == "height" || mode == "both") {
            f.window.setMinimumHeight(200);
            f.window.setMaximumHeight(200);
        }
        if (mode == "flag")
            f.window.setFlags(f.window.flags() | Qt::MSWindowsFixedSizeDialogHint);
        if (mode == "no-window") {
            f.delegate->host = nullptr;
            f.context.notifyWinIdChange();
        }
        QCOMPARE(f.context.isHostWidthFixed(), widthFixed);
        QCOMPARE(f.context.isHostHeightFixed(), heightFixed);
        QCOMPARE(f.context.isHostSizeFixed(), widthFixed && heightFixed);
    }
};

int main(int argc, char **argv) {
#ifdef Q_OS_WIN
    ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
    QGuiApplication app(argc, argv);
    auto arguments = app.arguments();
    childProcess = arguments.removeAll("--child") > 0;
    WindowContextTest test;
    return QTest::qExec(&test, arguments);
}

#include "tst_windowcontext.moc"
