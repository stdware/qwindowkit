// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "quickwindowagent_p.h"

#include <QtQuick/QQuickPaintedItem>
#include <QtQuick/private/qquickitem_p.h>

#include <QWKCore/qwindowkit_windows.h>
#include <QWKCore/private/windows10borderhandler_p.h>

namespace QWK {

#if QWINDOWKIT_CONFIG(ENABLE_WINDOWS_SYSTEM_BORDERS)

    class DeferredDeleteHook;

    class BorderItem : public QQuickPaintedItem, public Windows10BorderHandler {
    public:
        explicit BorderItem(QQuickItem *parent, AbstractWindowContext *context);
        ~BorderItem() override;

        void updateGeometry() override;

    public:
        void paint(QPainter *painter) override;
        void itemChange(ItemChange change, const ItemChangeData &data) override;

    protected:
        bool sharedEventFilter(QObject *obj, QEvent *event) override;

        std::unique_ptr<DeferredDeleteHook> paintHook;
    };

    class DeferredDeleteHook : public QObject {
    public:
        DeferredDeleteHook(BorderItem *item) : item(item) {
        }

        // Override deferred delete handler
        bool event(QEvent *event) override {
            if (event->type() == QEvent::DeferredDelete) {
                item->drawBorder();
                return true;
            }
            return QObject::event(event);
        }

        BorderItem *item;
    };

    BorderItem::BorderItem(QQuickItem *parent, AbstractWindowContext *context)
        : QQuickPaintedItem(parent), Windows10BorderHandler(context),
          paintHook(std::make_unique<DeferredDeleteHook>(this)) {
        setAntialiasing(true);   // We need anti-aliasing to give us better result.
        setFillColor({});        // Will improve the performance a little bit.
        setOpaquePainting(true); // Will also improve the performance, we don't draw
                                 // semi-transparent borders of course.

        auto parentPri = QQuickItemPrivate::get(parent);
        auto anchors = QQuickItemPrivate::get(this)->anchors();
        anchors->setTop(parentPri->top());
        anchors->setLeft(parentPri->left());
        anchors->setRight(parentPri->right());

        setZ(std::numeric_limits<qreal>::max()); // Make sure our fake border always above
                                                 // everything in the window.

        // First update
        if (context->windowId()) {
            setupNecessaryAttributes();
        }
        BorderItem::updateGeometry();
    }

    BorderItem::~BorderItem() = default;

    void BorderItem::updateGeometry() {
        setHeight(borderThickness());
        setVisible(isNormalWindow());
    }

    void BorderItem::paint(QPainter *painter) {
        Q_UNUSED(painter)

        // https://github.com/qt/qtdeclarative/blob/cc04afbb382fd4b1f65173d71f44d3372c47b0e1/src/quick/scenegraph/qsgthreadedrenderloop.cpp#L551
        // https://github.com/qt/qtdeclarative/blob/cc04afbb382fd4b1f65173d71f44d3372c47b0e1/src/quick/scenegraph/qsgthreadedrenderloop.cpp#L561
        // QCoreApplication must process all DeferredDelay events right away after rendering, we can
        // hook it by overriding a specifiy QObject's event handler and draw the native border in
        // it.
        paintHook->deleteLater();
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
        return Windows10BorderHandler::sharedEventFilter(obj, event);
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
