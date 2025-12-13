// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2025-2027 Wing-summer (wingsummer)
// SPDX-License-Identifier: Apache-2.0


#ifndef LINUXWAYLANDCONTEXT_P_H
#define LINUXWAYLANDCONTEXT_P_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the QWindowKit API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//


#include "qtwindowcontext_p.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
namespace QWK {

    class LinuxWaylandContext : public QtWindowContext {
        Q_OBJECT
    public:
        LinuxWaylandContext();
        ~LinuxWaylandContext() override;

        QString key() const override;
        void virtual_hook(int id, void *data) override;
    };

}
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#endif // LINUXWAYLANDCONTEXT_P_H
