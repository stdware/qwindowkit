// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0
#ifndef PORTALSETTINGS_P_H
#define PORTALSETTINGS_P_H

// Private implementation detail, not part of the QWindowKit API.
#include <QtCore/QObject>
#include <QtCore/QVariantMap>
#include <QtCore/QTimer>
#include "styleagent_p.h"

namespace QWK {
    // Only the D-Bus boundary is substituted in deterministic tests.
    class PortalSettingsSource : public QObject {
        Q_OBJECT
    public:
        using QObject::QObject;
        virtual void read() = 0;
    Q_SIGNALS:
        void changed();
        void unavailable();
        void snapshot(const QVariantMap &settings, bool valid);
    };

    PortalSettingsSource *createDBusPortalSettingsSource();

    class PortalStyleObserver : public QObject {
        Q_OBJECT
    public:
        PortalStyleObserver(StyleAgentPrivate *agent, PortalSettingsSource *source);
    private:
        void refresh();
        void receive(const QVariantMap &settings, bool valid);
        void invalidate();
        void publish(const QVariantMap &settings);
        StyleAgentPrivate *m_agent;
        PortalSettingsSource *m_source;
        QTimer m_refresh;
        bool m_pending = false;
        bool m_dirty = false;
    };
}
#endif
