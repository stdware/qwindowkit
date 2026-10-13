// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef EVENTDISPATCH_P_H
#define EVENTDISPATCH_P_H

// Implementation detail shared by the native and QObject event dispatchers.
// This file is not part of the QWindowKit API.

#include <QtCore/QList>
#include <memory>
#include <utility>

namespace QWK::Private {

    template <class Filter>
    class EventDispatchState {
    public:
        EventDispatchState() = default;

        template <class Dispatcher>
        void install(Filter *filter, Dispatcher *dispatcher, Dispatcher *Filter::*owner) {
            if (!filter || filter->*owner)
                return;
            m_filters.append(filter);
            filter->*owner = dispatcher;
        }

        template <class Dispatcher>
        void remove(Filter *filter, Dispatcher *Filter::*owner) {
            const qsizetype index = m_filters.indexOf(filter);
            if (index < 0)
                return;
            if (m_depth > 0)
                m_filters[index] = nullptr;
            else
                m_filters.removeAt(index);
            filter->*owner = nullptr;
        }

        // Called by the owning dispatcher before its state is destroyed.
        template <class Dispatcher>
        void detach(Dispatcher *Filter::*owner) {
            *m_alive = false;
            for (Filter *filter : std::as_const(m_filters)) {
                if (filter)
                    filter->*owner = nullptr;
            }
        }

        template <class Callback>
        bool dispatch(Callback callback) {
            // Keep a token for this specific dispatcher lifetime, not its address. A callback
            // may destroy the dispatcher and immediately construct another at the same address.
            const auto alive = m_alive;
            ++m_depth;
            bool filtered = false;
            // New filters participate in this dispatch. Removals leave tombstones until the
            // outermost frame returns, keeping every nested frame's index valid. Never retain
            // a list iterator or reference across callbacks, which may reallocate the list.
            for (qsizetype i = 0; i < m_filters.size(); ++i) {
                Filter *filter = m_filters.at(i);
                if (!filter)
                    continue;
                const bool consumed = callback(filter);
                // Do not touch this state (including depth/cleanup) after owner destruction.
                // Consume the event so the caller cannot forward through its deleted context.
                if (!*alive)
                    return true;
                if (consumed) {
                    filtered = true;
                    break;
                }
            }
            if (--m_depth == 0)
                m_filters.removeAll(nullptr);
            return filtered;
        }

        bool isEmpty() const { return m_filters.isEmpty(); }
        bool isIdleAndEmpty() const { return m_depth == 0 && isEmpty(); }

    private:
        QList<Filter *> m_filters;
        int m_depth = 0;
        std::shared_ptr<bool> m_alive = std::make_shared<bool>(true);

        Q_DISABLE_COPY(EventDispatchState)
    };

}

#endif // EVENTDISPATCH_P_H
