// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <QtGui/QGuiApplication>
#include <QtQuick/QQuickItem>
#include <QtTest/QTest>
#include <QWKQuick/private/quickitemdelegate_p.h>

class QuickGeometryTest : public QObject {
    Q_OBJECT

private:
    QWK::QuickItemDelegate delegate;

private Q_SLOTS:
    void mappedGeometry_data() {
        QTest::addColumn<bool>("ancestor");
        QTest::addColumn<qreal>("scale");
        QTest::addColumn<qreal>("rotation");
        QTest::addColumn<QRect>("expected");
        for (bool ancestor : {false, true}) {
            const QByteArray prefix = ancestor ? "ancestor-" : "item-";
            QTest::newRow(prefix + "scale") << ancestor << qreal(2) << qreal(0) << QRect(100, 100, 200, 80);
            QTest::newRow(prefix + "rotation") << ancestor << qreal(1) << qreal(90) << QRect(60, 100, 40, 100);
            QTest::newRow(prefix + "shrink") << ancestor << qreal(0.5) << qreal(0) << QRect(100, 100, 50, 20);
            QTest::newRow(prefix + "mirror") << ancestor << qreal(-1) << qreal(0) << QRect(0, 60, 100, 40);
        }
    }

    void mappedGeometry() {
        QFETCH(bool, ancestor);
        QFETCH(qreal, scale);
        QFETCH(qreal, rotation);
        QFETCH(QRect, expected);
        QQuickItem parent;
        parent.setPosition(QPointF(100, 100));
        parent.setTransformOrigin(QQuickItem::TopLeft);
        QQuickItem item(&parent);
        item.setSize(QSizeF(100, 40));
        item.setTransformOrigin(QQuickItem::TopLeft);
        auto *transformed = ancestor ? &parent : &item;
        transformed->setScale(scale);
        transformed->setRotation(rotation);
        QCOMPARE(delegate.mapGeometryToScene(&item), expected);
    }

    void rotationRejectsBoundingCorners() {
        QQuickItem item;
        item.setPosition(QPointF(100, 100));
        item.setSize(QSizeF(100, 40));
        item.setTransformOrigin(QQuickItem::TopLeft);
        item.setRotation(45);
        QVERIFY(delegate.containsScenePoint(&item, QPoint(150, 160)));
        // This is inside the bounding box but outside the rotated rectangle.
        QVERIFY(delegate.mapGeometryToScene(&item).contains(QPoint(105, 180)));
        QVERIFY(!delegate.containsScenePoint(&item, QPoint(105, 180)));
    }

    void fractionalBounds() {
        QQuickItem item;
        item.setPosition(QPointF(0.75, 0.75));
        item.setSize(QSizeF(0.5, 0.5));
        QCOMPARE(delegate.mapGeometryToScene(&item), QRect(0, 0, 2, 2));
        QVERIFY(delegate.containsScenePoint(&item, QPoint(1, 1)));
        QVERIFY(!delegate.containsScenePoint(&item, QPoint(0, 0)));
        item.setPosition(QPointF(0, 0));
        item.setSize(QSizeF(100, 40));
        QVERIFY(!delegate.containsScenePoint(&item, QPoint(100, 20)));
        QVERIFY(!delegate.containsScenePoint(&item, QPoint(50, 40)));
    }

    void singularTransforms() {
        QQuickItem parent;
        QQuickItem item(&parent);
        item.setSize(QSizeF(100, 40));
        item.setTransformOrigin(QQuickItem::TopLeft);
        item.setScale(0);
        QVERIFY(!delegate.containsScenePoint(&item, QPoint(10, 10)));
        item.setScale(1);
        parent.setScale(0);
        QVERIFY(!delegate.containsScenePoint(&item, QPoint(10, 10)));
        parent.setScale(1);
        item.setSize(QSizeF(0, 40));
        QVERIFY(!delegate.containsScenePoint(&item, QPoint(0, 10)));
    }

    void dynamicChanges() {
        QQuickItem parent;
        parent.setPosition(QPointF(100, 100));
        parent.setTransformOrigin(QQuickItem::TopLeft);
        QQuickItem item(&parent);
        item.setSize(QSizeF(100, 40));
        QVERIFY(!delegate.containsScenePoint(&item, QPoint(250, 140)));
        parent.setScale(2);
        QVERIFY(delegate.containsScenePoint(&item, QPoint(250, 140)));
        parent.setRotation(90);
        QVERIFY(!delegate.containsScenePoint(&item, QPoint(250, 140)));
        QVERIFY(delegate.containsScenePoint(&item, QPoint(60, 250)));
        parent.setScale(1);
        parent.setRotation(0);
        item.setScale(-1); // Default center origin: mirror inside the original footprint.
        QVERIFY(delegate.containsScenePoint(&item, QPoint(150, 120)));
    }
};

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QuickGeometryTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_quickgeometry.moc"
