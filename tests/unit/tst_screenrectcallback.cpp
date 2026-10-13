// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <QtTest/QTest>
#include <QWKCore/private/screenrectcallback_p.h>

using QWK::Private::ScreenRectCallbackState;

class ScreenRectCallbackTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void empty() {
        ScreenRectCallbackState state;
        QVERIFY(!state);
        QVERIFY(state.snapshot()(QSize(400, 30)).isEmpty());
        state.set({});
        QVERIFY(!state);
    }

    void resultAndSize() {
        ScreenRectCallbackState state;
        QSize observed;
        state.set([&](const QSize &size) {
            observed = size;
            return QRect(10, 20, 100, 30);
        });
        QVERIFY(state);
        QCOMPARE(state.snapshot()(QSize(400, 30)), QRect(10, 20, 100, 30));
        QCOMPARE(observed, QSize(400, 30));
    }

    void replacementDuringCall() {
        ScreenRectCallbackState state;
        state.set([&](const QSize &) {
            state.set([](const QSize &) { return QRect(20, 0, 80, 30); });
            return QRect(10, 0, 80, 30);
        });
        QVERIFY(state.snapshot()(QSize(400, 30)).isEmpty());
        QCOMPARE(state.snapshot()(QSize(400, 30)), QRect(20, 0, 80, 30));
    }

    void nestedReplacement() {
        ScreenRectCallbackState state;
        QRect nestedResult;
        state.set([&](const QSize &size) {
            state.set([](const QSize &) { return QRect(30, 0, 80, 30); });
            nestedResult = state.snapshot()(size);
            return QRect(10, 0, 80, 30);
        });
        QVERIFY(state.snapshot()(QSize(400, 30)).isEmpty());
        QCOMPARE(nestedResult, QRect(30, 0, 80, 30));
    }

    void clearDuringCall() {
        ScreenRectCallbackState state;
        state.set([&](const QSize &) {
            state.set({});
            return QRect(10, 0, 80, 30);
        });
        QVERIFY(state.snapshot()(QSize(400, 30)).isEmpty());
        QVERIFY(!state);
    }

    void ownerDestroyedDuringCall() {
        auto state = std::make_unique<ScreenRectCallbackState>();
        state->set([&](const QSize &) {
            state.reset();
            return QRect(10, 0, 80, 30);
        });
        QVERIFY(state->snapshot()(QSize(400, 30)).isEmpty());
        QVERIFY(!state);
    }

    void callableLivesUntilReturn() {
        ScreenRectCallbackState state;
        auto payload = std::make_shared<int>(42);
        const std::weak_ptr<int> weak = payload;
        bool survivedReplacement = false;
        state.set([&, payload](const QSize &) {
            state.set({});
            survivedReplacement = !weak.expired() && *payload == 42;
            return QRect(10, 0, 80, 30);
        });
        payload.reset();
        QVERIFY(state.snapshot()(QSize(400, 30)).isEmpty());
        QVERIFY(survivedReplacement);
        QVERIFY(weak.expired());
    }

    void supersededSnapshotIsNotInvoked() {
        ScreenRectCallbackState state;
        int calls = 0;
        state.set([&](const QSize &) { ++calls; return QRect(10, 0, 80, 30); });
        const auto stale = state.snapshot();
        state.set({});
        QVERIFY(!stale.isCurrent());
        QVERIFY(stale(QSize(400, 30)).isEmpty());
        QCOMPARE(calls, 0);
    }
};

QTEST_GUILESS_MAIN(ScreenRectCallbackTest)
#include "tst_screenrectcallback.moc"
