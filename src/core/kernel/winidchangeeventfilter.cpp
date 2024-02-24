// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "winidchangeeventfilter_p.h"

#include <QtGui/qpa/qplatformwindow.h>

#include "abstractwindowcontext_p.h"

namespace QWK {

    WindowWinIdChangeEventFilter::WindowWinIdChangeEventFilter(QWindow *host,
                                                               AbstractWindowContext *context)
        : WinIdChangeEventFilter(host, context), win(host), isAboutToBeDestroyed(false) {
        host->installEventFilter(this);
    }

    WId WindowWinIdChangeEventFilter::winId() const {
        auto win = static_cast<QWindow *>(host);
        if (isAboutToBeDestroyed)
            return 0;
        if (auto platformWindow = win->handle())
            return platformWindow->winId();
        return 0;
    }

    bool WindowWinIdChangeEventFilter::eventFilter(QObject *obj, QEvent *event) {
        Q_UNUSED(obj)
        if (event->type() == QEvent::PlatformSurface) {
            auto e = static_cast<QPlatformSurfaceEvent *>(event);
            if (e->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
                isAboutToBeDestroyed = true;
                context->notifyWinIdChange();
                isAboutToBeDestroyed = false;
            } else {
                context->notifyWinIdChange();
            }
        }
        return false;
    }

}
