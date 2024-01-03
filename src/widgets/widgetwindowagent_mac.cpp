// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "widgetwindowagent_p.h"

#include <QtGui/QtEvents>

namespace QWK {

    static inline QRect getWidgetSceneRect(QWidget *widget) {
        return {widget->mapTo(widget->window(), QPoint()), widget->size()};
    }

    class SystemButtonAreaWidgetEventFilter : public QObject {
    public:
        SystemButtonAreaWidgetEventFilter(QWidget *widget, AbstractWindowContext *ctx,
                                          QObject *parent = nullptr)
            : QObject(parent), widget(widget), ctx(ctx) {
            widget->installEventFilter(this);
            ctx->setSystemButtonAreaCallback([widget](const QSize &) {
                return getWidgetSceneRect(widget); //
            });
        }
        ~SystemButtonAreaWidgetEventFilter() override = default;

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override {
            Q_UNUSED(obj)
            switch (event->type()) {
                case QEvent::Move:
                case QEvent::Resize: {
                    ctx->virtual_hook(AbstractWindowContext::SystemButtonAreaChangedHook, nullptr);
                    break;
                }

                default:
                    break;
            }
            return false;
        }

    protected:
        QWidget *widget;
        AbstractWindowContext *ctx;
    };

    /*!
        Returns the widget that acts as the system button area.
    */
    QWidget *WidgetWindowAgent::systemButtonArea() const {
        Q_D(const WidgetWindowAgent);
        return d->systemButtonAreaWidget;
    }

    /*!
        Sets the widget that acts as the system button area. The system button will be centered in
        its area, it is recommended to place the widget in a layout and set a fixed size policy.

        The system button will be visible in the system title bar area.
    */
    void WidgetWindowAgent::setSystemButtonArea(QWidget *widget) {
        Q_D(WidgetWindowAgent);
        if (d->systemButtonAreaWidget == widget)
            return;

        auto ctx = d->context.get();
        d->systemButtonAreaWidget = widget;
        if (!widget) {
            d->context->setSystemButtonAreaCallback({});
            d->systemButtonAreaWidgetEventFilter.reset();
            return;
        }
        d->systemButtonAreaWidgetEventFilter =
            std::make_unique<SystemButtonAreaWidgetEventFilter>(widget, ctx);
    }

    /*!
        Returns the the system button area callback.
    */
    ScreenRectCallback WidgetWindowAgent::systemButtonAreaCallback() const {
        Q_D(const WidgetWindowAgent);
        return d->systemButtonAreaWidget ? nullptr : d->context->systemButtonAreaCallback();
    }

    /*!
        Sets the the system button area callback, the \c size of the callback is the native title
        bar size.
        
        The system button position will be updated when the window resizes.
    */
    void WidgetWindowAgent::setSystemButtonAreaCallback(const ScreenRectCallback &callback) {
        Q_D(WidgetWindowAgent);
        setSystemButtonArea(nullptr);
        d->context->setSystemButtonAreaCallback(callback);
    }

}