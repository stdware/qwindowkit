// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <QtGui/QGuiApplication>
#include <QtCore/QPointer>
#include <QtTest/QTest>
#include <QWKCore/private/systemwindow_p.h>

namespace {
    class Window : public QWindow {
    public:
        int moves = 0, releases = 0;
        bool event(QEvent *event) override {
            if (event->type() == QEvent::MouseMove)
                ++moves;
            if (event->type() == QEvent::MouseButtonRelease)
                ++releases;
            return QWindow::event(event);
        }
    };
    class Manipulator : public QWK::WindowMoveManipulator {
    public:
        using WindowMoveManipulator::WindowMoveManipulator;
        QList<QRect> areas;
        mutable int queries = 0;
    protected:
        QList<QRect> availableScreenGeometries() const override {
            ++queries;
            return areas;
        }
    };
    void mouse(QWindow *window, QEvent::Type type, QPoint global) {
        QMouseEvent event(type, QPointF(5, 5), QPointF(5, 5), QPointF(global),
                          type == QEvent::MouseButtonRelease ? Qt::LeftButton : Qt::NoButton,
                          type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,
                          Qt::NoModifier);
        QCoreApplication::sendEvent(window, &event);
    }
}

class WindowMoveTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void manualDrag_data() {
        QTest::addColumn<QPoint>("destination");
        QTest::addColumn<QPoint>("expected");
        QTest::addColumn<QList<QRect>>("areas");
        const QRect origin(0, 0, 1000, 800), above(0, -900, 1000, 800), left(-1200, 0, 1000, 800);
        QTest::newRow("upper-screen") << QPoint(100, -700) << QPoint(100, -700)
                                      << QList<QRect>{origin, above};
        QTest::newRow("left-screen") << QPoint(-1000, 100) << QPoint(-1000, 100)
                                     << QList<QRect>{origin, left};
        QTest::newRow("gap") << QPoint(100, -80) << QPoint(100, -137)
                             << QList<QRect>{origin, above};
        QTest::newRow("offscreen") << QPoint(900, 790) << QPoint(883, 763)
                                   << QList<QRect>{origin};
        QTest::newRow("screen-removed-during-drag") << QPoint(100, -700) << QPoint(100, -4)
                                                  << QList<QRect>{origin};
        QTest::newRow("no-screens") << QPoint(100, -700) << QPoint(100, -700) << QList<QRect>{};
    }

    void manualDrag() {
        QFETCH(QPoint, destination);
        QFETCH(QPoint, expected);
        QFETCH(QList<QRect>, areas);
        Window window;
        window.setFlags(Qt::Window | Qt::FramelessWindowHint);
        window.resize(300, 200);
        const QPoint cursor(700, 500); // Deterministic input; no desktop pointer access.
        const QPoint initial = cursor - QPoint(100, 20);
        window.setPosition(initial);
        auto manipulator = new Manipulator(&window, cursor);
        QPointer<Manipulator> guard(manipulator);
        manipulator->areas = {QRect(0, -900, 1000, 800)};
        mouse(&window, QEvent::MouseMove, cursor + destination - initial);
        QCOMPARE(window.position(), destination);
        QCOMPARE(window.moves, 0); // The production filter consumed the move.
        QCOMPARE(manipulator->queries, 0);
        manipulator->areas = areas; // Screen/work-area changes are read at release.
        mouse(&window, QEvent::MouseButtonRelease, cursor + destination - initial);
        QCOMPARE(window.position(), expected);
        QCOMPARE(window.releases, 1); // Release still reaches the window.
        QCOMPARE(manipulator->queries, 1);
        mouse(&window, QEvent::MouseMove, cursor + QPoint(600, 600));
        QCOMPARE(window.position(), expected);
        QCOMPARE(window.moves, 1); // Completed filter no longer intercepts input.
        mouse(&window, QEvent::MouseButtonRelease, cursor);
        QCOMPARE(manipulator->queries, 1);
        QCOMPARE(window.releases, 2);
        QCoreApplication::sendPostedEvents(manipulator, QEvent::DeferredDelete);
        QVERIFY(guard.isNull());
    }

    void realScreenProvider() {
        Window window;
        window.setFlags(Qt::Window | Qt::FramelessWindowHint);
        window.resize(300, 200);
        QVERIFY(window.screen());
        const QRect available = window.screen()->availableGeometry();
        QVERIFY(available.width() > 200 && available.height() > 100);
        const QPoint cursor(700, 500);
        const QPoint initial = cursor - QPoint(100, 20);
        window.setPosition(initial);
        auto manipulator = new QWK::WindowMoveManipulator(&window, cursor);
        QPointer<QObject> guard(manipulator);
        const QPoint destination = available.topLeft() + QPoint(30, 30);
        mouse(&window, QEvent::MouseMove, cursor + destination - initial);
        mouse(&window, QEvent::MouseButtonRelease, cursor + destination - initial);
        QCOMPARE(window.position(), destination);
        QCOMPARE(window.releases, 1);
        QCoreApplication::sendPostedEvents(manipulator, QEvent::DeferredDelete);
        QVERIFY(guard.isNull());
    }

    void destructionDuringDrag() {
        auto window = new Window;
        QPointer<QObject> manipulator = new Manipulator(window);
        delete window;
        QVERIFY(manipulator.isNull());
    }
};

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    WindowMoveTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_windowmove.moc"
