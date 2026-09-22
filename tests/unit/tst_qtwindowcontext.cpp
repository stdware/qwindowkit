// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <QtGui/QGuiApplication>
#include <QtGui/QMouseEvent>
#include <QtTest/QTest>
#include <QWKCore/private/qtwindowcontext_p.h>
#include "windowcontextfixture.h"

namespace {
    class Context : public QWK::QtWindowContext {
    public:
        QList<QPoint> menus;
        void virtual_hook(int id, void *data) override {
            if (id == ShowSystemMenuHook) {
                menus.append(*static_cast<QPoint *>(data));
                return;
            }
            QtWindowContext::virtual_hook(id, data);
        }
    };

    struct Fixture {
        QWindow window;
        QwkTest::Item title, excluded;
        QwkTest::Delegate *delegate = new QwkTest::Delegate(&window);
        Context context;
        Fixture() {
            window.resize(300, 200);
            context.setup(&window, delegate);
            context.setTitleBar(&title);
        }
        bool accepted = false;
        bool mouse(QEvent::Type type, const QPoint &scene, Qt::MouseButton button) {
            // Distinct local/scene/global positions catch accidental use of the wrong space.
            const QPoint global = scene + QPoint(700, 500);
            QMouseEvent event(type, QPointF(250, 150), QPointF(scene), QPointF(global),
                              button, type == QEvent::MouseButtonRelease ? Qt::NoButton : button,
                              Qt::NoModifier);
            event.setAccepted(false);
            const bool handled = context.sharedDispatch(&window, &event);
            accepted = event.isAccepted();
            return handled;
        }
    };
}

class QtWindowContextTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void doubleClick_data() {
        QTest::addColumn<QString>("mode");
        QTest::addColumn<int>("initial");
        QTest::addColumn<int>("expected");
        QTest::addColumn<bool>("handled");
        QTest::newRow("maximize") << QString("normal") << 0 << int(Qt::WindowMaximized) << true;
        QTest::newRow("restore-preserves-active") << QString("normal")
            << int(Qt::WindowMaximized | Qt::WindowActive) << int(Qt::WindowActive) << true;
        QTest::newRow("maximize-preserves-active") << QString("normal")
            << int(Qt::WindowActive) << int(Qt::WindowMaximized | Qt::WindowActive) << true;
        QTest::newRow("fullscreen") << QString("normal")
            << int(Qt::WindowFullScreen) << int(Qt::WindowFullScreen) << false;
        for (const auto &mode : {"no-maximize-button", "fixed-size", "no-title", "hidden-title",
                                "disabled-title", "excluded", "system-button", "outside",
                                "right-button", "middle-button"}) {
            QTest::newRow(mode) << QString(mode) << 0 << 0 << false;
        }
    }

    void doubleClick() {
        QFETCH(QString, mode);
        QFETCH(int, initial);
        QFETCH(int, expected);
        QFETCH(bool, handled);
        Fixture f;
        f.delegate->state = Qt::WindowStates(initial);
        QPoint point(50, 20);
        Qt::MouseButton button = Qt::LeftButton;
        if (mode == "no-maximize-button")
            f.delegate->flags &= ~Qt::WindowMaximizeButtonHint;
        if (mode == "fixed-size") {
            f.window.setMinimumSize(f.window.size());
            f.window.setMaximumSize(f.window.size());
        }
        if (mode == "no-title") {
            auto title = new QwkTest::Item;
            f.context.setTitleBar(title);
            delete title;
        }
        if (mode == "hidden-title")
            f.title.visible = false;
        if (mode == "disabled-title")
            f.title.enabled = false;
        if (mode == "excluded")
            f.context.setHitTestVisible(&f.excluded, true);
        if (mode == "system-button")
            f.context.setSystemButton(QWK::WindowAgentBase::Close, &f.excluded);
        if (mode == "outside")
            point = QPoint(50, 100);
        if (mode == "right-button")
            button = Qt::RightButton;
        if (mode == "middle-button")
            button = Qt::MiddleButton;
        QCOMPARE(f.mouse(QEvent::MouseButtonDblClick, point, button), handled);
        QCOMPARE(f.accepted, handled);
        QCOMPARE(int(f.delegate->state), expected);
        QCOMPARE(f.delegate->operations, handled ? QStringList({"state"}) : QStringList());
        QVERIFY(f.context.menus.isEmpty());
        QVERIFY(!f.window.isVisible());
    }

    void systemMenu_data() {
        QTest::addColumn<QString>("mode");
        QTest::addColumn<bool>("handled");
        QTest::newRow("title") << QString("title") << true;
        QTest::newRow("excluded") << QString("excluded") << false;
        QTest::newRow("outside") << QString("outside") << false;
        QTest::newRow("middle-button") << QString("middle-button") << false;
    }

    void systemMenu() {
        QFETCH(QString, mode);
        QFETCH(bool, handled);
        Fixture f;
        if (mode == "excluded")
            f.context.setHitTestVisible(&f.excluded, true);
        const QPoint point = mode == "outside" ? QPoint(50, 100) : QPoint(50, 20);
        const auto button = mode == "middle-button" ? Qt::MiddleButton : Qt::RightButton;
        QCOMPARE(f.mouse(QEvent::MouseButtonPress, point, button), handled);
        QCOMPARE(f.accepted, handled);
        QCOMPARE(f.context.menus, handled ? QList<QPoint>({point + QPoint(700, 500)})
                                          : QList<QPoint>());
        QVERIFY(f.delegate->operations.isEmpty());
        QVERIFY(!f.window.isVisible());
    }

    void releaseAfterTitlePress() {
        Fixture f;
        QVERIFY(f.mouse(QEvent::MouseButtonPress, QPoint(50, 20), Qt::LeftButton));
        QVERIFY(f.accepted);
        // No move event: releasing outside still finishes the title press, without dragging.
        QVERIFY(f.mouse(QEvent::MouseButtonRelease, QPoint(50, 100), Qt::LeftButton));
        QVERIFY(f.accepted);
        QVERIFY(!f.mouse(QEvent::MouseButtonRelease, QPoint(50, 100), Qt::LeftButton));
        QVERIFY(!f.accepted);
        QVERIFY(f.delegate->operations.isEmpty());
        QVERIFY(!f.window.isVisible());
    }

    void releaseAfterClientPress() {
        Fixture f;
        QVERIFY(!f.mouse(QEvent::MouseButtonPress, QPoint(50, 100), Qt::LeftButton));
        QVERIFY(!f.accepted);
        QVERIFY(!f.mouse(QEvent::MouseButtonRelease, QPoint(50, 20), Qt::LeftButton));
        QVERIFY(!f.accepted);
        // The previous release reset WaitingRelease to Idle; title releases are now consumed.
        QVERIFY(f.mouse(QEvent::MouseButtonRelease, QPoint(50, 20), Qt::LeftButton));
        QVERIFY(f.accepted);
        QVERIFY(f.delegate->operations.isEmpty());
    }

    void unrelatedEventKeepsPendingRelease() {
        Fixture f;
        QVERIFY(f.mouse(QEvent::MouseButtonPress, QPoint(50, 20), Qt::LeftButton));
        QEvent event(QEvent::User);
        event.setAccepted(false);
        QVERIFY(!f.context.sharedDispatch(&f.window, &event));
        QVERIFY(!event.isAccepted());
        QVERIFY(f.mouse(QEvent::MouseButtonRelease, QPoint(50, 100), Qt::LeftButton));
        QVERIFY(f.accepted);
        QVERIFY(f.delegate->operations.isEmpty());
    }

    void frameFlagsFollowWindowLifetime() {
        Fixture f;
        QCOMPARE(f.context.key(), QStringLiteral("qt"));
        const Qt::WindowFlags original = Qt::Window | Qt::WindowCloseButtonHint;
        f.delegate->flags = original;
        f.delegate->id = 1;
        f.context.notifyWinIdChange();
        QCOMPARE(f.delegate->flags, original | Qt::FramelessWindowHint);
        QCOMPARE(f.delegate->operations, QStringList({"flags"}));
        f.context.notifyWinIdChange();
        QCOMPARE(f.delegate->operations.size(), 1);
        f.delegate->host = nullptr;
        f.delegate->id = 0;
        f.context.notifyWinIdChange();
        QCOMPARE(f.delegate->flags, original);
        f.delegate->host = &f.window;
        f.delegate->id = 2;
        f.context.notifyWinIdChange();
        QCOMPARE(f.delegate->flags, original | Qt::FramelessWindowHint);
        QCOMPARE(f.delegate->operations, QStringList({"flags", "flags", "flags"}));
        QVERIFY(!f.window.isVisible());
    }
};

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QtWindowContextTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_qtwindowcontext.moc"
