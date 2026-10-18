// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "widgetwindowagent_p.h"

#include <QtGui/QtEvents>
#include <QtCore/QSet>
#include <QtCore/QVector>
#include <QtWidgets/QLayout>

namespace QWK {

    // Automatic QWidget safe-area margins can leave an empty strip above the title bar.
    // Some supported Qt versions enable these margins on child widgets too.
    // Opt out the host, title-bar subtree, and ancestor containers between them.
    class CocoaWidgetSafeAreaHandler : public QObject {
    public:
        explicit CocoaWidgetSafeAreaHandler(QWidget *host)
            : host(host) {
            host->setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false);
            refreshLayout(host);
        }

        ~CocoaWidgetSafeAreaHandler() override {
            if (titleBar)
                titleBar->removeEventFilter(this);
            clearBoundaryWidgets();
            restoreTitleBar();
        }

        void setTitleBar(QWidget *widget) {
            if (titleBar)
                titleBar->removeEventFilter(this);
            clearBoundaryWidgets();
            restoreTitleBar();
            titleBar = widget;
            if (titleBar)
                titleBar->installEventFilter(this);
            updateTitleBar();
        }

    protected:
        bool eventFilter(QObject *, QEvent *event) override {
            switch (event->type()) {
                case QEvent::ChildAdded:
                case QEvent::ChildRemoved:
                case QEvent::ChildPolished:
                case QEvent::ParentChange:
                case QEvent::Show:
                case QEvent::Hide:
                    if (!updatePending) {
                        updatePending = true;
                        QMetaObject::invokeMethod(this, [this] {
                            updatePending = false;
                            updateTitleBar();
                        }, Qt::QueuedConnection);
                    }
                    break;
                default:
                    break;
            }
            return false;
        }

    private:
        struct WidgetState {
            QPointer<QWidget> widget;
            bool respectsSafeArea;
        };

        static void invalidateLayout(QWidget *widget) {
            if (auto layout = widget->layout())
                layout->invalidate();
            widget->updateGeometry();
            widget->update();
        }

        static void refreshLayout(QWidget *widget) {
            invalidateLayout(widget);
            if (auto layout = widget->layout())
                layout->activate();
        }

        static void collectWidgets(QWidget *widget, QSet<QWidget *> &widgets,
                                   QSet<QWidget *> &childWindows) {
            widgets.insert(widget);
            for (auto child : widget->children()) {
                auto childWidget = qobject_cast<QWidget *>(child);
                if (!childWidget)
                    continue;
                if (childWidget->isWindow())
                    childWindows.insert(childWidget);
                else
                    collectWidgets(childWidget, widgets, childWindows);
            }
        }

        void clearBoundaryWidgets() {
            for (const auto &widget : boundaryWidgets) {
                if (widget)
                    widget->removeEventFilter(this);
            }
            boundaryWidgets.clear();
        }

        void watchBoundaryWidgets(QSet<QWidget *> widgets, const QSet<QWidget *> &managedWidgets) {
            auto parent = titleBar ? titleBar->parentWidget() : nullptr;
            while (parent) {
                widgets.insert(parent);
                if (parent->isWindow())
                    break;
                parent = parent->parentWidget();
            }
            widgets.subtract(managedWidgets);
            for (auto widget : widgets) {
                boundaryWidgets.append(widget);
                widget->installEventFilter(this);
            }
        }

        void restoreTitleBar() {
            for (const auto &state : widgets) {
                if (auto widget = state.widget.data()) {
                    widget->removeEventFilter(this);
                    if (state.respectsSafeArea)
                        widget->setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea);
                    invalidateLayout(widget);
                }
            }
            widgets.clear();
        }

        void updateTitleBar() {
            clearBoundaryWidgets();
            QSet<QWidget *> current;
            QSet<QWidget *> childWindows;
            if (host && titleBar && titleBar->window() == host) {
                collectWidgets(titleBar, current, childWindows);
                for (auto parent = titleBar->parentWidget(); parent && parent != host;
                     parent = parent->parentWidget())
                    current.insert(parent);
            }

            QSet<QWidget *> tracked;
            for (auto it = widgets.begin(); it != widgets.end();) {
                auto widget = it->widget.data();
                if (widget && current.contains(widget)) {
                    tracked.insert(widget);
                    ++it;
                    continue;
                }
                if (widget) {
                    if (widget != titleBar)
                        widget->removeEventFilter(this);
                    if (it->respectsSafeArea)
                        widget->setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea);
                    invalidateLayout(widget);
                }
                it = widgets.erase(it);
            }

            QVector<QPointer<QWidget>> changed;
            for (auto widget : current) {
                if (!tracked.contains(widget)) {
                    widgets.append(
                        {widget, widget->testAttribute(Qt::WA_ContentsMarginsRespectsSafeArea)});
                    widget->installEventFilter(this);
                }
                if (widget->testAttribute(Qt::WA_ContentsMarginsRespectsSafeArea)) {
                    widget->setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false);
                    changed.append(widget);
                }
            }
            watchBoundaryWidgets(childWindows, current);

            const auto hostWidget = host;
            for (const auto &widget : changed) {
                if (widget)
                    refreshLayout(widget);
            }
            if (!changed.isEmpty() && hostWidget)
                refreshLayout(hostWidget);
        }

        QPointer<QWidget> host;
        QPointer<QWidget> titleBar;
        QVector<QPointer<QWidget>> boundaryWidgets;
        QVector<WidgetState> widgets;
        bool updatePending = false;
    };

    void WidgetWindowAgentPrivate::setupCocoaWindow() {
        // Set QWK_DISABLE_SAFE_AREA_WORKAROUND=1 to keep Qt's default safe-area margins.
        if (qEnvironmentVariableIntValue("QWK_DISABLE_SAFE_AREA_WORKAROUND"))
            return;
        if (context->key() == QStringLiteral("cocoa"))
            safeAreaHandler = std::make_unique<CocoaWidgetSafeAreaHandler>(hostWidget);
    }

    void WidgetWindowAgentPrivate::updateCocoaTitleBar() {
        if (auto handler = static_cast<CocoaWidgetSafeAreaHandler *>(safeAreaHandler.get()))
            handler->setTitleBar(static_cast<QWidget *>(context->titleBar()));
    }

    static inline QRect getWidgetSceneRect(QWidget *widget) {
        return {widget->mapTo(widget->window(), QPoint()), widget->size()};
    }

    class SystemButtonAreaWidgetEventFilter : public QObject {
    public:
        SystemButtonAreaWidgetEventFilter(QWidget *widget, AbstractWindowContext *ctx,
                                          QObject *parent = nullptr)
            : QObject(parent), ctx(ctx) {
            widget->installEventFilter(this);
            connect(widget, &QObject::destroyed, this, [ctx] {
                ctx->setSystemButtonAreaCallback({});
            });
            ctx->setSystemButtonAreaCallback([area = QPointer<QWidget>(widget)](const QSize &) {
                return area ? getWidgetSceneRect(area) : QRect();
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
        // A destroyed area's QPointer is null, but its handler still needs to be disconnected.
        if (d->systemButtonAreaWidget == widget &&
            (widget || !d->systemButtonAreaWidgetEventFilter))
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
