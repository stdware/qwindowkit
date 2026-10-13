// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <functional>
#include <new>
#include <type_traits>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryDir>
#include <QtCore/qscopeguard.h>
#include <QtTest/QTest>
#include <QWKCore/private/nativeeventfilter_p.h>
#include <QWKCore/private/sharedeventfilter_p.h>

namespace {
    bool childProcess = false;

    class Filter : public QWK::SharedEventFilter, public QWK::NativeEventFilter {
    public:
        std::function<bool()> callback;
        bool sharedEventFilter(QObject *, QEvent *) override { return callback(); }
        bool nativeEventFilter(const QByteArray &, void *, QT_NATIVE_EVENT_RESULT_TYPE *) override {
            return callback();
        }
        bool detached() const { return !m_sharedDispatcher && !m_nativeDispatcher; }
    };

    template <bool Native>
    void exerciseDestruction(bool nested, bool consume) {
        using Dispatcher = std::conditional_t<Native, QWK::NativeEventDispatcher,
                                             QWK::SharedEventDispatcher>;
        QObject receiver;
        QEvent event(QEvent::User);
        int message = 0;
        QT_NATIVE_EVENT_RESULT_TYPE result = 0;
        auto dispatch = [&](Dispatcher *dispatcher) {
            if constexpr (Native)
                return dispatcher->nativeDispatch("lifetime", &message, &result);
            else
                return dispatcher->sharedDispatch(&receiver, &event);
        };
        auto install = [](Dispatcher *dispatcher, Filter *filter) {
            if constexpr (Native)
                dispatcher->installNativeEventFilter(filter);
            else
                dispatcher->installSharedEventFilter(filter);
        };
        auto remove = [](Dispatcher *dispatcher, Filter *filter) {
            if constexpr (Native)
                dispatcher->removeNativeEventFilter(filter);
            else
                dispatcher->removeSharedEventFilter(filter);
        };

        // Reuse the exact address immediately. This makes stale-frame continuation observable
        // even without an allocator diagnostic: it must never enter the replacement's filters.
        alignas(Dispatcher) unsigned char storage[sizeof(Dispatcher)];
        auto dispatcher = new (storage) Dispatcher;
        const auto cleanup = qScopeGuard([&] { dispatcher->~Dispatcher(); });
        Filter deleting, staleTail, replacementHead, replacementTail;
        QString calls;
        bool inNested = false;
        bool nestedConsumed = false;
        deleting.callback = [&] {
            calls += 'A';
            if (nested && !inNested) {
                inNested = true;
                nestedConsumed = dispatch(dispatcher);
                return consume;
            }
            dispatcher->~Dispatcher();
            dispatcher = new (storage) Dispatcher;
            install(dispatcher, &replacementHead);
            install(dispatcher, &replacementTail);
            return consume;
        };
        staleTail.callback = [&] { calls += 'S'; return false; };
        replacementHead.callback = [&] {
            calls += 'B';
            remove(dispatcher, &replacementHead);
            return false;
        };
        replacementTail.callback = [&] { calls += 'C'; return false; };
        install(dispatcher, &deleting);
        install(dispatcher, &staleTail);

        const bool consumed = dispatch(dispatcher);
        QCOMPARE(calls, nested ? QStringLiteral("AA") : QStringLiteral("A"));
        QVERIFY(consumed); // A destroyed dispatcher always stops delivery.
        if (nested)
            QVERIFY(nestedConsumed);
        QVERIFY(deleting.detached());
        QVERIFY(staleTail.detached());
        calls.clear();
        QVERIFY(!dispatch(dispatcher));
        QCOMPARE(calls, QStringLiteral("BC"));
    }
}

class DispatchLifetimeTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void destruction_data() {
        QTest::addColumn<bool>("native");
        QTest::addColumn<bool>("nested");
        QTest::addColumn<bool>("consume");
        for (bool native : {false, true}) {
            for (bool nested : {false, true}) {
                for (bool consume : {false, true}) {
                    const auto name = QString("%1-%2-%3")
                        .arg(native ? "native" : "shared", nested ? "nested" : "direct",
                             consume ? "consume" : "pass").toLatin1();
                    QTest::newRow(name.constData()) << native << nested << consume;
                }
            }
        }
    }

    void destruction() {
        if (!childProcess) {
            QTemporaryDir reports;
            QVERIFY(reports.isValid());
            const auto reportPath = reports.filePath("child.txt");
            QProcess child;
            child.setProcessChannelMode(QProcess::MergedChannels);
            child.start(QCoreApplication::applicationFilePath(),
                        {"--child", QString("destruction:%1").arg(QTest::currentDataTag()),
                         "-o", reportPath + ",txt"});
            QVERIFY(child.waitForStarted(2000));
            const bool finished = child.waitForFinished(5000);
            if (!finished) {
                child.kill();
                child.waitForFinished(1000);
            }
            QFile report(reportPath);
            const auto output = child.readAll() + (report.open(QIODevice::ReadOnly)
                ? report.readAll() : report.errorString().toUtf8());
            QVERIFY2(finished, output.constData());
            QVERIFY2(child.exitStatus() == QProcess::NormalExit && child.exitCode() == 0,
                     output.constData());
            return;
        }
        QFETCH(bool, native);
        QFETCH(bool, nested);
        QFETCH(bool, consume);
        if (native)
            exerciseDestruction<true>(nested, consume);
        else
            exerciseDestruction<false>(nested, consume);
    }
};

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    auto arguments = app.arguments();
    childProcess = arguments.removeAll("--child") > 0;
    DispatchLifetimeTest test;
    return QTest::qExec(&test, arguments);
}

#include "tst_dispatchlifetime.moc"
