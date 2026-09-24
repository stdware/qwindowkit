// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef QUICKSYSTEMBUTTONAREA_P_H
#define QUICKSYSTEMBUTTONAREA_P_H

// Private implementation detail, not part of the QWindowKit API.
#include <QtCore/QPointer>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

namespace QWK {

    // Observe the visual parent chain, which can differ from QObject ownership.
    // This is platform independent so the macOS geometry contract is testable.
    class QuickSystemButtonArea : public QObject {
        Q_OBJECT
    public:
        explicit QuickSystemButtonArea(QQuickItem *item, QObject *parent = nullptr);
        ~QuickSystemButtonArea() override;

        QQuickItem *item() const { return m_item; }
        QRect sceneRect(const QWindow *hostWindow) const;

    Q_SIGNALS:
        void changed();

    private:
        void scheduleRefresh(bool rebuild = false);
        void rebuildConnections();
        void clearConnections();
        void refresh();
        QRect mappedRect() const;

        QPointer<QQuickItem> m_item;
        QList<QMetaObject::Connection> m_connections;
        QRect m_lastRect;
        QQuickWindow *m_lastWindow = nullptr; // Identity only; never dereferenced.
        bool m_hadItem = false;
        bool m_pending = false;
        bool m_rebuild = true;
    };

}
#endif
