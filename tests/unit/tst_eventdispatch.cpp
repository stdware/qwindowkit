// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <functional>
#include <memory>

#include <QtCore/QAbstractEventDispatcher>
#include <QtTest/QTest>
#include <QWKCore/private/nativeeventfilter_p.h>
#include <QWKCore/private/sharedeventfilter_p.h>

namespace {
    // The adapters only select the production dispatcher. All expected call sequences below
    // are literal values, independent of the dispatcher's list/depth implementation.
    class Filter : public QWK::SharedEventFilter, public QWK::NativeEventFilter {
    public:
        std::function<bool()> callback = [] { return false; };
        QObject *objectSeen = nullptr;
        QEvent *eventSeen = nullptr;
        QByteArray typeSeen;
        void *messageSeen = nullptr;
        QT_NATIVE_EVENT_RESULT_TYPE *resultSeen = nullptr;

        bool sharedEventFilter(QObject *object, QEvent *event) override {
            objectSeen = object;
            eventSeen = event;
            auto action = callback; // A callback may destroy this filter, including callback.
            return action();
        }
        bool nativeEventFilter(const QByteArray &type, void *message,
                               QT_NATIVE_EVENT_RESULT_TYPE *result) override {
            typeSeen = type;
            messageSeen = message;
            resultSeen = result;
            auto action = callback;
            return action();
        }
        bool detached() const { return !m_sharedDispatcher && !m_nativeDispatcher; }
    };

    class Dispatcher {
    public:
        explicit Dispatcher(bool native) : native(native) {}
        void install(Filter *filter) {
            if (native)
                nativeDispatcher.installNativeEventFilter(filter);
            else
                sharedDispatcher.installSharedEventFilter(filter);
        }
        void remove(Filter *filter) {
            if (native)
                nativeDispatcher.removeNativeEventFilter(filter);
            else
                sharedDispatcher.removeSharedEventFilter(filter);
        }
        bool dispatch() {
            return native ? nativeDispatcher.nativeDispatch("unit-test", &message, &result)
                          : sharedDispatcher.sharedDispatch(&object, &event);
        }
        QObject object;
        QEvent event{QEvent::User};
        int message = 17;
        QT_NATIVE_EVENT_RESULT_TYPE result = 0;

    private:
        bool native;
        QWK::NativeEventDispatcher nativeDispatcher;
        QWK::SharedEventDispatcher sharedDispatcher;
    };

    void dispatcherRows() {
        QTest::addColumn<bool>("native");
        QTest::newRow("shared") << false;
        QTest::newRow("native") << true;
    }

    class AppFilter : public QWK::AppNativeEventFilter {
    public:
        std::function<bool()> callback;
        bool nativeEventFilter(const QByteArray &type, void *, QT_NATIVE_EVENT_RESULT_TYPE *) override {
            if (type != "qwk-unit-test")
                return false;
            auto action = callback;
            return action();
        }
    };

    bool dispatchAppEvent() {
        QT_NATIVE_EVENT_RESULT_TYPE result = 0;
        int message = 0;
        return QAbstractEventDispatcher::instance()->filterNativeEvent("qwk-unit-test", &message, &result);
    }
}

class EventDispatchTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void argumentsAndEmpty_data() { dispatcherRows(); }
    void argumentsAndEmpty() {
        QFETCH(bool, native);
        Dispatcher dispatcher(native);
        QVERIFY(!dispatcher.dispatch());
        dispatcher.install(nullptr);
        Filter filter;
        dispatcher.install(&filter);
        filter.callback = [&] {
            if (native)
                *filter.resultSeen = 73;
            return true;
        };
        QVERIFY(dispatcher.dispatch());
        if (native) {
            QCOMPARE(filter.typeSeen, QByteArray("unit-test"));
            QCOMPARE(filter.messageSeen, static_cast<void *>(&dispatcher.message));
            QCOMPARE(filter.resultSeen, &dispatcher.result);
            QCOMPARE(dispatcher.result, QT_NATIVE_EVENT_RESULT_TYPE(73));
        } else {
            QCOMPARE(filter.objectSeen, &dispatcher.object);
            QCOMPARE(filter.eventSeen, &dispatcher.event);
        }
    }

    void orderAndConsumption_data() { dispatcherRows(); }
    void orderAndConsumption() {
        QFETCH(bool, native);
        Dispatcher dispatcher(native);
        Filter a, b, c;
        QString calls;
        bool consume = true;
        a.callback = [&] { calls += 'A'; return false; };
        b.callback = [&] { calls += 'B'; return consume; };
        c.callback = [&] { calls += 'C'; return false; };
        for (auto *filter : {&a, &b, &c})
            dispatcher.install(filter);
        QVERIFY(dispatcher.dispatch());
        QCOMPARE(calls, QStringLiteral("AB"));
        calls.clear();
        consume = false;
        QVERIFY(!dispatcher.dispatch());
        QCOMPARE(calls, QStringLiteral("ABC"));
    }

    void registrationAndTransfer_data() { dispatcherRows(); }
    void registrationAndTransfer() {
        QFETCH(bool, native);
        Dispatcher first(native), second(native);
        Filter filter;
        int calls = 0;
        filter.callback = [&] { ++calls; return false; };
        first.install(&filter);
        first.install(&filter);
        second.install(&filter); // One dispatcher owns the registration at a time.
        second.remove(&filter);  // Removing a foreign filter must leave it attached.
        QVERIFY(!filter.detached());
        first.dispatch();
        second.dispatch();
        QCOMPARE(calls, 1);
        first.remove(&filter);
        first.remove(&filter);
        QVERIFY(filter.detached());
        second.install(&filter);
        first.dispatch();
        second.dispatch();
        QCOMPARE(calls, 2);
    }

    void removeSelf_data() { dispatcherRows(); }
    void removeSelf() {
        QFETCH(bool, native);
        Dispatcher dispatcher(native);
        Filter a, b;
        QString calls;
        a.callback = [&] { calls += 'A'; dispatcher.remove(&a); return false; };
        b.callback = [&] { calls += 'B'; return false; };
        dispatcher.install(&a);
        dispatcher.install(&b);
        QVERIFY(!dispatcher.dispatch());
        QCOMPARE(calls, QStringLiteral("AB"));
        QVERIFY(a.detached());
        calls.clear();
        dispatcher.dispatch();
        QCOMPARE(calls, QStringLiteral("B"));
    }

    void destroyNext_data() { dispatcherRows(); }
    void destroyNext() {
        QFETCH(bool, native);
        Dispatcher dispatcher(native);
        Filter a, c;
        auto b = std::make_unique<Filter>();
        QString calls;
        a.callback = [&] { calls += 'A'; b.reset(); return false; };
        b->callback = [&] { calls += 'B'; return false; };
        c.callback = [&] { calls += 'C'; return false; };
        dispatcher.install(&a);
        dispatcher.install(b.get());
        dispatcher.install(&c);
        dispatcher.dispatch();
        QCOMPARE(calls, QStringLiteral("AC"));
        calls.clear();
        dispatcher.dispatch();
        QCOMPARE(calls, QStringLiteral("AC"));
    }

    void destroySelfAndConsume_data() { dispatcherRows(); }
    void destroySelfAndConsume() {
        QFETCH(bool, native);
        Dispatcher dispatcher(native);
        auto a = std::make_unique<Filter>();
        Filter b;
        QString calls;
        a->callback = [&] { calls += 'A'; a.reset(); return true; };
        b.callback = [&] { calls += 'B'; return false; };
        dispatcher.install(a.get());
        dispatcher.install(&b);
        QVERIFY(dispatcher.dispatch());
        QCOMPARE(calls, QStringLiteral("A"));
        calls.clear();
        QVERIFY(!dispatcher.dispatch());
        QCOMPARE(calls, QStringLiteral("B"));
    }

    void appendDuringDispatch_data() { dispatcherRows(); }
    void appendDuringDispatch() {
        QFETCH(bool, native);
        Dispatcher dispatcher(native);
        Filter a, b, c;
        QString calls;
        a.callback = [&] { calls += 'A'; dispatcher.install(&c); return false; };
        b.callback = [&] { calls += 'B'; return false; };
        c.callback = [&] { calls += 'C'; return false; };
        dispatcher.install(&a);
        dispatcher.install(&b);
        dispatcher.dispatch();
        QCOMPARE(calls, QStringLiteral("ABC"));
        calls.clear();
        dispatcher.dispatch();
        QCOMPARE(calls, QStringLiteral("ABC"));
    }

    void nestedRemoval_data() { dispatcherRows(); }
    void nestedRemoval() {
        QFETCH(bool, native);
        Dispatcher dispatcher(native);
        Filter a, b, c;
        QString calls;
        bool nested = false;
        bool innerConsumed = false;
        a.callback = [&] {
            calls += nested ? 'a' : 'A';
            if (!nested) {
                nested = true;
                innerConsumed = dispatcher.dispatch();
                nested = false;
            }
            return false;
        };
        b.callback = [&] {
            calls += 'b';
            dispatcher.remove(&a);
            dispatcher.remove(&b);
            return true;
        };
        c.callback = [&] { calls += 'C'; return false; };
        for (auto *filter : {&a, &b, &c})
            dispatcher.install(filter);
        QVERIFY(!dispatcher.dispatch());
        QVERIFY(innerConsumed);
        QCOMPARE(calls, QStringLiteral("AabC"));
        QVERIFY(a.detached());
        QVERIFY(b.detached());
        calls.clear();
        dispatcher.dispatch();
        QCOMPARE(calls, QStringLiteral("C"));
    }

    void dispatcherDestroyedFirst_data() { dispatcherRows(); }
    void dispatcherDestroyedFirst() {
        QFETCH(bool, native);
        Filter filter;
        int calls = 0;
        filter.callback = [&] { ++calls; return false; };
        {
            Dispatcher dispatcher(native);
            dispatcher.install(&filter);
            QVERIFY(!filter.detached());
        }
        QVERIFY(filter.detached());
        Dispatcher replacement(native);
        replacement.install(&filter);
        replacement.dispatch();
        QCOMPARE(calls, 1);
    }

    void filterDestroyedFirst_data() { dispatcherRows(); }
    void filterDestroyedFirst() {
        QFETCH(bool, native);
        Dispatcher dispatcher(native);
        int calls = 0;
        {
            Filter filter;
            filter.callback = [&] { ++calls; return true; };
            dispatcher.install(&filter);
            QVERIFY(dispatcher.dispatch());
        }
        QVERIFY(!dispatcher.dispatch());
        QCOMPARE(calls, 1);
    }

    void appFilterUnregisters() {
        int calls = 0;
        {
            AppFilter filter;
            filter.callback = [&] { ++calls; return true; };
            QVERIFY(dispatchAppEvent());
            QCOMPARE(calls, 1);
        }
        QVERIFY(!dispatchAppEvent());
        QCOMPARE(calls, 1);
    }

    void lastAppFilterDestroyedInCallback() {
        auto filter = std::make_unique<AppFilter>();
        int calls = 0;
        filter->callback = [&] { ++calls; filter.reset(); return true; };
        QVERIFY(dispatchAppEvent());
        QVERIFY(!filter);
        QVERIFY(!dispatchAppEvent());
        QCOMPARE(calls, 1);
        {
            AppFilter replacement;
            replacement.callback = [&] { ++calls; return true; };
            QVERIFY(dispatchAppEvent());
            QCOMPARE(calls, 2);
        }
        QVERIFY(!dispatchAppEvent());
    }
};

QTEST_GUILESS_MAIN(EventDispatchTest)
#include "tst_eventdispatch.moc"
