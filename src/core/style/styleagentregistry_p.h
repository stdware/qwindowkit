// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef STYLEAGENTREGISTRY_P_H
#define STYLEAGENTREGISTRY_P_H

// Implementation detail. All access is confined to the GUI thread.
#include "styleagent_p.h"
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QVector>
#include <utility>

namespace QWK::Private {

    class StyleAgentRegistry {
    public:
        void insert(StyleAgentPrivate *agent) { m_agents.insert(agent); }
        bool remove(StyleAgentPrivate *agent) { return m_agents.remove(agent); }
        bool isEmpty() const { return m_agents.isEmpty(); }

        template <class ReadAppearance>
        void notify(ReadAppearance readAppearance) {
            const auto revision = ++m_revision;
            const auto appearance = readAppearance();
            if (revision != m_revision)
                return;
            struct Entry {
                StyleAgentPrivate *agent;
                QPointer<StyleAgent> owner;
            };
            QVector<Entry> agents;
            agents.reserve(m_agents.size());
            for (auto *agent : std::as_const(m_agents))
                agents.append(Entry{agent, QPointer<StyleAgent>(agent->q_ptr)});

            // Callbacks can unregister, destroy or create agents and deliver a newer
            // OS notification. Stop the old batch as soon as that happens.
            for (const auto &entry : std::as_const(agents)) {
                if (revision != m_revision)
                    return;
                if (!entry.owner || !m_agents.contains(entry.agent))
                    continue;
                entry.agent->notifyThemeChanged(appearance.first);
                if (revision != m_revision)
                    return;
                if (!entry.owner || !m_agents.contains(entry.agent))
                    continue;
                entry.agent->notifyAccentColorChanged(appearance.second);
            }
        }

    private:
        QSet<StyleAgentPrivate *> m_agents;
        quint64 m_revision = 0;
    };

}

#endif // STYLEAGENTREGISTRY_P_H
