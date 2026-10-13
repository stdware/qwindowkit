// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <atomic>

#include <QtCore/QThread>
#include <QtCore/qt_windows.h>
#include <QtGui/QGuiApplication>
#include <QtQuick/QQuickPaintedItem>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRendererInterface>
#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

#include <QWKCore/private/windowagentbase_p.h>
#include <QWKQuick/quickwindowagent.h>

using namespace QWK;

namespace {
    QPointer<QQuickItem> observedBorder;
    bool contextDestroyedWithBorder = false;
    std::atomic_bool queriedOffGuiThread{false};
    QColor testColor(Qt::red);

    // Force the production border on any Windows version, without claiming to reproduce DWM.
    class BorderTestContext : public AbstractWindowContext {
    public:
        ~BorderTestContext() override {
            contextDestroyedWithBorder = !observedBorder.isNull();
        }

        QVariant windowAttribute(const QString &key) const override {
            if (key == QStringLiteral("win10-border-needed"))
                return true;
            return AbstractWindowContext::windowAttribute(key);
        }

        void virtual_hook(int id, void *data) override {
            if (id == Windows10BorderColorHook) {
                if (QThread::currentThread() != qGuiApp->thread())
                    queriedOffGuiThread = true;
                *static_cast<QColor *>(data) = testColor;
                return;
            }
            AbstractWindowContext::virtual_hook(id, data);
        }

    protected:
        void winIdChanged(WId, WId) override {}
        bool windowAttributeChanged(const QString &, const QVariant &, const QVariant &) override {
            return true;
        }
    };

    BorderTestContext *testContext = nullptr;

    AbstractWindowContext *createTestContext() {
        testContext = new BorderTestContext;
        return testContext;
    }

    void showTestWindow(QQuickWindow &window) {
        window.show();
        // CTest can launch with STARTUPINFO requesting that the first window stay hidden.
        // A second explicit show makes exposure deterministic without requiring activation.
        ::ShowWindow(reinterpret_cast<HWND>(window.winId()), SW_SHOWNOACTIVATE);
    }

    bool hasTopLine(QQuickWindow &window, const QColor &color) {
        const QImage image = window.grabWindow();
        if (image.isNull() || image.height() < 3 || image.width() < 10)
            return false;
        // Check the whole interior span: exactly one physical row, no tolerance that could
        // conceal a missing line or an extra row. Avoid the native side edges.
        for (int x = 3; x < image.width() - 3; ++x) {
            if (image.pixelColor(x, 0) != color || image.pixelColor(x, 1) != QColor(Qt::white) ||
                image.pixelColor(x, 2) != QColor(Qt::white))
                return false;
        }
        return true;
    }
}

class QuickBorderTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void init() {
        previousFactory = WindowAgentBasePrivate::windowContextFactoryMethod;
        WindowAgentBasePrivate::windowContextFactoryMethod = createTestContext;
        contextDestroyedWithBorder = false;
        queriedOffGuiThread = false;
        observedBorder.clear();
        testColor = Qt::red;
    }

    void cleanup() {
        WindowAgentBasePrivate::windowContextFactoryMethod = previousFactory;
        QVERIFY(!contextDestroyedWithBorder);
        QVERIFY(!queriedOffGuiThread.load());
    }

    void agentBeforeWindow() {
        QQuickWindow window;
        window.resize(240, 120);
        window.setColor(Qt::white);
        auto agent = std::make_unique<QuickWindowAgent>();
        QVERIFY(agent->setup(&window));
        QCOMPARE(window.contentItem()->childItems().size(), 1);
        observedBorder = window.contentItem()->childItems().first();
        showTestWindow(window);
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTRY_VERIFY(hasTopLine(window, Qt::red));

        agent.reset();
        QVERIFY(observedBorder.isNull());
        QVERIFY(window.contentItem()->childItems().isEmpty());
        window.resize(260, 140);
        window.requestActivate();
        window.update();
        QTRY_VERIFY(hasTopLine(window, Qt::white));
    }

    void windowBeforeAgent() {
        auto agent = std::make_unique<QuickWindowAgent>();
        auto window = std::make_unique<QQuickWindow>();
        window->resize(240, 120);
        QVERIFY(agent->setup(window.get()));
        QCOMPARE(window->contentItem()->childItems().size(), 1);
        observedBorder = window->contentItem()->childItems().first();
        showTestWindow(*window);
        QVERIFY(QTest::qWaitForWindowExposed(window.get()));
        QVERIFY(!window->grabWindow().isNull());
        window.reset();
        QVERIFY(observedBorder.isNull());
        agent.reset(); // Must not delete the already-destroyed decoration again.
    }

    void geometryColorAndSceneAttachment() {
        QQuickWindow window;
        window.resize(240, 120);
        window.setColor(Qt::white);
        QuickWindowAgent agent;
        QVERIFY(agent.setup(&window));
        QCOMPARE(window.contentItem()->childItems().size(), 1);
        observedBorder = window.contentItem()->childItems().first();
        auto border = observedBorder.data();
        QVERIFY(!qobject_cast<QQuickPaintedItem *>(border));
        QVERIFY(border->childItems().isEmpty());
        QCOMPARE(border->acceptedMouseButtons(), Qt::NoButton);
        QVERIFY(!border->acceptTouchEvents());
        QVERIFY(!border->acceptHoverEvents());
        QVERIFY(!border->activeFocusOnTab());
        QVERIFY(!border->isEnabled());

        showTestWindow(window);
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QCOMPARE(border->y(), qreal(0));
        QVERIFY(qAbs(border->height() * window.effectiveDevicePixelRatio() - 1) < 0.000001);
        qInfo() << "Actual DPR:" << window.effectiveDevicePixelRatio();
        const qreal requestedScale = qEnvironmentVariable("QT_SCALE_FACTOR").toDouble();
        if (requestedScale > 0)
            QCOMPARE(window.effectiveDevicePixelRatio(), requestedScale);
        QTRY_VERIFY(hasTopLine(window, Qt::red));
        window.resize(320, 160);
        QTRY_COMPARE(border->width(), window.contentItem()->width());
        QTRY_VERIFY(hasTopLine(window, Qt::red));

        testColor = Qt::blue;
        MSG message{};
        message.message = WM_SYSCOLORCHANGE;
        QT_NATIVE_EVENT_RESULT_TYPE result = 0;
        testContext->nativeDispatch({}, &message, &result);
        QTRY_VERIFY(hasTopLine(window, Qt::blue));

        window.setWindowStates(Qt::WindowMaximized);
        QTRY_VERIFY(!border->isVisible());
        window.setWindowStates(Qt::WindowNoState);
        QTRY_VERIFY(border->isVisible());
        QTRY_VERIFY(hasTopLine(window, Qt::blue));

        // Detachment releases the item's node even on backends that retain their entire
        // scene graph when a window is merely hidden or releaseResources() is requested.
        border->setParentItem(nullptr);
        QTRY_VERIFY(hasTopLine(window, Qt::white));
        border->setParentItem(window.contentItem());
        QTRY_VERIFY(hasTopLine(window, Qt::blue));
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void nativeSurfaceLifecycle() {
        auto window = std::make_unique<QQuickWindow>();
        window->resize(240, 120);
        auto agent = std::make_unique<QuickWindowAgent>();
        QVERIFY(agent->setup(window.get()));
        observedBorder = window->contentItem()->childItems().first();
        showTestWindow(*window);
        QVERIFY(QTest::qWaitForWindowExposed(window.get()));
        QCOMPARE(window->rendererInterface()->graphicsApi(), QSGRendererInterface::Direct3D11);
        QTRY_COMPARE(observedBorder->childItems().size(), 1);
        QVERIFY(qobject_cast<QQuickPaintedItem *>(observedBorder->childItems().first()));
        QCOMPARE(observedBorder->height(), qreal(0.5));
        QCOMPARE(observedBorder->y(), qreal(-0.49));

        QSignalSpy frames(window.get(), &QQuickWindow::afterSynchronizing);
        window->resize(260, 140);
        QTRY_VERIFY(!frames.isEmpty());
        agent.reset();
        QVERIFY(observedBorder.isNull());
        frames.clear();
        window->update();
        QTRY_VERIFY(!frames.isEmpty());

        agent = std::make_unique<QuickWindowAgent>();
        QVERIFY(agent->setup(window.get()));
        observedBorder = window->contentItem()->childItems().first();
        window->update();
        QTRY_COMPARE(observedBorder->childItems().size(), 1);
        window.reset();
        QVERIFY(observedBorder.isNull());
        agent.reset();
    }
#endif

private:
    WindowAgentBasePrivate::WindowContextFactoryMethod previousFactory = nullptr;
};

int main(int argc, char **argv) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
    QGuiApplication application(argc, argv);
    QuickBorderTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_quickborder.moc"
