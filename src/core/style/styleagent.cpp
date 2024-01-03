// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "styleagent.h"
#include "styleagent_p.h"

#include <QtCore/QVariant>

namespace QWK {

    /*!
        \class StyleAgent
        \brief StyleAgent provides some features related to system theme.

        Qt6.6 started to support system theme detection, this class is intended as an auxiliary
        support for lower versions of Qt. If your Qt already supports it, it is recommended that
        you don't include this class in your build system.
    */

    StyleAgentPrivate::StyleAgentPrivate() {
    }

    StyleAgentPrivate::~StyleAgentPrivate() {
        removeSystemThemeHook();
    }

    void StyleAgentPrivate::init() {
        setupSystemThemeHook();
    }

    void StyleAgentPrivate::notifyThemeChanged(StyleAgent::SystemTheme theme) {
        if (theme == systemTheme)
            return;
        systemTheme = theme;

        Q_Q(StyleAgent);
        Q_EMIT q->systemThemeChanged();
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
    StyleAgent::~StyleAgent() {
    }

    /*!
        Returns the system theme.
    */
    StyleAgent::SystemTheme StyleAgent::systemTheme() const {
        Q_D(const StyleAgent);
        return d->systemTheme;
    }

    /*!
        \internal
    */
    StyleAgent::StyleAgent(StyleAgentPrivate &d, QObject *parent) : QObject(parent), d_ptr(&d) {
        d.q_ptr = this;

        d.init();
    }

    /*!
        \fn void StyleAgent::systemThemeChanged()

        This signal is emitted when the system theme changes.
    */

}
