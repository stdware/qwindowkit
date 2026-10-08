// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#ifndef QUICKWINDOWAGENTPRIVATE_H
#define QUICKWINDOWAGENTPRIVATE_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the QWindowKit API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <QPointer>

#include <QWKCore/qwkconfig.h>
#include <QWKCore/private/windowagentbase_p.h>
#include <QWKQuick/quickwindowagent.h>

namespace QWK {

    class QuickWindowAgentPrivate : public WindowAgentBasePrivate {
        Q_DECLARE_PUBLIC(QuickWindowAgent)
    public:
        QuickWindowAgentPrivate();
        ~QuickWindowAgentPrivate() override;

        void init();

        // Host
        QQuickWindow *hostWindow{};

#ifdef Q_OS_MAC
        QQuickItem *systemButtonAreaItem{};
        std::unique_ptr<QObject> systemButtonAreaItemHandler;
#endif

#if defined(Q_OS_WINDOWS) && QWINDOWKIT_CONFIG(ENABLE_WINDOWS_SYSTEM_BORDERS)
        void setupWindows10BorderWorkaround();

        // The Win10 border workaround item is reparented into the host
        // window's content item and holds a raw context pointer; it must be
        // tracked and destroyed together with the agent (strictly before the
        // context unique_ptr), or the window teardown dereferences freed
        // memory through BorderItem::itemChange().
        QPointer<QQuickItem> win10BorderItem{};
#endif
    };

}

#endif // QUICKWINDOWAGENTPRIVATE_H
