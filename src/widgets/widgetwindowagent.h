// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#ifndef WIDGETWINDOWAGENT_H
#define WIDGETWINDOWAGENT_H

#include <QtWidgets/QWidget>

#include <QWKCore/windowagentbase.h>
#include <QWKWidgets/qwkwidgetsglobal.h>

namespace QWK {

    class WidgetWindowAgentPrivate;

    class QWK_WIDGETS_EXPORT WidgetWindowAgent : public WindowAgentBase {
        Q_OBJECT
        Q_DECLARE_PRIVATE(WidgetWindowAgent)
    public:
        explicit WidgetWindowAgent(QObject *parent = nullptr);
        ~WidgetWindowAgent() override;

    public:
        bool setup(QWidget *w);

        QWidget *titleBar() const;
        void setTitleBar(QWidget *w);

        QWidget *systemButton(SystemButton button) const;
        void setSystemButton(SystemButton button, QWidget *w);

#ifdef Q_OS_MAC
        // The system button area APIs are experimental, very likely to change in the future.
        QWidget *systemButtonArea() const;
        void setSystemButtonArea(QWidget *widget);

        ScreenRectCallback systemButtonAreaCallback() const;
        void setSystemButtonAreaCallback(const ScreenRectCallback &callback);
#endif

        bool isHitTestVisible(const QWidget *w) const;
        void setHitTestVisible(QWidget *w, bool visible = true);

    Q_SIGNALS:
        void titleBarChanged(QWidget *w);
        void systemButtonChanged(SystemButton button, QWidget *w);

    protected:
        WidgetWindowAgent(WidgetWindowAgentPrivate &d, QObject *parent = nullptr);
    };

}

#endif // WIDGETWINDOWAGENT_H