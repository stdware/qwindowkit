// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "windowbar.h"
#include "windowbar_p.h"

#include <QtCore/QDebug>
#include <QtCore/QLocale>
#include <QtGui/QtEvents>

namespace QWK {

    WindowBarPrivate::WindowBarPrivate() {
        w = nullptr;
        autoTitle = true;
        autoIcon = false;
    }

    WindowBarPrivate::~WindowBarPrivate() = default;

    void WindowBarPrivate::init() {
        Q_Q(WindowBar);
        layout = new QHBoxLayout();
        if (QLocale::system().textDirection() == Qt::RightToLeft) {
            layout->setDirection(QBoxLayout::RightToLeft);
        }

        layout->setContentsMargins(QMargins());
        layout->setSpacing(0);
        for (int i = IconButton; i <= CloseButton; ++i) {
            insertDefaultSpace(i);
        }
        q->setLayout(layout);
    }

    void WindowBarPrivate::setWidgetAt(int index, QWidget *widget) {
        auto item = layout->takeAt(index);
        auto orgWidget = item->widget();
        if (orgWidget) {
            orgWidget->deleteLater();
        }
        delete item;
        if (!widget) {
            insertDefaultSpace(index);
        } else {
            layout->insertWidget(index, widget);
        }
    }

    QWidget *WindowBarPrivate::takeWidgetAt(int index) {
        auto item = layout->itemAt(index);
        auto orgWidget = item->widget();
        if (orgWidget) {
            item = layout->takeAt(index);
            delete item;
            insertDefaultSpace(index);
        }
        return orgWidget;
    }

    WindowBar::WindowBar(QWidget *parent) : WindowBar(*new WindowBarPrivate(), parent) {
    }

    WindowBar::~WindowBar() = default;

    QMenuBar *WindowBar::menuBar() const {
        Q_D(const WindowBar);
        return static_cast<QMenuBar *>(d->widgetAt(WindowBarPrivate::MenuWidget));
    }

    QLabel *WindowBar::titleLabel() const {
        Q_D(const WindowBar);
        return static_cast<QLabel *>(d->widgetAt(WindowBarPrivate::TitleLabel));
    }

    QAbstractButton *WindowBar::iconButton() const {
        Q_D(const WindowBar);
        return static_cast<QAbstractButton *>(d->widgetAt(WindowBarPrivate::IconButton));
    }

    QAbstractButton *WindowBar::pinButton() const {
        Q_D(const WindowBar);
        return static_cast<QAbstractButton *>(d->widgetAt(WindowBarPrivate::PinButton));
    }

    QAbstractButton *WindowBar::minButton() const {
        Q_D(const WindowBar);
        return static_cast<QAbstractButton *>(d->widgetAt(WindowBarPrivate::MinimizeButton));
    }

    QAbstractButton *WindowBar::maxButton() const {
        Q_D(const WindowBar);
        return static_cast<QAbstractButton *>(d->widgetAt(WindowBarPrivate::MaximizeButton));
    }

    QAbstractButton *WindowBar::closeButton() const {
        Q_D(const WindowBar);
        return static_cast<QAbstractButton *>(d->widgetAt(WindowBarPrivate::CloseButton));
    }

    void WindowBar::setMenuBar(QMenuBar *menuBar) {
        Q_D(WindowBar);
        auto org = takeMenuBar();
        if (org)
            org->deleteLater();
        if (!menuBar)
            return;
        d->setWidgetAt(WindowBarPrivate::MenuWidget, menuBar);
        menuBar->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
    }

    void WindowBar::setTitleLabel(QLabel *label) {
        Q_D(WindowBar);
        auto org = takeTitleLabel();
        if (org)
            org->deleteLater();
        if (!label)
            return;
        d->setWidgetAt(WindowBarPrivate::TitleLabel, label);
        if (d->autoTitle && d->w)
            label->setText(d->w->windowTitle());
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    }

    void WindowBar::setIconButton(QAbstractButton *btn) {
        Q_D(WindowBar);
        auto org = takeIconButton();
        if (org)
            org->deleteLater();
        if (!btn)
            return;
        d->setWidgetAt(WindowBarPrivate::IconButton, btn);
        if (d->autoIcon && d->w)
            btn->setIcon(d->w->windowIcon());
        btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    }

    void WindowBar::setPinButton(QAbstractButton *btn) {
        Q_D(WindowBar);
        auto org = takePinButton();
        if (org)
            org->deleteLater();
        if (!btn)
            return;
        d->setWidgetAt(WindowBarPrivate::PinButton, btn);
        connect(btn, &QAbstractButton::clicked, this, &WindowBar::pinRequested);
    }

    void WindowBar::setMinButton(QAbstractButton *btn) {
        Q_D(WindowBar);
        auto org = takeMinButton();
        if (org)
            org->deleteLater();
        if (!btn)
            return;
        d->setWidgetAt(WindowBarPrivate::MinimizeButton, btn);
        connect(btn, &QAbstractButton::clicked, this, &WindowBar::minimizeRequested);
    }

    void WindowBar::setMaxButton(QAbstractButton *btn) {
        Q_D(WindowBar);
        auto org = takeMaxButton();
        if (org)
            org->deleteLater();
        if (!btn)
            return;
        d->setWidgetAt(WindowBarPrivate::MaximizeButton, btn);
        connect(btn, &QAbstractButton::clicked, this, &WindowBar::maximizeRequested);
    }

    void WindowBar::setCloseButton(QAbstractButton *btn) {
        Q_D(WindowBar);
        auto org = takeCloseButton();
        if (org)
            org->deleteLater();
        if (!btn)
            return;
        d->setWidgetAt(WindowBarPrivate::CloseButton, btn);
        connect(btn, &QAbstractButton::clicked, this, &WindowBar::closeRequested);
    }

    QMenuBar *WindowBar::takeMenuBar() {
        Q_D(WindowBar);
        return static_cast<QMenuBar *>(d->takeWidgetAt(WindowBarPrivate::MenuWidget));
    }

    QLabel *WindowBar::takeTitleLabel() {
        Q_D(WindowBar);
        return static_cast<QLabel *>(d->takeWidgetAt(WindowBarPrivate::TitleLabel));
    }

    QAbstractButton *WindowBar::takeIconButton() {
        Q_D(WindowBar);
        return static_cast<QAbstractButton *>(d->takeWidgetAt(WindowBarPrivate::IconButton));
    }

    QAbstractButton *WindowBar::takePinButton() {
        Q_D(WindowBar);
        auto btn = static_cast<QAbstractButton *>(d->takeWidgetAt(WindowBarPrivate::PinButton));
        if (!btn) {
            return nullptr;
        }
        disconnect(btn, &QAbstractButton::clicked, this, &WindowBar::pinRequested);
        return btn;
    }

    QAbstractButton *WindowBar::takeMinButton() {
        Q_D(WindowBar);
        auto btn = static_cast<QAbstractButton *>(d->takeWidgetAt(WindowBarPrivate::MinimizeButton));
        if (!btn) {
            return nullptr;
        }
        disconnect(btn, &QAbstractButton::clicked, this, &WindowBar::minimizeRequested);
        return btn;
    }

    QAbstractButton *WindowBar::takeMaxButton() {
        Q_D(WindowBar);
        auto btn = static_cast<QAbstractButton *>(d->takeWidgetAt(WindowBarPrivate::MaximizeButton));
        if (!btn) {
            return nullptr;
        }
        disconnect(btn, &QAbstractButton::clicked, this, &WindowBar::maximizeRequested);
        return btn;
    }

    QAbstractButton *WindowBar::takeCloseButton() {
        Q_D(WindowBar);
        auto btn = static_cast<QAbstractButton *>(d->takeWidgetAt(WindowBarPrivate::CloseButton));
        if (!btn) {
            return nullptr;
        }
        disconnect(btn, &QAbstractButton::clicked, this, &WindowBar::closeRequested);
        return btn;
    }

    QWidget *WindowBar::hostWidget() const {
        Q_D(const WindowBar);
        return d->w;
    }

    void WindowBar::setHostWidget(QWidget *w) {
        Q_D(WindowBar);

        QWidget *org = d->w;
        if (org) {
            org->removeEventFilter(this);
        }
        d_ptr->w = w;
        if (w) {
            w->installEventFilter(this);
        }
    }

    bool WindowBar::titleFollowWindow() const {
        Q_D(const WindowBar);
        return d->autoTitle;
    }

    void WindowBar::setTitleFollowWindow(bool value) {
        Q_D(WindowBar);
        d->autoTitle = value;
    }

    bool WindowBar::iconFollowWindow() const {
        Q_D(const WindowBar);
        return d->autoIcon;
    }

    void WindowBar::setIconFollowWindow(bool value) {
        Q_D(WindowBar);
        d->autoIcon = value;
    }

    bool WindowBar::eventFilter(QObject *obj, QEvent *event) {
        Q_D(WindowBar);
        auto w = d->w;
        if (obj == w) {
            QAbstractButton *iconBtn = iconButton();
            QLabel *label = titleLabel();
            QAbstractButton *maxBtn = maxButton();
            switch (event->type()) {
                case QEvent::WindowIconChange: {
                    if (d_ptr->autoIcon && iconBtn) {
                        iconBtn->setIcon(w->windowIcon());
                        iconChanged(w->windowIcon());
                    }
                    break;
                }
                case QEvent::WindowTitleChange: {
                    if (d_ptr->autoTitle && label) {
                        label->setText(w->windowTitle());
                        titleChanged(w->windowTitle());
                    }
                    break;
                }
                case QEvent::WindowStateChange: {
                    if (maxBtn) {
                        maxBtn->setChecked(w->isMaximized());
                    }
                    break;
                }
                default:
                    break;
            }
        }
        return QWidget::eventFilter(obj, event);
    }

    void WindowBar::titleChanged(const QString &text) {
        Q_UNUSED(text)
    }

    void WindowBar::iconChanged(const QIcon &icon){Q_UNUSED(icon)}

    WindowBar::WindowBar(WindowBarPrivate &d, QWidget *parent)
        : QFrame(parent), d_ptr(&d) {
        d.q_ptr = this;

        d.init();
    }

}
