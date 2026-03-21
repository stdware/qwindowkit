// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef QTWINDOWCONTEXT_P_H
#define QTWINDOWCONTEXT_P_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the QWindowKit API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <QWKCore/private/abstractwindowcontext_p.h>

#ifdef Q_OS_WIN
#  define QWK_FINAL final
#else
#  define QWK_FINAL
#endif

namespace QWK {

    class QWK_CORE_EXPORT QtWindowContext QWK_FINAL : public AbstractWindowContext {
        Q_OBJECT
    public:
        QtWindowContext();
        ~QtWindowContext() override;

        QString key() const override;
        void virtual_hook(int id, void *data) override;

    protected:
        void winIdChanged(WId winId, WId oldWinId) override;

    protected:
        std::unique_ptr<SharedEventFilter> qtWindowEventFilter;
    };

}

#undef QWK_FINAL

#endif // QTWINDOWCONTEXT_P_H
