// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "quickwindowagent_p.h"

#include <QtQuick/QQuickPaintedItem>
#include <QtQuick/private/qquickitem_p.h>

#include <QWKCore/qwindowkit_windows.h>

namespace QWK {

#if QWINDOWKIT_CONFIG(ENABLE_WINDOWS_SYSTEM_BORDERS)
    // TODO: Find a way to draw native border
    // We haven't found a way to place hooks in the Quick program and call the GDI API to draw
    // the native border area so that we'll use the emulated drawn border for now.

    class BorderItem : public QQuickPaintedItem,
                       public NativeEventFilter,
                       public SharedEventFilter {
    public:
        explicit BorderItem(QQuickItem *parent, AbstractWindowContext *context);
        ~BorderItem() override;

        inline bool isNormalWindow() const;

        inline void updateGeometry();

    public:
        void paint(QPainter *painter) override;
        void itemChange(ItemChange change, const ItemChangeData &data) override;

    protected:
        bool nativeEventFilter(const QByteArray &eventType, void *message,
                               QT_NATIVE_EVENT_RESULT_TYPE *result) override;

        bool sharedEventFilter(QObject *obj, QEvent *event) override;

        AbstractWindowContext *context;

    private:
        void _q_windowActivityChanged();
    };

    BorderItem::BorderItem(QQuickItem *parent, AbstractWindowContext *context)
        : QQuickPaintedItem(parent), context(context) {
        setAntialiasing(true);   // We need anti-aliasing to give us better result.
        setFillColor({});        // Will improve the performance a little bit.
        setOpaquePainting(true); // Will also improve the performance, we don't draw
                                 // semi-transparent borders of course.

        auto parentPri = QQuickItemPrivate::get(parent);
        auto anchors = QQuickItemPrivate::get(this)->anchors();
        anchors->setTop(parentPri->top());
        anchors->setLeft(parentPri->left());
        anchors->setRight(parentPri->right());

        setZ(std::numeric_limits<qreal>::max()); // Make sure our fake border always above everything in the window.

        context->installNativeEventFilter(this);
        context->installSharedEventFilter(this);

        connect(window(), &QQuickWindow::activeChanged, this,
                &BorderItem::_q_windowActivityChanged);
        updateGeometry();
    }

    BorderItem::~BorderItem() = default;

    bool BorderItem::isNormalWindow() const {
        return !(context->window()->windowStates() &
                 (Qt::WindowMinimized | Qt::WindowMaximized | Qt::WindowFullScreen));
    }

    void BorderItem::updateGeometry() {
        setHeight(context->windowAttribute(QStringLiteral("border-thickness")).toInt());
        setVisible(isNormalWindow());
    }

    void BorderItem::paint(QPainter *painter) {
        QRect rect(QPoint(0, 0), size().toSize());
        QRegion region(rect);
        void *args[] = {
            painter,
            &rect,
            &region,
        };
        context->virtual_hook(AbstractWindowContext::DrawWindows10BorderHook, args);
    }

    void BorderItem::itemChange(ItemChange change, const ItemChangeData &data) {
        QQuickPaintedItem::itemChange(change, data);
        switch (change) {
            case ItemVisibleHasChanged:
            case ItemDevicePixelRatioHasChanged: {
                updateGeometry();
                break;
            }
            default:
                break;
        }
    }

    bool BorderItem::nativeEventFilter(const QByteArray &eventType, void *message,
                                       QT_NATIVE_EVENT_RESULT_TYPE *result) {
        Q_UNUSED(eventType)

        const auto msg = static_cast<const MSG *>(message);
        switch (msg->message) {
            case WM_THEMECHANGED:
            case WM_SYSCOLORCHANGE:
            case WM_DWMCOLORIZATIONCOLORCHANGED: {
                update();
                break;
            }

            case WM_SETTINGCHANGE: {
                if (isImmersiveColorSetChange(msg->wParam, msg->lParam)) {
                    update();
                }
                break;
            }

            default:
                break;
        }
        return false;
    }

    bool BorderItem::sharedEventFilter(QObject *obj, QEvent *event) {
        Q_UNUSED(obj)
        
        switch (event->type()) {
            case QEvent::WindowStateChange: {
                updateGeometry();
                break;
            }
            default:
                break;
        }
        return false;
    }

    void BorderItem::_q_windowActivityChanged() {
        update();
    }

    void QuickWindowAgentPrivate::setupWindows10BorderWorkaround() {
        // Install painting hook
        auto ctx = context.get();
        if (ctx->windowAttribute(QStringLiteral("win10-border-needed")).toBool()) {
            std::ignore = new BorderItem(hostWindow->contentItem(), ctx);
        }
    }
#endif

}
