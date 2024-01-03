// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#ifndef WINDOWAGENTBASEPRIVATE_H
#define WINDOWAGENTBASEPRIVATE_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the QWindowKit API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <QWKCore/windowagentbase.h>
#include <QWKCore/private/abstractwindowcontext_p.h>

namespace QWK {

    class QWK_CORE_EXPORT WindowAgentBasePrivate {
        Q_DECLARE_PUBLIC(WindowAgentBase)
    public:
        WindowAgentBasePrivate();
        virtual ~WindowAgentBasePrivate();

        void init();

        WindowAgentBase *q_ptr; // no need to initialize

        virtual AbstractWindowContext *createContext() const;

        void setup(QObject *host, WindowItemDelegate *delegate);

        std::unique_ptr<AbstractWindowContext> context;

    public:
        using WindowContextFactoryMethod = AbstractWindowContext *(*) ();

        static WindowContextFactoryMethod windowContextFactoryMethod;

    private:
        Q_DISABLE_COPY(WindowAgentBasePrivate)
    };

}

#endif // WINDOWAGENTBASEPRIVATE_H