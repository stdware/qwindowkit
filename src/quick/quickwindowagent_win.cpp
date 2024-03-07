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

#  if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        volatile bool needPaint = false;

    private:
        void _q_afterSynchronizing();
#  endif
    };

    BorderItem::BorderItem(QQuickItem *parent, AbstractWindowContext *context)
        : QQuickPaintedItem(parent), Windows10BorderHandler(context) {
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

#  if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        connect(window(), &QQuickWindow::afterSynchronizing, this,
                &BorderItem::_q_afterSynchronizing, Qt::DirectConnection);
#  endif

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

#  if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        if (auto api = window()->rendererInterface()->graphicsApi();
            !(api == QSGRendererInterface::Direct3D11

#    if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
              || api == QSGRendererInterface::Direct3D12
#    endif
              )) {
#  endif
            QRect rect(QPoint(0, 0), size().toSize());
            QRegion region(rect);
            void *args[] = {
                painter,
                &rect,
                &region,
            };
            ctx->virtual_hook(AbstractWindowContext::DrawWindows10BorderHook, args);
#  if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        } else {
            needPaint = true;
        }
#  endif
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
            }
            default:
                break;
        }
        return Windows10BorderHandler::sharedEventFilter(obj, event);
    }

#  if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void BorderItem::_q_afterSynchronizing() {
        if (needPaint) {
            needPaint = false;
            drawBorder();
        }
    }
#  endif

    void QuickWindowAgentPrivate::setupWindows10BorderWorkaround() {
        // Install painting hook
        auto ctx = context.get();
        if (ctx->windowAttribute(QStringLiteral("win10-border-needed")).toBool()) {
            std::ignore = new BorderItem(hostWindow->contentItem(), ctx);
        }
    }
#endif

}
