// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#ifndef QUICKITEMDELEGATE_P_H
#define QUICKITEMDELEGATE_P_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the QWindowKit API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <QtCore/QObject>
#include <QtGui/QWindow>

#include <QWKCore/private/windowitemdelegate_p.h>
#include <QWKQuick/qwkquickglobal.h>

namespace QWK {

    class QWK_QUICK_EXPORT QuickItemDelegate : public WindowItemDelegate {
    public:
        QuickItemDelegate();
        ~QuickItemDelegate() override;

    public:
        QWindow *window(const QObject *obj) const override;
        bool isEnabled(const QObject *obj) const override;
        bool isVisible(const QObject *obj) const override;
        QRect mapGeometryToScene(const QObject *obj) const override;

        QWindow *hostWindow(const QObject *host) const override;
        bool isWindowActive(const QObject *host) const override;
        Qt::WindowStates getWindowState(const QObject *host) const override;
        Qt::WindowFlags getWindowFlags(const QObject *host) const override;
        QRect getGeometry(const QObject *host) const override;

        void setWindowState(QObject *host, Qt::WindowStates state) const override;
        void setCursorShape(QObject *host, Qt::CursorShape shape) const override;
        void restoreCursorShape(QObject *host) const override;
        void setWindowFlags(QObject *host, Qt::WindowFlags flags) const override;
        void setWindowVisible(QObject *host, bool visible) const override;
        void setGeometry(QObject *host, const QRect &rect) override;
        void bringWindowToTop(QObject *host) const override;
    };

}

#endif // QUICKITEMDELEGATE_P_H
