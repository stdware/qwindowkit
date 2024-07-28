// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "quickwindowagent.h"
#include "quickwindowagent_p.h"

#include <QtQuick/QQuickWindow>
#include <QtQuick/private/qquickanchors_p.h>

#include "quickitemdelegate_p.h"

namespace QWK {

    /*!
        \class QuickWindowAgent
        \brief QuickWindowAgent is the window agent for QtQuick.

        It provides interfaces for QtQuick and processes some Qt events related to the QQuickItem
        instance. The usage of all APIs is consistent with the \a Widgets module.
    */

    QuickWindowAgentPrivate::QuickWindowAgentPrivate() = default;

    QuickWindowAgentPrivate::~QuickWindowAgentPrivate() = default;

    void QuickWindowAgentPrivate::init() {
    }

    QuickWindowAgent::QuickWindowAgent(QObject *parent)
        : QuickWindowAgent(*new QuickWindowAgentPrivate(), parent) {
    }

    QuickWindowAgent::~QuickWindowAgent() = default;

    bool QuickWindowAgent::setup(QQuickWindow *window) {
        Q_ASSERT(window);
        if (!window) {
            return false;
        }

        Q_D(QuickWindowAgent);
        if (d->hostWindow) {
            return false;
        }

        d->setup(window, new QuickItemDelegate());
        d->hostWindow = window;

#if defined(Q_OS_WINDOWS) && QWINDOWKIT_CONFIG(ENABLE_WINDOWS_SYSTEM_BORDERS)
        d->setupWindows10BorderWorkaround();
#endif
        return true;
    }

    QQuickItem *QuickWindowAgent::titleBar() const {
        Q_D(const QuickWindowAgent);
        return static_cast<QQuickItem *>(d->context->titleBar());
    }

    void QuickWindowAgent::setTitleBar(QQuickItem *item) {
        Q_D(QuickWindowAgent);
        if (!d->context->setTitleBar(item)) {
            return;
        }
#ifdef Q_OS_MAC
        setSystemButtonArea(nullptr);
#endif
        Q_EMIT titleBarWidgetChanged(item);
    }

    QQuickItem *QuickWindowAgent::systemButton(SystemButton button) const {
        Q_D(const QuickWindowAgent);
        return static_cast<QQuickItem *>(d->context->systemButton(button));
    }

    void QuickWindowAgent::setSystemButton(SystemButton button, QQuickItem *item) {
        Q_D(QuickWindowAgent);
        if (!d->context->setSystemButton(button, item)) {
            return;
        }
        Q_EMIT systemButtonChanged(button, item);
    }

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    bool QuickWindowAgent::isHitTestVisible(QQuickItem *item) const {
#else
    bool QuickWindowAgent::isHitTestVisible(const QQuickItem *item) const {
#endif
        Q_D(const QuickWindowAgent);
        return d->context->isHitTestVisible(item);
    }

    void QuickWindowAgent::setHitTestVisible(QQuickItem *item, bool visible) {
        Q_D(QuickWindowAgent);
        d->context->setHitTestVisible(item, visible);
    }

    /*!
        \internal
    */
    QuickWindowAgent::QuickWindowAgent(QuickWindowAgentPrivate &d, QObject *parent)
        : WindowAgentBase(d, parent) {
        d.init();
    }

}
