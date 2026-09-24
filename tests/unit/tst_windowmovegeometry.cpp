// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <QtTest/QTest>
#include <QWKCore/private/systemwindow_p.h>

class WindowMoveGeometryTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void reachablePosition_data() {
        QTest::addColumn<QPoint>("position");
        QTest::addColumn<QPoint>("grab");
        QTest::addColumn<QList<QRect>>("areas");
        QTest::addColumn<QPoint>("expected");
        const QRect origin(0, 0, 1000, 800), above(0, -900, 1000, 800);
        const QRect left(-1200, 0, 1000, 800);
        const QPoint grab(100, 20);
        const auto row = [&](const char *name, QPoint pos, QList<QRect> areas, QPoint expected) {
            QTest::newRow(name) << pos << grab << areas << expected;
        };
        row("origin", {100, 100}, {origin}, {100, 100});
        row("above-negative-y", {100, -700}, {origin, above}, {100, -700});
        row("left-negative-x", {-1000, 100}, {origin, left}, {-1000, 100});
        row("above-top-boundary", {100, -910}, {origin, above}, {100, -904});
        row("top-inset", {100, -10}, {origin}, {100, -4});
        row("left-inset", {-150, 100}, {origin}, {-84, 100});
        row("right-inset", {900, 100}, {origin}, {883, 100});
        row("bottom-inset", {100, 790}, {origin}, {100, 763});
        row("corner-minimal", {900, -10}, {origin}, {883, -4});
        row("vertical-gap-nearest-above", {100, -80}, {origin, above}, {100, -137});
        row("horizontal-gap-nearest-origin", {-200, 100}, {left, origin}, {-84, 100});
        row("screen-order-independent", {100, -700}, {above, origin}, {100, -700});
        row("removed-upper-screen", {100, -700}, {origin}, {100, -4});
        row("reserved-top-area", {100, 0}, {QRect(0, 40, 1000, 760)}, {100, 36});
        row("missing-screens", {100, -700}, {}, {100, -700});
        row("invalid-screens", {100, -700}, {QRect(), QRect(0, 0, -1, 5)}, {100, -700});
        row("ignore-invalid-screen", {100, -700}, {QRect(), above}, {100, -700});
        row("tiny-screen", {0, 0}, {QRect(10, 20, 5, 3)}, {-88, 1});
        row("single-pixel-screen", {0, 0}, {QRect(10, 20, 1, 1)}, {-90, 0});
        QTest::newRow("oversized-window") << QPoint(-1000, 100) << QPoint(1500, 20)
                                          << QList<QRect>{origin} << QPoint(-1000, 100);
        QTest::newRow("custom-title-below-top") << QPoint(100, -100) << QPoint(100, 150)
                                              << QList<QRect>{origin} << QPoint(100, -100);
        QTest::newRow("tie-prefers-first-screen") << QPoint(0, 0) << QPoint(0, 0)
            << QList<QRect>{QRect(-100, -50, 85, 101), QRect(16, -50, 85, 101)}
            << QPoint(-32, 0);
    }

    void reachablePosition() {
        QFETCH(QPoint, position);
        QFETCH(QPoint, grab);
        QFETCH(QList<QRect>, areas);
        QFETCH(QPoint, expected);
        QCOMPARE(QWK::reachableWindowPosition(position, grab, areas), expected);
        // Releasing again must not keep nudging the window.
        QCOMPARE(QWK::reachableWindowPosition(expected, grab, areas), expected);
    }
};

QTEST_GUILESS_MAIN(WindowMoveGeometryTest)
#include "tst_windowmovegeometry.moc"
