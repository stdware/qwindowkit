// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2025-2027 Wing-summer (wingsummer)
// SPDX-License-Identifier: Apache-2.0


#ifndef WAYLANDCONTEXT_P_H
#define WAYLANDCONTEXT_P_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the QWindowKit API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//


#include "qtwindowcontext_p.h"

// QWaylandApplication was introduced in Qt 6.5. Since Qt 6.7 its declaration
// also depends on the wayland feature, which older Qt versions do not define.
#if defined(Q_OS_LINUX) && QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#  if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
#    define QWK_HAS_WAYLAND_CONTEXT
#  elif QT_CONFIG(wayland)
#    define QWK_HAS_WAYLAND_CONTEXT
#  endif
#endif

#ifdef QWK_HAS_WAYLAND_CONTEXT
namespace QWK {

    class WaylandContext : public QtWindowContext {
        Q_OBJECT
    public:
        WaylandContext();
        ~WaylandContext() override;

        QString key() const override;
        void virtual_hook(int id, void *data) override;
    };

}
#endif // QWK_HAS_WAYLAND_CONTEXT
#endif // WAYLANDCONTEXT_P_H
