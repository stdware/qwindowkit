// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "quickwindowagent_p.h"

namespace QWK {

    class SystemButtonAreaItemHandler : public QObject {
    public:
        SystemButtonAreaItemHandler(QQuickItem *item, AbstractWindowContext *ctx,
                                    QObject *parent = nullptr);
        ~SystemButtonAreaItemHandler() override = default;

        void updateSystemButtonArea();

    protected:
        QQuickItem *item;
        AbstractWindowContext *ctx;
    };

    SystemButtonAreaItemHandler::SystemButtonAreaItemHandler(QQuickItem *item,
                                                             AbstractWindowContext *ctx,
                                                             QObject *parent)
        : QObject(parent), item(item), ctx(ctx) {
        connect(item, &QQuickItem::xChanged, this,
                &SystemButtonAreaItemHandler::updateSystemButtonArea);
        connect(item, &QQuickItem::yChanged, this,
                &SystemButtonAreaItemHandler::updateSystemButtonArea);
        connect(item, &QQuickItem::widthChanged, this,
                &SystemButtonAreaItemHandler::updateSystemButtonArea);
        connect(item, &QQuickItem::heightChanged, this,
                &SystemButtonAreaItemHandler::updateSystemButtonArea);

        ctx->setSystemButtonAreaCallback([item](const QSize &) {
            return QRectF(item->mapToScene(QPointF(0, 0)), item->size()).toRect(); //
        });
    }

    void SystemButtonAreaItemHandler::updateSystemButtonArea() {
        ctx->virtual_hook(AbstractWindowContext::SystemButtonAreaChangedHook, nullptr);
    }

    QQuickItem *QuickWindowAgent::systemButtonArea() const {
        Q_D(const QuickWindowAgent);
        return d->systemButtonAreaItem;
    }

    void QuickWindowAgent::setSystemButtonArea(QQuickItem *item) {
        Q_D(QuickWindowAgent);
        if (d->systemButtonAreaItem == item)
            return;

        auto ctx = d->context.get();
        d->systemButtonAreaItem = item;
        if (!item) {
            d->systemButtonAreaItemHandler.reset();
            ctx->setSystemButtonAreaCallback({});
            return;
        }
        d->systemButtonAreaItemHandler = std::make_unique<SystemButtonAreaItemHandler>(item, ctx);
    }

    ScreenRectCallback QuickWindowAgent::systemButtonAreaCallback() const {
        Q_D(const QuickWindowAgent);
        return d->systemButtonAreaItem ? nullptr : d->context->systemButtonAreaCallback();
    }

    void QuickWindowAgent::setSystemButtonAreaCallback(const ScreenRectCallback &callback) {
        Q_D(QuickWindowAgent);
        setSystemButtonArea(nullptr);
        d->context->setSystemButtonAreaCallback(callback);
    }

}
