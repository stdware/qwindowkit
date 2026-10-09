// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "styleagent.h"
#include "styleagent_p.h"

#include <QtCore/QPointer>

namespace QWK {

    /*!
        \class StyleAgent
        \brief StyleAgent provides some features related to system theme.

        Qt6.6 started to support system theme detection, this class is intended as an auxiliary
        support for lower versions of Qt. If your Qt already supports it, it is recommended that
        you don't include this class in your build system.

        Create and use this object on the GUI thread. On Linux, Qt DBus reads the
        XDG Desktop Portal Settings interface asynchronously and subscribes to
        appearance changes. Until the initial reply, the theme is Unknown and
        the accent color is invalid. Unsupported or unavailable settings have
        the same values; application palette overrides are not system settings.
        ReadAll calls have a 1000 ms timeout; failed reads retry after 5000 ms.
        Service/connection loss invalidates the cache and starts recovery.
        An active event loop and a working portal backend are required.
    */

    StyleAgentPrivate::StyleAgentPrivate() = default;

    StyleAgentPrivate::~StyleAgentPrivate() {
        removeSystemThemeHook();
    }

    void StyleAgentPrivate::notifyAppearanceChanged(StyleAgent::SystemTheme theme,
                                                    const QColor &color) {
        const bool themeChanged = theme != systemTheme;
        const bool colorChanged = color != systemAccentColor;
        if (!themeChanged && !colorChanged)
            return;
        systemTheme = theme;
        systemAccentColor = color;
        accentNotificationPending |= colorChanged;

        // Both getters must expose the same snapshot in either notification. A slot
        // can delete this agent or publish a newer snapshot. A nested theme-only
        // change must still deliver the pending accent notification exactly once.
        QPointer<StyleAgent> owner(q_ptr);
        if (themeChanged)
            Q_EMIT q_ptr->systemThemeChanged();
        if (!owner)
            return;
        if (accentNotificationPending) {
            accentNotificationPending = false;
            Q_EMIT q_ptr->systemAccentColorChanged();
        }
    }

    /*!
        Constructor. Since it is not related to a concrete window instance, it is better to be used
        as a singleton.
    */
    StyleAgent::StyleAgent(QObject *parent) : StyleAgent(*new StyleAgentPrivate(), parent) {
    }

    /*!
        Destructor.
    */
    StyleAgent::~StyleAgent() = default;

    /*!
        Returns the last observed system theme, or Unknown when unavailable or
        when the Linux portal reports no preference. A high-contrast preference
        takes precedence over the Linux color-scheme preference.
    */
    StyleAgent::SystemTheme StyleAgent::systemTheme() const {
        Q_D(const StyleAgent);
        return d->systemTheme;
    }

    /*!
        Returns the last observed system accent color, or an invalid QColor when
        unavailable. Linux reads the portal accent-color key, not the app palette.
    */
    QColor StyleAgent::systemAccentColor() const {
        Q_D(const StyleAgent);
        return d->systemAccentColor;
    }

    /*!
        \internal
    */
    StyleAgent::StyleAgent(StyleAgentPrivate &d, QObject *parent) : QObject(parent), d_ptr(&d) {
        d.q_ptr = this;

        d.setupSystemThemeHook();
    }

    /*!
        \fn void StyleAgent::systemThemeChanged()

        This signal is emitted when the observed system theme changes, including
        transitions to Unknown. Both appearance getters are updated before either
        change signal is emitted. Equal values do not emit another signal.
    */

    /*!
        \fn void StyleAgent::systemAccentColorChanged()

        This signal is emitted when the observed system accent color changes,
        including transitions to an invalid color. Both appearance getters are
        updated before either change signal is emitted.
    */

}
