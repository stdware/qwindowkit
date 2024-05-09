// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#ifndef WINDOWITEMDELEGATE_P_H
#define WINDOWITEMDELEGATE_P_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the QWindowKit API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtGui/QWindow>

#include <QWKCore/private/winidchangeeventfilter_p.h>
#include <QWKCore/qwkglobal.h>

namespace QWK {

    class QWK_CORE_EXPORT WindowItemDelegate {
    public:
        WindowItemDelegate();
        virtual ~WindowItemDelegate();

    public:
        // Item property query
        virtual QWindow *window(const QObject *obj) const = 0;
        virtual bool isEnabled(const QObject *obj) const = 0;
        virtual bool isVisible(const QObject *obj) const = 0;
        virtual QRect mapGeometryToScene(const QObject *obj) const = 0;

        // Host property query
        virtual QWindow *hostWindow(const QObject *host) const = 0;
        virtual bool isWindowActive(const QObject *host) const = 0;
        virtual Qt::WindowStates getWindowState(const QObject *host) const = 0;
        virtual Qt::WindowFlags getWindowFlags(const QObject *host) const = 0;
        virtual QRect getGeometry(const QObject *host) const = 0;

        // Callbacks
        virtual void resetQtGrabbedControl(QObject *host) const;
        virtual void setWindowState(QObject *host, Qt::WindowStates state) const = 0;
        virtual void setCursorShape(QObject *host, Qt::CursorShape shape) const = 0;
        virtual void restoreCursorShape(QObject *host) const = 0;
        virtual void setWindowFlags(QObject *host, Qt::WindowFlags flags) const = 0;
        virtual void setWindowVisible(QObject *host, bool visible) const = 0;
        virtual void setGeometry(QObject *host, const QRect &rect) = 0;
        virtual void bringWindowToTop(QObject *host) const = 0;

        // Factories
        virtual WinIdChangeEventFilter *
            createWinIdEventFilter(QObject *host, AbstractWindowContext *context) const;

    private:
        Q_DISABLE_COPY(WindowItemDelegate)
    };

}

#endif // WINDOWITEMDELEGATE_P_H
