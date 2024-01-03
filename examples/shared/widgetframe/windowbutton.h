// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#ifndef WINDOWBUTTON_H
#define WINDOWBUTTON_H

#include <QtWidgets/QPushButton>

namespace QWK {

    class WindowButtonPrivate;

    class WindowButton : public QPushButton {
        Q_OBJECT
        Q_DECLARE_PRIVATE(WindowButton)
        Q_PROPERTY(QIcon iconNormal READ iconNormal WRITE setIconNormal FINAL)
        Q_PROPERTY(QIcon iconChecked READ iconChecked WRITE setIconChecked FINAL)
        Q_PROPERTY(QIcon iconDisabled READ iconDisabled WRITE setIconDisabled FINAL)
    public:
        explicit WindowButton(QWidget *parent = nullptr);
        ~WindowButton();

    public:
        QIcon iconNormal() const;
        void setIconNormal(const QIcon &icon);

        QIcon iconChecked() const;
        void setIconChecked(const QIcon &icon);

        QIcon iconDisabled() const;
        void setIconDisabled(const QIcon &icon);

    Q_SIGNALS:
        void doubleClicked();

    protected:
        void checkStateSet() override;

        void mouseDoubleClickEvent(QMouseEvent *event) override;

    protected:
        WindowButton(WindowButtonPrivate &d, QWidget *parent = nullptr);

        QScopedPointer<WindowButtonPrivate> d_ptr;
    };

}

#endif // WINDOWBUTTON_H