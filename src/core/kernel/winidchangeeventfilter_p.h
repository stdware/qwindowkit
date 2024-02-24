// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#ifndef WINIDCHANGEEVENTFILTER_P_H
#define WINIDCHANGEEVENTFILTER_P_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the QWindowKit API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <QtGui/QWindow>

#include <QWKCore/qwkglobal.h>

namespace QWK {

    class AbstractWindowContext;

    class WinIdChangeEventFilter : public QObject {
    public:
        WinIdChangeEventFilter(QObject *host, AbstractWindowContext *context)
            : host(host), context(context) {
        }

        virtual WId winId() const = 0;

    protected:
        QObject *host;
        AbstractWindowContext *context;
    };

    class QWK_CORE_EXPORT WindowWinIdChangeEventFilter : public WinIdChangeEventFilter {
    public:
        WindowWinIdChangeEventFilter(QWindow *host, AbstractWindowContext *context);

        WId winId() const override;

    protected:
        QWindow *win;
        bool isAboutToBeDestroyed;

        bool eventFilter(QObject *obj, QEvent *event) override;
    };

}

#endif // WINIDCHANGEEVENTFILTER_P_H
