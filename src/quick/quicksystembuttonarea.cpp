// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include "quicksystembuttonarea_p.h"

#include <QtCore/QTimer>

namespace QWK {

    QuickSystemButtonArea::QuickSystemButtonArea(QQuickItem *item, QObject *parent)
        : QObject(parent), m_item(item) {
        rebuildConnections();
        m_lastRect = mappedRect();
        m_lastWindow = item ? item->window() : nullptr;
        m_hadItem = item != nullptr;
    }

    QuickSystemButtonArea::~QuickSystemButtonArea() {
        clearConnections();
    }

    QRect QuickSystemButtonArea::mappedRect() const {
        return m_item ? m_item->mapRectToScene(QRectF(QPointF(), m_item->size())).toRect()
                      : QRect();
    }

    QRect QuickSystemButtonArea::sceneRect(const QWindow *hostWindow) const {
        // Never interpret coordinates from an unrelated window as host coordinates.
        // The callback stays installed while detached and becomes usable on reattachment.
        if (!m_item || !hostWindow || m_item->window() != hostWindow)
            return {};
        return mappedRect();
    }

    void QuickSystemButtonArea::clearConnections() {
        for (const auto &connection : m_connections)
            disconnect(connection);
        m_connections.clear();
    }

    void QuickSystemButtonArea::rebuildConnections() {
        clearConnections();
        m_rebuild = false;
        for (auto item = m_item.data(); item; item = item->parentItem()) {
            const auto geometryChanged = [this] { scheduleRefresh(); };
            m_connections.append(connect(item, &QQuickItem::xChanged, this, geometryChanged));
            m_connections.append(connect(item, &QQuickItem::yChanged, this, geometryChanged));
            m_connections.append(connect(item, &QQuickItem::widthChanged, this, geometryChanged));
            m_connections.append(connect(item, &QQuickItem::heightChanged, this, geometryChanged));
            m_connections.append(connect(item, &QQuickItem::scaleChanged, this, geometryChanged));
            m_connections.append(connect(item, &QQuickItem::rotationChanged, this, geometryChanged));
            m_connections.append(connect(item, &QQuickItem::transformOriginChanged, this,
                                         geometryChanged));
            const auto hierarchyChanged = [this] { scheduleRefresh(true); };
            m_connections.append(connect(item, &QQuickItem::parentChanged, this, hierarchyChanged));
            m_connections.append(connect(item, &QQuickItem::windowChanged, this, hierarchyChanged));
            // Defer traversal until the destructor/reparent operation has finished.
            m_connections.append(connect(item, &QObject::destroyed, this, hierarchyChanged));
        }
        if (m_item && m_item->window()) {
            // Qt 5 does not expose a general transform-list change signal. This GUI-thread
            // frame boundary also catches arbitrary QQuickTransform updates, additions and
            // removals, without polling an idle window or reading items on the render thread.
            m_connections.append(connect(m_item->window(), &QQuickWindow::afterAnimating,
                                         this, &QuickSystemButtonArea::refresh));
        }
    }

    void QuickSystemButtonArea::scheduleRefresh(bool rebuild) {
        m_rebuild |= rebuild;
        if (m_pending)
            return;
        m_pending = true;
        QTimer::singleShot(0, this, [this] {
            m_pending = false;
            refresh();
        });
    }

    void QuickSystemButtonArea::refresh() {
        if (m_rebuild)
            rebuildConnections();
        const QRect rect = mappedRect();
        auto window = m_item ? m_item->window() : nullptr;
        const bool hasItem = !m_item.isNull();
        if (rect == m_lastRect && window == m_lastWindow && hasItem == m_hadItem)
            return;
        m_lastRect = rect;
        m_lastWindow = window;
        m_hadItem = hasItem;
        // State is committed before notification. Receivers may reparent the item or
        // destroy this registration; do not touch members after emitting the signal.
        Q_EMIT changed();
    }

}
