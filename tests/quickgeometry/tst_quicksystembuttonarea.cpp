// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <QtGui/QGuiApplication>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>
#include <QWKQuick/private/quicksystembuttonarea_p.h>
#include <memory>

namespace {
    struct Fixture {
        QQuickWindow window;
        QQuickItem grandparent{window.contentItem()};
        QQuickItem parent{&grandparent};
        QQuickItem item{&parent};
        Fixture() {
            window.resize(500, 400);
            grandparent.setTransformOrigin(QQuickItem::TopLeft);
            parent.setTransformOrigin(QQuickItem::TopLeft);
            parent.setSize({200, 100});
            parent.setPosition({100, 50});
            item.setTransformOrigin(QQuickItem::TopLeft);
            item.setPosition({10, 20});
            item.setSize({100, 40});
        }
    };
}

class QuickSystemButtonAreaTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void geometry_data() {
        QTest::addColumn<QString>("change");
        QTest::addColumn<QRect>("expected");
        QTest::newRow("identity") << QString("identity") << QRect(110, 70, 100, 40);
        QTest::newRow("item-translation") << QString("item-translation") << QRect(130, 90, 100, 40);
        QTest::newRow("parent-translation") << QString("parent-translation") << QRect(210, 120, 100, 40);
        QTest::newRow("item-scale") << QString("item-scale") << QRect(110, 70, 200, 80);
        QTest::newRow("parent-scale") << QString("parent-scale") << QRect(120, 90, 200, 80);
        QTest::newRow("item-rotation") << QString("item-rotation") << QRect(70, 70, 40, 100);
        QTest::newRow("parent-rotation") << QString("parent-rotation") << QRect(40, 60, 40, 100);
        QTest::newRow("nested-scale-translation") << QString("nested") << QRect(250, 180, 200, 80);
        QTest::newRow("mirror") << QString("mirror") << QRect(10, 30, 100, 40);
        QTest::newRow("item-center-origin") << QString("item-origin") << QRect(60, 50, 200, 80);
        QTest::newRow("parent-center-origin") << QString("parent-origin") << QRect(20, 40, 200, 80);
        QTest::newRow("fractional-rounding") << QString("fractional") << QRect(111, 71, 100, 40);
        QTest::newRow("size") << QString("size") << QRect(110, 70, 140, 60);
    }

    void geometry() {
        QFETCH(QString, change);
        QFETCH(QRect, expected);
        Fixture f;
        QWK::QuickSystemButtonArea area(&f.item);
        QSignalSpy spy(&area, &QWK::QuickSystemButtonArea::changed);
        QRect notified;
        connect(&area, &QWK::QuickSystemButtonArea::changed, this,
                [&] { notified = area.sceneRect(&f.window); });
        if (change == "item-translation") f.item.setPosition({30, 40});
        if (change == "parent-translation") f.parent.setPosition({200, 100});
        if (change == "item-scale") f.item.setScale(2);
        if (change == "parent-scale") f.parent.setScale(2);
        if (change == "item-rotation") f.item.setRotation(90);
        if (change == "parent-rotation") f.parent.setRotation(90);
        if (change == "nested") {
            f.grandparent.setPosition({30, 40});
            f.grandparent.setScale(2);
        }
        if (change == "mirror") f.item.setScale(-1);
        if (change == "item-origin") {
            f.item.setScale(2);
            f.item.setTransformOrigin(QQuickItem::Center);
        }
        if (change == "parent-origin") {
            f.parent.setScale(2);
            f.parent.setTransformOrigin(QQuickItem::Center);
        }
        if (change == "fractional") f.item.setPosition({10.75, 20.75});
        if (change == "size") f.item.setSize({140, 60});
        // The native callback always reads current geometry, even before queued notification.
        QCOMPARE(area.sceneRect(&f.window), expected);
        if (change != "identity") {
            QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 1000);
            QCOMPARE(notified, expected);
        } else {
            QCoreApplication::processEvents();
            QCOMPARE(spy.count(), 0);
        }
        QCOMPARE(area.sceneRect(&f.window).center(), expected.center());
    }

    void resizedAncestorChangesTransformOrigin() {
        Fixture f;
        f.parent.setTransformOrigin(QQuickItem::Center);
        f.parent.setScale(2);
        QWK::QuickSystemButtonArea area(&f.item);
        QSignalSpy spy(&area, &QWK::QuickSystemButtonArea::changed);
        f.parent.setSize({300, 200});
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 1000);
        QCOMPARE(area.sceneRect(&f.window), QRect(-30, -10, 200, 80));
    }

    void reparentAndDisconnectOldAncestors() {
        Fixture f;
        QQuickItem replacement(f.window.contentItem());
        replacement.setPosition({300, 100});
        QWK::QuickSystemButtonArea area(&f.item);
        QSignalSpy spy(&area, &QWK::QuickSystemButtonArea::changed);
        for (int i = 0; i < 3; ++i) {
            f.item.setParentItem(&replacement);
            QTRY_COMPARE_WITH_TIMEOUT(spy.count(), i * 2 + 1, 1000);
            QCOMPARE(area.sceneRect(&f.window), QRect(310, 120, 100, 40));
            f.item.setParentItem(&f.parent);
            QTRY_COMPARE_WITH_TIMEOUT(spy.count(), i * 2 + 2, 1000);
            QCOMPARE(area.sceneRect(&f.window), QRect(110, 70, 100, 40));
        }
        replacement.setX(400);
        QCoreApplication::processEvents();
        QCOMPARE(spy.count(), 6);
        f.parent.setX(120);
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 7, 1000);
        QCOMPARE(area.sceneRect(&f.window), QRect(130, 70, 100, 40));
    }

    void ancestorReparent() {
        Fixture f;
        QQuickItem replacement(f.window.contentItem());
        replacement.setPosition({30, 40});
        QWK::QuickSystemButtonArea area(&f.item);
        QSignalSpy spy(&area, &QWK::QuickSystemButtonArea::changed);
        f.parent.setParentItem(&replacement);
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 1000);
        QCOMPARE(area.sceneRect(&f.window), QRect(140, 110, 100, 40));
        f.grandparent.setScale(5);
        QCoreApplication::processEvents();
        QCOMPARE(spy.count(), 1);
        replacement.setScale(2);
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 1000);
        QCOMPARE(area.sceneRect(&f.window), QRect(250, 180, 200, 80));
    }

    void windowSwitchAndDetach() {
        Fixture f;
        QQuickWindow other;
        QWK::QuickSystemButtonArea area(&f.item);
        QSignalSpy spy(&area, &QWK::QuickSystemButtonArea::changed);
        f.grandparent.setParentItem(other.contentItem());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 1000);
        QCOMPARE(area.sceneRect(&f.window), QRect());
        QCOMPARE(area.sceneRect(&other), QRect(110, 70, 100, 40));
        f.grandparent.setParentItem(nullptr);
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 1000);
        QCOMPARE(area.sceneRect(&other), QRect());
        QCOMPARE(area.sceneRect(nullptr), QRect());
        f.grandparent.setParentItem(f.window.contentItem());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 3, 1000);
        QCOMPARE(area.sceneRect(&f.window), QRect(110, 70, 100, 40));
    }

    void areaDestruction() {
        QQuickWindow window;
        auto item = new QQuickItem(window.contentItem());
        item->setSize({100, 40});
        QWK::QuickSystemButtonArea area(item);
        QSignalSpy spy(&area, &QWK::QuickSystemButtonArea::changed);
        delete item;
        QVERIFY(!area.item());
        QCOMPARE(area.sceneRect(&window), QRect());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 1000);
    }

    void ancestorDestructionWithSurvivingItem() {
        QQuickWindow window;
        auto parent = new QQuickItem(window.contentItem());
        QQuickItem item;
        item.setSize({100, 40});
        item.setParentItem(parent); // Deliberately different QObject ownership.
        QWK::QuickSystemButtonArea area(&item);
        QSignalSpy spy(&area, &QWK::QuickSystemButtonArea::changed);
        delete parent;
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 1000);
        QCOMPARE(area.sceneRect(&window), QRect());
        item.setParentItem(window.contentItem());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 1000);
        QCOMPARE(area.sceneRect(&window), QRect(0, 0, 100, 40));
    }

    void windowDestruction() {
        auto window = new QQuickWindow;
        QQuickItem item;
        item.setSize({100, 40});
        item.setParentItem(window->contentItem());
        QWK::QuickSystemButtonArea area(&item);
        QSignalSpy spy(&area, &QWK::QuickSystemButtonArea::changed);
        delete window;
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 1000);
        QVERIFY(!item.window());
        QCOMPARE(area.sceneRect(nullptr), QRect());
    }

    void destroyObserverWithPendingRefresh() {
        Fixture f;
        int notifications = 0;
        auto area = std::make_unique<QWK::QuickSystemButtonArea>(&f.item);
        connect(area.get(), &QWK::QuickSystemButtonArea::changed, this, [&] { ++notifications; });
        f.item.setX(50);
        area.reset();
        QCoreApplication::processEvents();
        f.parent.setScale(2);
        QCoreApplication::processEvents();
        QCOMPARE(notifications, 0);
    }

    void destroyObserverDuringNotification() {
        Fixture f;
        auto area = std::make_unique<QWK::QuickSystemButtonArea>(&f.item);
        int notifications = 0;
        connect(area.get(), &QWK::QuickSystemButtonArea::changed, this, [&] {
            ++notifications;
            area.reset();
        });
        f.item.setX(50);
        QTRY_VERIFY_WITH_TIMEOUT(!area, 1000);
        QCOMPARE(notifications, 1);
    }

    void reparentDuringNotification() {
        Fixture f;
        QQuickItem replacement(f.window.contentItem());
        replacement.setPosition({300, 100});
        QWK::QuickSystemButtonArea area(&f.item);
        int notifications = 0;
        QRect last;
        connect(&area, &QWK::QuickSystemButtonArea::changed, this, [&] {
            last = area.sceneRect(&f.window);
            if (++notifications == 1)
                f.item.setParentItem(&replacement);
        });
        f.item.setX(20);
        QTRY_COMPARE_WITH_TIMEOUT(notifications, 2, 1000);
        QCOMPARE(last, QRect(320, 120, 100, 40));
    }

    void registrationBeforeNativeWindow() {
        QQuickWindow window;
        QQuickItem item;
        item.setSize({100, 40});
        item.setTransformOrigin(QQuickItem::TopLeft);
        QWK::QuickSystemButtonArea area(&item);
        QSignalSpy spy(&area, &QWK::QuickSystemButtonArea::changed);
        QVERIFY(!window.handle());
        QCOMPARE(area.sceneRect(&window), QRect());
        item.setScale(2);
        item.setPosition({30, 40});
        item.setParentItem(window.contentItem());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 1000);
        QCOMPARE(area.sceneRect(&window), QRect(30, 40, 200, 80));
        QVERIFY(!window.handle()); // Geometry updates must not realize a native window.
    }

    void retainedCallbackAfterReplacement() {
        Fixture f;
        auto area = std::make_unique<QWK::QuickSystemButtonArea>(&f.item);
        // Same guarded capture contract used by the macOS adapter.
        const auto oldCallback = [guard = QPointer<QWK::QuickSystemButtonArea>(area.get()),
                                  window = QPointer<QQuickWindow>(&f.window)] {
            return guard ? guard->sceneRect(window) : QRect();
        };
        QCOMPARE(oldCallback(), QRect(110, 70, 100, 40));
        area.reset();
        QCOMPARE(oldCallback(), QRect());
        area = std::make_unique<QWK::QuickSystemButtonArea>(&f.item);
        QCOMPARE(oldCallback(), QRect());
        QCOMPARE(area->sceneRect(&f.window), QRect(110, 70, 100, 40));
    }

    void qmlTransformList_data() {
        QTest::addColumn<bool>("ancestor");
        QTest::newRow("item") << false;
        QTest::newRow("ancestor") << true;
    }

    void qmlTransformList() {
        QFETCH(bool, ancestor);
        QQuickWindow window;
        window.resize(500, 400);
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick 2.15
            Item {
                width: 300; height: 200; x: 100; y: 50
                property bool onAncestor: false
                property bool attached: true
                property alias delta: shift.x
                Translate { id: shift; x: 0 }
                transform: attached && onAncestor ? [shift] : []
                Item {
                    objectName: "area"; x: 10; y: 20; width: 100; height: 40
                    transform: attached && !onAncestor ? [shift] : []
                }
            }
        )", QUrl());
        std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        auto rootItem = qobject_cast<QQuickItem *>(root.get());
        QVERIFY(rootItem);
        rootItem->setProperty("onAncestor", ancestor);
        rootItem->setParentItem(window.contentItem());
        auto item = root->findChild<QQuickItem *>("area");
        QVERIFY(item);
        QWK::QuickSystemButtonArea area(item);
        QSignalSpy spy(&area, &QWK::QuickSystemButtonArea::changed);
        window.show(); // Real offscreen frames drive the production afterAnimating connection.
        QTRY_VERIFY_WITH_TIMEOUT(window.isExposed(), 1000);
        QVERIFY(root->setProperty("delta", 30));
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 1000);
        QCOMPARE(area.sceneRect(&window), QRect(140, 70, 100, 40));
        QVERIFY(root->setProperty("attached", false));
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 1000);
        QCOMPARE(area.sceneRect(&window), QRect(110, 70, 100, 40));
        QVERIFY(root->setProperty("attached", true));
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 3, 1000);
        QCOMPARE(area.sceneRect(&window), QRect(140, 70, 100, 40));
        QVERIFY(root->setProperty("delta", -20));
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 4, 1000);
        QCOMPARE(area.sceneRect(&window), QRect(90, 70, 100, 40));
    }
};

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QuickSystemButtonAreaTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_quicksystembuttonarea.moc"
