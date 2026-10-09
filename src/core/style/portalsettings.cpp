// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include "portalsettings_p.h"
#include <cmath>

namespace QWK {
    PortalStyleObserver::PortalStyleObserver(StyleAgentPrivate *agent, PortalSettingsSource *source)
        : QObject(agent->q_ptr), m_agent(agent), m_source(source), m_refresh(this) {
        source->setParent(this);
        m_refresh.setSingleShot(true);
        connect(&m_refresh, &QTimer::timeout, this, &PortalStyleObserver::refresh);
        connect(source, &PortalSettingsSource::changed, this, [this] {
            m_dirty = true;
            m_refresh.start(0);
        });
        connect(source, &PortalSettingsSource::unavailable, this, &PortalStyleObserver::invalidate);
        connect(source, &PortalSettingsSource::snapshot, this, &PortalStyleObserver::receive);
        // Subscribe before requesting the first snapshot; never block a getter or constructor.
        m_refresh.start(0);
    }

    void PortalStyleObserver::refresh() {
        if (m_pending)
            return; // receive() will discard an obsolete reply and read again.
        m_pending = true;
        m_dirty = false;
        m_source->read();
    }

    void PortalStyleObserver::receive(const QVariantMap &settings, bool valid) {
        m_pending = false;
        if (m_dirty) {
            m_refresh.start(0);
            return;
        }
        // Retry only when unavailable, not when a valid portal simply omits a key.
        if (!valid)
            m_refresh.start(5000);
        publish(valid ? settings : QVariantMap()); // May delete this observer.
    }

    void PortalStyleObserver::invalidate() {
        m_pending = false;
        m_dirty = false;
        m_refresh.start(0);
        publish({}); // Disconnection must not expose an indefinitely stale value.
    }

    void PortalStyleObserver::publish(const QVariantMap &settings) {
        auto theme = StyleAgent::Unknown;
        const auto scheme = settings.value(QStringLiteral("color-scheme"));
        const auto contrast = settings.value(QStringLiteral("contrast"));
        if (contrast.userType() == QMetaType::UInt && contrast.toUInt() == 1) {
            theme = StyleAgent::HighContrast;
        } else if (scheme.userType() == QMetaType::UInt) {
            if (scheme.toUInt() == 1) theme = StyleAgent::Dark;
            if (scheme.toUInt() == 2) theme = StyleAgent::Light;
        }
        QColor color;
        const auto rgb = settings.value(QStringLiteral("accent-color")).toList();
        if (rgb.size() == 3) {
            bool valid = true;
            for (const auto &channel : rgb) {
                valid &= channel.userType() == QMetaType::Double &&
                         std::isfinite(channel.toDouble()) && channel.toDouble() >= 0 &&
                         channel.toDouble() <= 1;
            }
            if (valid)
                color = QColor::fromRgbF(rgb[0].toDouble(), rgb[1].toDouble(), rgb[2].toDouble());
        }
        m_agent->notifyAppearanceChanged(theme, color);
    }
}
