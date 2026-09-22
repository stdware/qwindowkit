// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
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
        AbstractWindowContext *ctx;
    };

    SystemButtonAreaItemHandler::SystemButtonAreaItemHandler(QQuickItem *item,
                                                             AbstractWindowContext *ctx,
                                                             QObject *parent)
        : QObject(parent), ctx(ctx) {
        connect(item, &QQuickItem::xChanged, this,
                &SystemButtonAreaItemHandler::updateSystemButtonArea);
        connect(item, &QQuickItem::yChanged, this,
                &SystemButtonAreaItemHandler::updateSystemButtonArea);
        connect(item, &QQuickItem::widthChanged, this,
                &SystemButtonAreaItemHandler::updateSystemButtonArea);
        connect(item, &QQuickItem::heightChanged, this,
                &SystemButtonAreaItemHandler::updateSystemButtonArea);

        // Tie cleanup to this registration so replacing it disconnects the old area.
        connect(item, &QObject::destroyed, this, [ctx] {
            ctx->setSystemButtonAreaCallback({});
        });
        ctx->setSystemButtonAreaCallback([item = QPointer<QQuickItem>(item)](const QSize &) {
            return item ? QRectF(item->mapToScene(QPointF(0, 0)), item->size()).toRect() : QRect();
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
        if (d->systemButtonAreaItem == item && (item || !d->systemButtonAreaItemHandler))
            return;

        auto ctx = d->context.get();
        // QPointer is already null during destroyed(); still retire the old registration
        // before installing another area or a custom callback.
        d->systemButtonAreaItemHandler.reset();
        d->systemButtonAreaItem = item;
        if (!item) {
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
