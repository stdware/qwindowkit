// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "quickwindowagent_p.h"
#include "quicksystembuttonarea_p.h"

namespace QWK {

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
        auto handler = new QuickSystemButtonArea(item);
        d->systemButtonAreaItemHandler.reset(handler);
        const QPointer<AbstractWindowContext> context(ctx);
        connect(handler, &QuickSystemButtonArea::changed, ctx,
                [context, area = QPointer<QuickSystemButtonArea>(handler)] {
            if (!context || !area)
                return;
            if (!area->item()) {
                context->setSystemButtonAreaCallback({});
                return;
            }
            context->virtual_hook(AbstractWindowContext::SystemButtonAreaChangedHook, nullptr);
        });
        ctx->setSystemButtonAreaCallback(
            [context, area = QPointer<QuickSystemButtonArea>(handler)](const QSize &) {
                return context && area ? area->sceneRect(context->window()) : QRect();
            });
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
