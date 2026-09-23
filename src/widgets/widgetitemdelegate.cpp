// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "widgetitemdelegate_p.h"

#include <QtGui/QMouseEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

#include <QWKCore/private/abstractwindowcontext_p.h>

extern Q_DECL_IMPORT QWidget *qt_button_down;

namespace QWK {

    class WidgetWinIdChangeEventFilter : public WinIdChangeEventFilter {
    public:
        explicit WidgetWinIdChangeEventFilter(QObject *host, AbstractWindowContext *ctx)
            : WinIdChangeEventFilter(host, ctx), widget(static_cast<QWidget *>(host)) {
            widget->installEventFilter(this);
        }

        WId winId() const override {
            return widget->effectiveWinId();
        }

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override {
            if (event->type() != QEvent::WinIdChange)
                return false;
            const QPointer<QObject> receiver(obj);
            context->notifyWinIdChange();
            return !receiver;
        }

        QWidget *widget;
    };

    WidgetItemDelegate::WidgetItemDelegate() = default;

    WidgetItemDelegate::~WidgetItemDelegate() = default;

    bool WidgetItemDelegate::isEnabled(const QObject *obj) const {
        return static_cast<const QWidget *>(obj)->isEnabled();
    }

    bool WidgetItemDelegate::isVisible(const QObject *obj) const {
        return static_cast<const QWidget *>(obj)->isVisible();
    }

    QRect WidgetItemDelegate::mapGeometryToScene(const QObject *obj) const {
        auto widget = static_cast<const QWidget *>(obj);
        const QPoint originPoint = widget->mapTo(widget->window(), QPoint(0, 0));
        const QSize size = widget->size();
        return {originPoint, size};
    }

    QWindow *WidgetItemDelegate::hostWindow(const QObject *host) const {
        return static_cast<const QWidget *>(host)->windowHandle();
    }

    bool WidgetItemDelegate::isWindowActive(const QObject *host) const {
        return static_cast<const QWidget *>(host)->isActiveWindow();
    }

    void WidgetItemDelegate::resetQtGrabbedControl(QObject *host) const {
        Q_UNUSED(host);
        if (!qt_button_down) {
            return;
        }
        static constexpr const auto invalidPos =
            QPoint{std::numeric_limits<int>::lowest(), std::numeric_limits<int>::lowest()};
        const auto event = new QMouseEvent(
            QEvent::MouseButtonRelease, invalidPos, invalidPos, invalidPos, Qt::LeftButton,
            QGuiApplication::mouseButtons() ^ Qt::LeftButton, QGuiApplication::keyboardModifiers());
        QCoreApplication::postEvent(qt_button_down, event);
        qt_button_down = nullptr;
    }

    Qt::WindowStates WidgetItemDelegate::getWindowState(const QObject *host) const {
        return static_cast<const QWidget *>(host)->windowState();
    }

    void WidgetItemDelegate::setWindowState(QObject *host, Qt::WindowStates state) const {
        static_cast<QWidget *>(host)->setWindowState(state);
    }

    void WidgetItemDelegate::setCursorShape(QObject *host, Qt::CursorShape shape) const {
        auto widget = static_cast<QWidget *>(host);
        if (!m_savedCursor) {
            m_savedCursor = widget->cursor();
            m_hadExplicitCursor = widget->testAttribute(Qt::WA_SetCursor);
        }
        widget->setCursor(QCursor(shape));
    }

    void WidgetItemDelegate::restoreCursorShape(QObject *host) const {
        if (!m_savedCursor)
            return;
        const auto cursor = *m_savedCursor;
        const bool explicitCursor = m_hadExplicitCursor;
        // Clear before QWidget sends CursorChange, which can reenter or delete the agent.
        m_savedCursor.reset();
        auto widget = static_cast<QWidget *>(host);
        if (explicitCursor)
            widget->setCursor(cursor);
        else {
            // unsetCursor() only clears this attribute on non-window widgets.
            widget->setAttribute(Qt::WA_SetCursor, false);
            widget->unsetCursor();
        }
    }

    Qt::WindowFlags WidgetItemDelegate::getWindowFlags(const QObject *host) const {
        return static_cast<const QWidget *>(host)->windowFlags();
    }

    QRect WidgetItemDelegate::getGeometry(const QObject *host) const {
        return static_cast<const QWidget *>(host)->geometry();
    }

    void WidgetItemDelegate::setWindowFlags(QObject *host, Qt::WindowFlags flags) const {
        static_cast<QWidget *>(host)->setWindowFlags(flags);
    }

    void WidgetItemDelegate::setWindowVisible(QObject *host, bool visible) const {
        static_cast<QWidget *>(host)->setVisible(visible);
    }

    void WidgetItemDelegate::setGeometry(QObject *host, const QRect &rect) {
        static_cast<QWidget *>(host)->setGeometry(rect);
    }

    void WidgetItemDelegate::bringWindowToTop(QObject *host) const {
        static_cast<QWidget *>(host)->raise();
    }

    WinIdChangeEventFilter *
        WidgetItemDelegate::createWinIdEventFilter(QObject *host,
                                                   AbstractWindowContext *context) const {
        return new WidgetWinIdChangeEventFilter(host, context);
    }

}
