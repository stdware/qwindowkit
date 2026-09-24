// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <functional>
#include <memory>
#include <QtTest/QTest>
#include <QWKCore/private/qwkglobal_p.h>

namespace {
    class Filter : public QObject {
    public:
        std::function<bool(QObject *, QEvent *)> callback;
        int calls = 0;
        bool eventFilter(QObject *receiver, QEvent *event) override {
            ++calls;
            return callback ? callback(receiver, event) : false;
        }
    };
}

class ObjectEventFiltersTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void forwardsOnlyRemainingFiltersInQtOrder() {
        QObject receiver;
        Filter first, second, current, ahead;
        receiver.installEventFilter(&first);
        receiver.installEventFilter(&second);
        receiver.installEventFilter(&current);
        receiver.installEventFilter(&ahead);
        QEvent event(QEvent::User);
        QList<int> order;
        QObject *seenReceiver = nullptr;
        QEvent *seenEvent = nullptr;
        first.callback = [&](QObject *obj, QEvent *ev) {
            order.append(1);
            seenReceiver = obj;
            seenEvent = ev;
            return false;
        };
        second.callback = [&](QObject *, QEvent *) { order.append(2); return false; };
        QVERIFY(!QWK::forwardObjectEventFilters(&current, &receiver, &event));
        QCOMPARE(order, QList<int>({2, 1}));
        QCOMPARE(seenReceiver, &receiver);
        QCOMPARE(seenEvent, &event);
        QCOMPARE(current.calls, 0);
        QCOMPARE(ahead.calls, 0);
        QCOMPARE(first.calls, 1);
        QCOMPARE(second.calls, 1);
    }

    void consumptionStopsForwarding() {
        QObject receiver;
        Filter remaining, consuming, current;
        receiver.installEventFilter(&remaining);
        receiver.installEventFilter(&consuming);
        receiver.installEventFilter(&current);
        consuming.callback = [](QObject *, QEvent *event) { event->accept(); return true; };
        QEvent event(QEvent::User);
        event.setAccepted(false);
        QVERIFY(QWK::forwardObjectEventFilters(&current, &receiver, &event));
        QVERIFY(event.isAccepted());
        QCOMPARE(consuming.calls, 1);
        QCOMPARE(remaining.calls, 0);
        QCOMPARE(current.calls, 0);
    }

    void missingOrLastCurrentFilterDoesNothing() {
        QObject receiver;
        Filter current, other;
        QEvent event(QEvent::User);
        QVERIFY(!QWK::forwardObjectEventFilters(&current, &receiver, &event));
        receiver.installEventFilter(&other);
        QVERIFY(!QWK::forwardObjectEventFilters(&current, &receiver, &event));
        QCOMPARE(other.calls, 0);
        receiver.removeEventFilter(&other);
        receiver.installEventFilter(&current);
        receiver.installEventFilter(&other);
        QVERIFY(!QWK::forwardObjectEventFilters(&current, &receiver, &event));
        QCOMPARE(current.calls, 0);
        QCOMPARE(other.calls, 0);
    }

    void destroyedFiltersAreSkipped() {
        QObject receiver;
        Filter remaining, current;
        auto removed = std::make_unique<Filter>();
        receiver.installEventFilter(&remaining);
        receiver.installEventFilter(removed.get());
        receiver.installEventFilter(&current);
        removed.reset();
        QEvent event(QEvent::User);
        QVERIFY(!QWK::forwardObjectEventFilters(&current, &receiver, &event));
        QCOMPARE(remaining.calls, 1);
        QCOMPARE(current.calls, 0);
    }

    void applicationReceiverIsExcluded() {
        Filter remaining, current;
        qApp->installEventFilter(&remaining);
        qApp->installEventFilter(&current);
        QEvent event(QEvent::User);
        QVERIFY(!QWK::forwardObjectEventFilters(&current, qApp, &event));
        QCOMPARE(remaining.calls, 0);
        QCOMPARE(current.calls, 0);
    }
};

QTEST_GUILESS_MAIN(ObjectEventFiltersTest)

#include "tst_objecteventfilters.moc"
