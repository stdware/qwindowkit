// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "quickitemdelegate_p.h"

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

namespace QWK {

    QuickItemDelegate::QuickItemDelegate() = default;

    QuickItemDelegate::~QuickItemDelegate() = default;

    bool QuickItemDelegate::isEnabled(const QObject *obj) const {
        return static_cast<const QQuickItem *>(obj)->isEnabled();
    }

    bool QuickItemDelegate::isVisible(const QObject *obj) const {
        return static_cast<const QQuickItem *>(obj)->isVisible();
    }

    QRect QuickItemDelegate::mapGeometryToScene(const QObject *obj) const {
        auto item = static_cast<const QQuickItem *>(obj);
        return item->mapRectToScene(QRectF(QPointF(), item->size())).toAlignedRect();
    }

    bool QuickItemDelegate::containsScenePoint(const QObject *obj, const QPoint &pos) const {
        auto item = static_cast<const QQuickItem *>(obj);
        // Invert the full item-to-scene transform, including ancestors. An axis-aligned
        // scene bounding box includes empty corners when the item is rotated.
        bool invertible = false;
        const auto sceneToItem = item->itemTransform(nullptr, nullptr).inverted(&invertible);
        if (!invertible) {
            // A collapsed item has no hit area; QTransform otherwise returns identity.
            return false;
        }
        const auto local = sceneToItem.map(QPointF(pos));
        return local.x() >= 0 && local.x() < item->width() &&
               local.y() >= 0 && local.y() < item->height();
    }

    QWindow *QuickItemDelegate::hostWindow(const QObject *host) const {
        return static_cast<QQuickWindow *>(const_cast<QObject *>(host));
    }

    bool QuickItemDelegate::isWindowActive(const QObject *host) const {
        return static_cast<const QQuickWindow *>(host)->isActive();
    }

    Qt::WindowStates QuickItemDelegate::getWindowState(const QObject *host) const {
        return static_cast<const QQuickWindow *>(host)->windowStates();
    }

    void QuickItemDelegate::setWindowState(QObject *host, Qt::WindowStates state) const {
        static_cast<QQuickWindow *>(host)->setWindowStates(state);
    }

    void QuickItemDelegate::setCursorShape(QObject *host, const Qt::CursorShape shape) const {
        auto window = static_cast<QQuickWindow *>(host);
        if (!m_savedCursor)
            m_savedCursor = window->cursor();
        window->setCursor(QCursor(shape));
    }

    void QuickItemDelegate::restoreCursorShape(QObject *host) const {
        if (!m_savedCursor)
            return;
        const auto cursor = *m_savedCursor;
        m_savedCursor.reset();
        static_cast<QQuickWindow *>(host)->setCursor(cursor);
    }

    Qt::WindowFlags QuickItemDelegate::getWindowFlags(const QObject *host) const {
        return static_cast<const QQuickWindow *>(host)->flags();
    }

    QRect QuickItemDelegate::getGeometry(const QObject *host) const {
        return static_cast<const QQuickWindow *>(host)->geometry();
    }

    void QuickItemDelegate::setWindowFlags(QObject *host, Qt::WindowFlags flags) const {
        static_cast<QQuickWindow *>(host)->setFlags(flags);
    }

    void QuickItemDelegate::setWindowVisible(QObject *host, bool visible) const {
        static_cast<QQuickWindow *>(host)->setVisible(visible);
    }

    void QuickItemDelegate::setGeometry(QObject *host, const QRect &rect) {
        static_cast<QQuickWindow *>(host)->setGeometry(rect);
    }

    void QuickItemDelegate::bringWindowToTop(QObject *host) const {
        static_cast<QQuickWindow *>(host)->raise();
    }

}
