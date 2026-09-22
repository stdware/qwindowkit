// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <functional>
#include <memory>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtTest/QTest>
#include "windowcontextfixture.h"

namespace {
    using Button = QWK::WindowAgentBase;

    using QwkTest::Item;
    using QwkTest::Delegate;

    struct AttributeCall {
        QString key;
        QVariant value;
        QVariant oldValue;
    };

    class Context : public QWK::AbstractWindowContext {
    public:
        QList<AttributeCall> calls;
        QStringList rejectedKeys;
        QList<QPair<WId, WId>> handles;
        QStringList sequence;
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
        }
        bool windowAttributeChanged(const QString &key, const QVariant &value,
                                    const QVariant &oldValue) override {
            calls.append({key, value, oldValue});
            sequence.append("attribute:" + key);
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
private Q_SLOTS:
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
        QVERIFY(!f.context.calls.first().oldValue.isValid());
        f.context.rejectedKeys << "alpha" << "beta";
        QVERIFY(!f.context.setWindowAttribute("alpha", 9));
        QCOMPARE(f.context.windowAttribute("alpha").toInt(), 7);
        QCOMPARE(f.context.calls.last().value.toInt(), 9);
        QCOMPARE(f.context.calls.last().oldValue.toInt(), 7);
        QVERIFY(!f.context.setWindowAttribute("alpha", {}));
        QCOMPARE(f.context.windowAttribute("alpha").toInt(), 7);
        QVERIFY(!f.context.calls.last().value.isValid());
        QVERIFY(!f.context.setWindowAttribute("beta", 11));
        QVERIFY(!f.context.windowAttribute("beta").isValid());
        f.context.rejectedKeys.clear();
        QVERIFY(f.context.setWindowAttribute("alpha", 13));
        QCOMPARE(f.context.calls.last().oldValue.toInt(), 7);
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
        for (const auto &call : f.context.calls)
            QVERIFY(!call.oldValue.isValid());

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
        QVERIFY(!f.context.calls.first().oldValue.isValid());
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
    QGuiApplication app(argc, argv);
    WindowContextTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_windowcontext.moc"
