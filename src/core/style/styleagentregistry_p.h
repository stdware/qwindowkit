// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef STYLEAGENTREGISTRY_P_H
#define STYLEAGENTREGISTRY_P_H

// Private synchronous notification support for Windows and macOS.
// Platform subscriptions and the asynchronous Linux portal remain separate.
#include "styleagent_p.h"

#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QVector>
#include <utility>

namespace QWK {

    class StyleAgentRegistry {
    public:
        struct Appearance {
            StyleAgent::SystemTheme theme;
            QColor accentColor;
        };

        void insert(StyleAgentPrivate *agent) { m_agents.insert(agent); }
        bool remove(StyleAgentPrivate *agent) { return m_agents.remove(agent); }
        bool isEmpty() const { return m_agents.isEmpty(); }

        template <class ReadAppearance>
        void notify(ReadAppearance readAppearance) {
            // Increment before platform reads: even reentry while sampling supersedes this run.
            const auto revision = ++m_notificationRevision;
            const auto appearance = readAppearance();

            struct Entry {
                StyleAgentPrivate *agent;
                QPointer<StyleAgent> owner;
            };
            QVector<Entry> agents;
            agents.reserve(m_agents.size());
            for (auto agent : std::as_const(m_agents))
                agents.append({agent, QPointer<StyleAgent>(agent->q_ptr)});

            // Signals may add/remove agents. New agents wait for a subsequent snapshot.
            // QPointer rejects a destroyed owner even if its address is reused; membership
            // rejects a private object unregistered before QObject clears its guards.
            for (const auto &entry : std::as_const(agents)) {
                if (revision != m_notificationRevision)
                    return;
                if (entry.owner && m_agents.contains(entry.agent))
                    entry.agent->notifyAppearanceChanged(appearance.theme, appearance.accentColor);
            }
        }

    private:
        QSet<StyleAgentPrivate *> m_agents;
        quint64 m_notificationRevision = 0;
    };

}

#endif // STYLEAGENTREGISTRY_P_H
