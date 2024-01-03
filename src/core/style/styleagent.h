// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#ifndef STYLEAGENT_H
#define STYLEAGENT_H

#include <memory>

#include <QtCore/QObject>
#include <QtGui/QWindow>

#include <QWKCore/qwkglobal.h>

namespace QWK {

    class StyleAgentPrivate;

    class QWK_CORE_EXPORT StyleAgent : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(StyleAgent)
    public:
        explicit StyleAgent(QObject *parent = nullptr);
        ~StyleAgent() override;

        enum SystemTheme {
            Unknown,
            Light,
            Dark,
            HighContrast,
        };
        Q_ENUM(SystemTheme)

    public:
        SystemTheme systemTheme() const;

    Q_SIGNALS:
        void systemThemeChanged();

    protected:
        StyleAgent(StyleAgentPrivate &d, QObject *parent = nullptr);

        const std::unique_ptr<StyleAgentPrivate> d_ptr;
    };

}

#endif // STYLEAGENT_H