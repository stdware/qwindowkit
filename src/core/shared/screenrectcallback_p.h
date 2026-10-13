// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SCREENRECTCALLBACK_P_H
#define SCREENRECTCALLBACK_P_H

// Implementation detail. All access is confined to the GUI thread.
#include <QWKCore/qwkglobal.h>
#include <memory>
#include <utility>

namespace QWK::Private {

    class ScreenRectCallbackState {
        struct Entry {
            ScreenRectCallback callback;
            bool current = true;
        };

    public:
        class Snapshot {
        public:
            explicit operator bool() const { return isCurrent() && bool(m_entry->callback); }
            bool isCurrent() const { return m_entry && m_entry->current; }

            QRect operator()(const QSize &size) const {
                if (!*this)
                    return {};
                // Keep the callable alive even if it replaces itself or deletes its owner.
                const QRect result = m_entry->callback(size);
                return isCurrent() ? result : QRect{};
            }

        private:
            friend class ScreenRectCallbackState;
            explicit Snapshot(const std::shared_ptr<Entry> &entry) : m_entry(entry) {}
            std::shared_ptr<Entry> m_entry;
        };

        ScreenRectCallbackState() = default;
        Q_DISABLE_COPY(ScreenRectCallbackState)
        ~ScreenRectCallbackState() { invalidate(); }

        void set(const ScreenRectCallback &callback) {
            auto next = std::make_shared<Entry>(Entry{callback});
            invalidate();
            m_entry = std::move(next);
        }

        explicit operator bool() const { return bool(snapshot()); }
        Snapshot snapshot() const { return Snapshot(m_entry); }

    private:
        void invalidate() {
            if (m_entry)
                m_entry->current = false;
        }
        std::shared_ptr<Entry> m_entry;
    };

}

#endif // SCREENRECTCALLBACK_P_H
