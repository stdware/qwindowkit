// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#ifndef WINDOWAGENTBASE_H
#define WINDOWAGENTBASE_H

#include <memory>

#include <QtCore/QObject>

#include <QWKCore/qwkglobal.h>

namespace QWK {

    class WindowAgentBasePrivate;

    class QWK_CORE_EXPORT WindowAgentBase : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(WindowAgentBase)
    public:
        ~WindowAgentBase() override;

        enum SystemButton : quint8 {
            Unknown,
            WindowIcon,
            Help,
            Minimize,
            Maximize,
            Close,
        };
        Q_ENUM(SystemButton)

        enum SpecialBit : quint8 {
            DontCenterWindowBeforeShow
        };
        Q_ENUM(SpecialBit)

        QVariant windowAttribute(const QString &key) const;
        Q_INVOKABLE bool setWindowAttribute(const QString &key, const QVariant &attribute);

        Q_INVOKABLE void setFlag(quint8 bit, bool value = true);
        bool isFlagSet(quint8 bit) const;
        Q_INVOKABLE void toggleFlag(quint8 bit);

    public Q_SLOTS:
        void showSystemMenu(const QPoint &pos); // Only available on Windows now
        void centralize();
        void raise();

    protected:
        explicit WindowAgentBase(WindowAgentBasePrivate &d, QObject *parent = nullptr);

        const std::unique_ptr<WindowAgentBasePrivate> d_ptr;
    };

}

#endif // WINDOWAGENTBASE_H