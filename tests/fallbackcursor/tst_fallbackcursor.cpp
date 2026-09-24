// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <memory>
#include <functional>
#include <type_traits>
#include <QtGui/QBitmap>
#include <QtGui/QMouseEvent>
#include <QtTest/QTest>
#include <QWKCore/private/qtwindowcontext_p.h>
#include <QWKCore/private/windowagentbase_p.h>
#ifdef TEST_WIDGETS
#  include <QtWidgets/QApplication>
#  include <QWKWidgets/widgetwindowagent.h>
#  include <QWKWidgets/private/widgetitemdelegate_p.h>
#endif
#ifdef TEST_QUICK
#  include <QtGui/QGuiApplication>
#  include <QWKQuick/quickwindowagent.h>
#endif

namespace {
    class FactoryGuard {
    public:
        FactoryGuard() : previous(QWK::WindowAgentBasePrivate::windowContextFactoryMethod) {
            QWK::WindowAgentBasePrivate::windowContextFactoryMethod = []() -> QWK::AbstractWindowContext * {
                return new QWK::QtWindowContext;
            };
        }
        ~FactoryGuard() { QWK::WindowAgentBasePrivate::windowContextFactoryMethod = previous; }
    private:
        QWK::WindowAgentBasePrivate::WindowContextFactoryMethod previous;
    };

#ifdef TEST_WIDGETS
    class CursorCallback : public QObject {
    public:
        std::function<void()> callback;
        bool eventFilter(QObject *, QEvent *event) override {
            if (event->type() == QEvent::CursorChange && callback) {
                auto once = std::move(callback);
                once();
            }
            return false;
        }
    };

    class Widget : public QWidget {
    public:
        using QWidget::QWidget;
        using QWidget::destroy;
    };
    QWindow *handle(Widget &w) { return w.windowHandle(); }
#endif
#ifdef TEST_QUICK
    QWindow *handle(QQuickWindow &w) { return &w; }
#endif

    void mouse(QWindow *window, QPoint point, QEvent::Type type = QEvent::MouseMove) {
        QMouseEvent event(type, QPointF(point), QPointF(point), QPointF(window->mapToGlobal(point)),
                          type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                          Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(window, &event);
    }

    QCursor customCursor(int kind) {
        if (kind == 0)
            return QCursor(Qt::CrossCursor);
        if (kind == 1) {
            QPixmap pixmap(19, 23);
            pixmap.fill(Qt::transparent);
            QImage image = pixmap.toImage();
            for (int x = 2; x < 16; ++x)
                for (int y = 3; y < 18; ++y)
                    image.setPixelColor(x, y, QColor(20 + x * 10, 20 + y * 10, 80));
            return QCursor(QPixmap::fromImage(image), 3, 7);
        }
        QBitmap bits(16, 16), mask(16, 16);
        bits.fill(Qt::color0);
        mask.fill(Qt::color1);
        return QCursor(bits, mask, 5, 9);
    }

    void compareCursor(const QCursor &actual, const QCursor &expected) {
        QCOMPARE(actual.shape(), expected.shape());
        QCOMPARE(actual.hotSpot(), expected.hotSpot());
        if (expected.shape() == Qt::BitmapCursor) {
            QCOMPARE(actual.pixmap().toImage(), expected.pixmap().toImage());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            QCOMPARE(actual.bitmap().toImage(), expected.bitmap().toImage());
            QCOMPARE(actual.mask().toImage(), expected.mask().toImage());
#else
            QCOMPARE(actual.bitmap() ? actual.bitmap()->toImage() : QImage(),
                     expected.bitmap() ? expected.bitmap()->toImage() : QImage());
            QCOMPARE(actual.mask() ? actual.mask()->toImage() : QImage(),
                     expected.mask() ? expected.mask()->toImage() : QImage());
#endif
        }
    }

    template <typename Window, typename Agent> struct Fixture {
        FactoryGuard factory;
        std::unique_ptr<Window> window = std::make_unique<Window>();
        std::unique_ptr<Agent> agent = std::make_unique<Agent>();
        Fixture() {
            window->resize(320, 240);
            agent->setup(window.get());
            window->show();
            QCoreApplication::processEvents();
        }
        void edge() {
            mouse(handle(*window), {1, 100});
            QCOMPARE(window->cursor().shape(), Qt::SizeHorCursor);
            mouse(handle(*window), {100, 1});
            QCOMPARE(window->cursor().shape(), Qt::SizeVerCursor);
        }
    };

    template <typename Callback> void withFixture(bool quick, Callback callback) {
        Q_UNUSED(quick)
#ifdef TEST_WIDGETS
        if (!quick) {
            Fixture<Widget, QWK::WidgetWindowAgent> f;
            callback(f);
        }
#endif
#ifdef TEST_QUICK
        if (quick) {
            Fixture<QQuickWindow, QWK::QuickWindowAgent> f;
            callback(f);
        }
#endif
    }

    void backendRows() {
        QTest::addColumn<bool>("quick");
#ifdef TEST_WIDGETS
        QTest::newRow("widgets") << false;
#endif
#ifdef TEST_QUICK
        QTest::newRow("quick") << true;
#endif
    }
}

class FallbackCursorTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void restore_data() {
        QTest::addColumn<bool>("quick");
        QTest::addColumn<int>("kind");
        QTest::addColumn<QString>("boundary");
        QList<bool> backends;
#ifdef TEST_WIDGETS
        backends << false;
#endif
#ifdef TEST_QUICK
        backends << true;
#endif
        for (bool quick : backends)
            for (int kind = 0; kind < 3; ++kind)
                for (const auto boundary : {"client", "release", "leave", "hide", "state",
                                            "constraints", "recreate", "agent", "app-write"}) {
                    const auto name = QByteArray(quick ? "quick-" : "widgets-")
                        + QByteArray::number(kind) + "-" + boundary;
                    QTest::newRow(name.constData()) << quick << kind << QString(boundary);
                }
    }

    void restore() {
        QFETCH(bool, quick);
        QFETCH(int, kind);
        QFETCH(QString, boundary);
        withFixture(quick, [&](auto &f) {
            auto &w = *f.window;
            const auto original = customCursor(kind);
            w.setCursor(original);
            f.edge();
            if (boundary == "client")
                mouse(handle(w), {100, 100});
            else if (boundary == "release")
                mouse(handle(w), {100, 100}, QEvent::MouseButtonRelease);
            else if (boundary == "leave") {
                QEvent leave(QEvent::Leave);
                QCoreApplication::sendEvent(handle(w), &leave);
            } else if (boundary == "hide")
                w.hide();
            else if (boundary == "state")
                w.showMaximized();
            else if (boundary == "constraints") {
                w.setMinimumSize(w.size());
                w.setMaximumSize(w.size());
#ifdef TEST_WIDGETS
                // QWidget writes QWindow's constraints without emitting its signals.
                if constexpr (std::is_base_of_v<QWidget, std::decay_t<decltype(w)>>)
                    mouse(handle(w), {100, 1});
#endif
            } else if (boundary == "recreate") {
                w.destroy();
                compareCursor(w.cursor(), original);
                w.show();
            } else if (boundary == "agent")
                f.agent.reset();
            else if (boundary == "app-write") {
                w.setCursor(Qt::WaitCursor);
                f.edge();
                mouse(handle(w), {100, 100});
            }
            compareCursor(w.cursor(), original);
#ifdef TEST_WIDGETS
            if constexpr (std::is_base_of_v<QWidget, std::decay_t<decltype(w)>>)
                QVERIFY(w.testAttribute(Qt::WA_SetCursor));
#endif
        });
    }

    void repeatedOwnership_data() { backendRows(); }
    void repeatedOwnership() {
        QFETCH(bool, quick);
        withFixture(quick, [](auto &f) {
            for (int i = 0; i < 4; ++i) {
                const auto cursor = customCursor(i % 3);
                f.window->setCursor(cursor);
                f.edge();
                mouse(handle(*f.window), {100, 100});
                compareCursor(f.window->cursor(), cursor);
                f.edge();
                f.window->destroy();
                compareCursor(f.window->cursor(), cursor);
                f.window->show();
                QCoreApplication::processEvents();
            }
        });
    }

    void hostDestroyedFirst_data() { backendRows(); }
    void hostDestroyedFirst() {
        QFETCH(bool, quick);
        withFixture(quick, [](auto &f) {
            f.window->setCursor(customCursor(1));
            f.edge();
            f.window.reset();
            f.agent.reset();
        });
    }

    void hostOwnsAgent_data() { backendRows(); }
    void hostOwnsAgent() {
        QFETCH(bool, quick);
        withFixture(quick, [](auto &f) {
            f.agent->setParent(f.window.get());
            QPointer<QObject> agent(f.agent.release());
            f.edge();
            f.window.reset();
            QVERIFY(!agent);
        });
    }

    void flagsChange_data() { backendRows(); }
    void flagsChange() {
        QFETCH(bool, quick);
        withFixture(quick, [](auto &f) {
            f.window->setCursor(Qt::CrossCursor);
            f.edge();
            auto native = handle(*f.window);
            native->setFlags(native->flags() | Qt::MSWindowsFixedSizeDialogHint);
            mouse(native, {1, 100});
            QCOMPARE(f.window->cursor().shape(), Qt::CrossCursor);
        });
    }

    void defaultCursor_data() { backendRows(); }
    void defaultCursor() {
        QFETCH(bool, quick);
        withFixture(quick, [](auto &f) {
            f.window->unsetCursor();
            const auto original = f.window->cursor();
            f.edge();
            f.agent.reset();
            compareCursor(f.window->cursor(), original);
#ifdef TEST_WIDGETS
            if constexpr (std::is_base_of_v<QWidget, std::decay_t<decltype(*f.window)>>)
                QVERIFY(!f.window->testAttribute(Qt::WA_SetCursor));
#endif
        });
    }

#ifdef TEST_WIDGETS
    void deleteAgentInCursorNotification_data() {
        QTest::addColumn<bool>("restoring");
        QTest::newRow("takeover") << false;
        QTest::newRow("restore") << true;
    }
    void deleteAgentInCursorNotification() {
        QFETCH(bool, restoring);
        Fixture<Widget, QWK::WidgetWindowAgent> f;
        const auto original = customCursor(1);
        f.window->setCursor(original);
        if (restoring)
            f.edge();
        CursorCallback filter;
        filter.callback = [&] { f.agent.reset(); };
        f.window->installEventFilter(&filter);
        mouse(handle(*f.window), restoring ? QPoint(100, 100) : QPoint(1, 100));
        QVERIFY(!f.agent);
        compareCursor(f.window->cursor(), original);
    }

    void inheritedCursor_data() {
        QTest::addColumn<bool>("changeParent");
        QTest::newRow("original-parent") << false;
        QTest::newRow("updated-parent") << true;
    }
    void inheritedCursor() {
        QFETCH(bool, changeParent);
        QWidget parent;
        parent.setCursor(Qt::CrossCursor);
        // Qt top-level widgets never inherit a parent's cursor. Exercise actual
        // inheritance with a child widget and the unchanged production delegate.
        QWidget w(&parent);
        QWK::WidgetItemDelegate delegate;
        QVERIFY(!w.testAttribute(Qt::WA_SetCursor));
        delegate.setCursorShape(&w, Qt::SizeHorCursor);
        QCOMPARE(w.cursor().shape(), Qt::SizeHorCursor);
        delegate.setCursorShape(&w, Qt::SizeVerCursor);
        if (changeParent)
            parent.setCursor(customCursor(1));
        delegate.restoreCursorShape(&w);
        QVERIFY(!w.testAttribute(Qt::WA_SetCursor));
        compareCursor(w.cursor(), parent.cursor());
        parent.setCursor(Qt::WaitCursor);
        QCOMPARE(w.cursor().shape(), Qt::WaitCursor);
    }
#endif
};

int main(int argc, char **argv) {
#ifdef TEST_WIDGETS
    QApplication app(argc, argv);
#else
    QGuiApplication app(argc, argv);
#endif
    FallbackCursorTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_fallbackcursor.moc"
