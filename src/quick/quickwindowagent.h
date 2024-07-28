// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#ifndef QUICKWINDOWAGENT_H
#define QUICKWINDOWAGENT_H

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

#include <QWKCore/windowagentbase.h>
#include <QWKQuick/qwkquickglobal.h>

namespace QWK {

    class QuickWindowAgentPrivate;

    class QWK_QUICK_EXPORT QuickWindowAgent : public WindowAgentBase {
        Q_OBJECT
        Q_DECLARE_PRIVATE(QuickWindowAgent)
    public:
        explicit QuickWindowAgent(QObject *parent = nullptr);
        ~QuickWindowAgent() override;

    public:
        Q_INVOKABLE bool setup(QQuickWindow *window);

        Q_INVOKABLE QQuickItem *titleBar() const;
        Q_INVOKABLE void setTitleBar(QQuickItem *item);

        Q_INVOKABLE QQuickItem *systemButton(SystemButton button) const;
        Q_INVOKABLE void setSystemButton(SystemButton button, QQuickItem *item);

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        Q_INVOKABLE bool isHitTestVisible(QQuickItem *item) const;
#else
        Q_INVOKABLE bool isHitTestVisible(const QQuickItem *item) const;
#endif
        Q_INVOKABLE void setHitTestVisible(QQuickItem *item, bool visible = true);

#ifdef Q_OS_MAC
        // The system button area APIs are experimental, very likely to change in the future.
        Q_INVOKABLE QQuickItem *systemButtonArea() const;
        Q_INVOKABLE void setSystemButtonArea(QQuickItem *item);

        Q_INVOKABLE ScreenRectCallback systemButtonAreaCallback() const;
        Q_INVOKABLE void setSystemButtonAreaCallback(const ScreenRectCallback &callback);
#endif

    Q_SIGNALS:
        void titleBarWidgetChanged(QQuickItem *item);
        void systemButtonChanged(SystemButton button, QQuickItem *item);

    protected:
        QuickWindowAgent(QuickWindowAgentPrivate &d, QObject *parent = nullptr);
    };

}

#endif // QUICKWINDOWAGENT_H
