// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "winidchangeeventfilter_p.h"

#include <QtGui/qpa/qplatformwindow.h>

#include "abstractwindowcontext_p.h"

namespace QWK {

    WindowWinIdChangeEventFilter::WindowWinIdChangeEventFilter(QWindow *host,
                                                               AbstractWindowContext *context)
        : WinIdChangeEventFilter(host, context), isAboutToBeDestroyed(false) {
        host->installEventFilter(this);
    }

    WId WindowWinIdChangeEventFilter::winId() const {
        if (isAboutToBeDestroyed)
            return 0;
        auto window = static_cast<QWindow *>(host);
        if (auto platformWindow = window->handle())
            return platformWindow->winId();
        return 0;
    }

    bool WindowWinIdChangeEventFilter::eventFilter(QObject *obj, QEvent *event) {
        if (event->type() != QEvent::PlatformSurface)
            return false;
        const QPointer<QObject> receiver(obj);
        const QPointer<WindowWinIdChangeEventFilter> self(this);
        auto e = static_cast<QPlatformSurfaceEvent *>(event);
        if (e->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
            const bool previous = isAboutToBeDestroyed;
            isAboutToBeDestroyed = true;
            context->notifyWinIdChange();
            if (self)
                self->isAboutToBeDestroyed = previous;
        } else {
            context->notifyWinIdChange();
        }
        // Qt must not deliver the event to a receiver deleted by a shared callback.
        return !receiver;
    }

}
