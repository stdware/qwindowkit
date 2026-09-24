// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <QtGui/QGuiApplication>
#include <QtGui/QMouseEvent>
#include <QtTest/QTest>
#include <QWKCore/private/qtwindowcontext_p.h>
#include "windowcontextfixture.h"

namespace {
    class Delegate : public QwkTest::Delegate {
    public:
        using QwkTest::Delegate::Delegate;
        mutable Qt::CursorShape cursor = Qt::CrossCursor;
        mutable int restores = 0;
        void setCursorShape(QObject *, Qt::CursorShape shape) const override { cursor = shape; }
        void restoreCursorShape(QObject *) const override {
            cursor = Qt::CrossCursor;
            ++restores;
        }
    };

    class Context : public QWK::QtWindowContext {
    public:
        QList<QPoint> menus;
        QList<Qt::Edges> resizes;
        int moves = 0;
        void systemMove() override { ++moves; }
        void systemResize(Qt::Edges edges) override { resizes.append(edges); }
        void showSystemMenu(const QPoint &pos) override { menus.append(pos); }
    };

    struct Fixture {
        QWindow window;
        QwkTest::Item title, excluded;
        Delegate *delegate = new Delegate(&window);
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
    void resizeConstraints_data() {
        QTest::addColumn<int>("mode");
        QTest::addColumn<int>("visibility");
        QTest::addColumn<QPoint>("point");
        QTest::addColumn<int>("edges");
        QTest::addColumn<int>("cursor");
        struct Hit {
            const char *name;
            QPoint point;
            Qt::Edges free, width, height;
            Qt::CursorShape freeCursor, widthCursor, heightCursor;
        };
        const Hit hits[] = {
            {"left", {1, 100}, Qt::LeftEdge, {}, Qt::LeftEdge,
             Qt::SizeHorCursor, Qt::CrossCursor, Qt::SizeHorCursor},
            {"right", {298, 100}, Qt::RightEdge, {}, Qt::RightEdge,
             Qt::SizeHorCursor, Qt::CrossCursor, Qt::SizeHorCursor},
            {"top", {150, 1}, Qt::TopEdge, Qt::TopEdge, {},
             Qt::SizeVerCursor, Qt::SizeVerCursor, Qt::CrossCursor},
            {"bottom", {150, 198}, Qt::BottomEdge, Qt::BottomEdge, {},
             Qt::SizeVerCursor, Qt::SizeVerCursor, Qt::CrossCursor},
            {"top-left", {1, 1}, Qt::LeftEdge | Qt::TopEdge, Qt::TopEdge, Qt::LeftEdge,
             Qt::SizeFDiagCursor, Qt::SizeVerCursor, Qt::SizeHorCursor},
            {"top-right", {298, 1}, Qt::RightEdge | Qt::TopEdge, Qt::TopEdge, Qt::RightEdge,
             Qt::SizeBDiagCursor, Qt::SizeVerCursor, Qt::SizeHorCursor},
            {"bottom-left", {1, 198}, Qt::LeftEdge | Qt::BottomEdge, Qt::BottomEdge, Qt::LeftEdge,
             Qt::SizeBDiagCursor, Qt::SizeVerCursor, Qt::SizeHorCursor},
            {"bottom-right", {298, 198}, Qt::RightEdge | Qt::BottomEdge, Qt::BottomEdge, Qt::RightEdge,
             Qt::SizeFDiagCursor, Qt::SizeVerCursor, Qt::SizeHorCursor},
            {"title", {150, 20}, {}, {}, {}, Qt::CrossCursor, Qt::CrossCursor, Qt::CrossCursor},
            {"client", {150, 100}, {}, {}, {}, Qt::CrossCursor, Qt::CrossCursor, Qt::CrossCursor},
        };
        for (int mode = 0; mode < 5; ++mode) {
            for (auto visibility : {QWindow::Windowed, QWindow::Maximized, QWindow::FullScreen}) {
                for (const auto &hit : hits) {
                    Qt::Edges edges;
                    auto cursor = Qt::CrossCursor;
#ifndef Q_OS_MACOS
                    if (visibility == QWindow::Windowed && mode < 3) {
                        edges = mode == 0 ? hit.free : mode == 1 ? hit.width : hit.height;
                        cursor = mode == 0 ? hit.freeCursor : mode == 1 ? hit.widthCursor : hit.heightCursor;
                    }
#endif
                    const auto name = QByteArray::number(mode) + "-" + QByteArray::number(visibility)
                                      + "-" + hit.name;
                    QTest::newRow(name.constData()) << mode << int(visibility) << hit.point
                                                    << int(edges) << int(cursor);
                }
            }
        }
    }

    void resizeConstraints() {
        QFETCH(int, mode);
        QFETCH(int, visibility);
        QFETCH(QPoint, point);
        QFETCH(int, edges);
        QFETCH(int, cursor);
        Fixture f;
        f.title.rect.setWidth(300);
        f.window.setFlags(Qt::Window | Qt::FramelessWindowHint);
        if (mode == 1 || mode == 3) {
            f.window.setMinimumWidth(300);
            f.window.setMaximumWidth(300);
        }
        if (mode == 2 || mode == 3) {
            f.window.setMinimumHeight(200);
            f.window.setMaximumHeight(200);
        }
        if (mode == 4)
            f.window.setFlags(f.window.flags() | Qt::MSWindowsFixedSizeDialogHint);
        f.window.setVisibility(QWindow::Visibility(visibility)); // Offscreen QPA only.
        // Keep the hit-test geometry identical across visibility states.
        f.window.resize(300, 200);
        QCOMPARE(int(f.window.visibility()), visibility);
        QVERIFY(!f.mouse(QEvent::MouseMove, point, Qt::NoButton));
        QCOMPARE(int(f.delegate->cursor), cursor);
        const bool title = point.y() < 40;
        const bool consumed = edges != 0 || title;
        QCOMPARE(f.mouse(QEvent::MouseButtonPress, point, Qt::LeftButton), consumed);
        QCOMPARE(f.accepted, consumed);
        QCOMPARE(f.context.resizes, edges ? QList<Qt::Edges>({Qt::Edges(edges)}) : QList<Qt::Edges>());
        QCOMPARE(f.mouse(QEvent::MouseMove, point + QPoint(10, 0), Qt::LeftButton), consumed);
        QCOMPARE(f.context.moves, edges == 0 && title ? 1 : 0);
        QCOMPARE(f.mouse(QEvent::MouseButtonRelease, QPoint(150, 100), Qt::LeftButton), consumed);
        QVERIFY(!f.mouse(QEvent::MouseButtonRelease, QPoint(150, 100), Qt::LeftButton));
    }

    void cursorAfterConstraintChange_data() {
        QTest::addColumn<int>("mode");
        QTest::addColumn<bool>("press");
        for (int mode = 0; mode < 5; ++mode) {
            for (bool press : {false, true}) {
                const auto name = QByteArray::number(mode) + (press ? "-press" : "-hover");
                QTest::newRow(name.constData()) << mode << press;
            }
        }
    }

    void cursorAfterConstraintChange() {
        QFETCH(int, mode);
        QFETCH(bool, press);
        Fixture f;
        f.window.setVisibility(QWindow::Windowed);
        QVERIFY(!f.mouse(QEvent::MouseMove, {1, 1}, Qt::NoButton));
#ifdef Q_OS_MACOS
        QCOMPARE(f.delegate->cursor, Qt::CrossCursor);
#else
        QCOMPARE(f.delegate->cursor, Qt::SizeFDiagCursor);
#endif
        if (mode < 3) {
            if (mode != 1) {
                f.window.setMinimumWidth(300);
                f.window.setMaximumWidth(300);
            }
            if (mode != 0) {
                f.window.setMinimumHeight(200);
                f.window.setMaximumHeight(200);
            }
        } else {
            f.window.setVisibility(mode == 3 ? QWindow::Maximized : QWindow::FullScreen);
        }
        f.mouse(press ? QEvent::MouseButtonPress : QEvent::MouseMove, {1, 1},
                press ? Qt::LeftButton : Qt::NoButton);
#ifdef Q_OS_MACOS
        QCOMPARE(f.delegate->cursor, Qt::CrossCursor);
        QCOMPARE(f.delegate->restores, 0);
#else
        QCOMPARE(f.delegate->cursor, mode == 0 ? Qt::SizeVerCursor :
                                     mode == 1 ? Qt::SizeHorCursor : Qt::CrossCursor);
        // Changing constraints/state releases ownership before the next mouse event.
        QCOMPARE(f.delegate->restores, 1);
#endif
        f.mouse(QEvent::MouseButtonRelease, {150, 100}, Qt::LeftButton);
        f.mouse(QEvent::MouseMove, {150, 100}, Qt::NoButton);
        QCOMPARE(f.delegate->cursor, Qt::CrossCursor);
    }

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
