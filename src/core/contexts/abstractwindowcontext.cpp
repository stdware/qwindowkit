// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "abstractwindowcontext_p.h"

#include <QtGui/QPen>
#include <QtGui/QPainter>
#include <QtGui/QScreen>

#include "qwkglobal_p.h"

namespace QWK {

    AbstractWindowContext::AbstractWindowContext() = default;

    AbstractWindowContext::~AbstractWindowContext() = default;

    void AbstractWindowContext::setup(QObject *host, WindowItemDelegate *delegate) {
        if (m_host || !host || !delegate) {
            return;
        }
        m_host = host;
        m_delegate.reset(delegate);
        m_winIdChangeEventFilter.reset(delegate->createWinIdEventFilter(host, this));
        notifyWinIdChange();
    }

    bool AbstractWindowContext::setHitTestVisible(QObject *obj, bool visible) {
        Q_ASSERT(obj);
        if (!obj) {
            return false;
        }

        auto it = m_hitTestVisibleItems.find(obj);
        if (visible) {
            if (it != m_hitTestVisibleItems.end()) {
                return true;
            }
            connect(obj, &QObject::destroyed, this,
                    &AbstractWindowContext::_q_hitTestVisibleItemDestroyed);
            m_hitTestVisibleItems.insert(obj);
        } else {
            if (it == m_hitTestVisibleItems.end()) {
                return false;
            }
            disconnect(obj, &QObject::destroyed, this,
                       &AbstractWindowContext::_q_hitTestVisibleItemDestroyed);
            m_hitTestVisibleItems.erase(it);
        }
        return true;
    }

    bool AbstractWindowContext::setSystemButton(WindowAgentBase::SystemButton button,
                                                QObject *obj) {
        Q_ASSERT(button != WindowAgentBase::Unknown);
        if (button == WindowAgentBase::Unknown) {
            return false;
        }

        auto org = m_systemButtons[button];
        if (org == obj) {
            return true;
        }

        if (org) {
            disconnect(org, &QObject::destroyed, this,
                       &AbstractWindowContext::_q_systemButtonDestroyed);
        }
        if (obj) {
            connect(obj, &QObject::destroyed, this,
                    &AbstractWindowContext::_q_systemButtonDestroyed);
        }
        m_systemButtons[button] = obj;
        return true;
    }

    bool AbstractWindowContext::setTitleBar(QObject *item) {
        Q_ASSERT(item);
        auto org = m_titleBar;
        if (org == item) {
            return false;
        }

        if (org) {
            // Since the title bar is changed, all items inside it should be dereferenced right away
            removeSystemButtonsAndHitTestItems();
            disconnect(org, &QObject::destroyed, this,
                       &AbstractWindowContext::_q_titleBarDistroyed);
        }
        if (item) {
            connect(item, &QObject::destroyed, this, &AbstractWindowContext::_q_titleBarDistroyed);
        }
        m_titleBar = item;
        return true;
    }

#ifdef Q_OS_MAC
    void AbstractWindowContext::setSystemButtonAreaCallback(const ScreenRectCallback &callback) {
        m_systemButtonAreaCallback = callback;
        virtual_hook(SystemButtonAreaChangedHook, nullptr);
    }
#endif

    bool AbstractWindowContext::isInSystemButtons(const QPoint &pos,
                                                  WindowAgentBase::SystemButton *button) const {
        *button = WindowAgentBase::Unknown;
        for (int i = WindowAgentBase::WindowIcon; i <= WindowAgentBase::Close; ++i) {
            auto currentButton = m_systemButtons[i];
            if (!currentButton || !m_delegate->isVisible(currentButton) ||
                !m_delegate->isEnabled(currentButton)) {
                continue;
            }
            if (m_delegate->mapGeometryToScene(currentButton).contains(pos)) {
                *button = static_cast<WindowAgentBase::SystemButton>(i);
                return true;
            }
        }
        return false;
    }

    bool AbstractWindowContext::isInTitleBarDraggableArea(const QPoint &pos) const {
        if (!m_titleBar) {
            // There's no title bar at all, the mouse will always be in the client area.
            return false;
        }
        if (!m_delegate->isVisible(m_titleBar) || !m_delegate->isEnabled(m_titleBar)) {
            // The title bar is hidden or disabled for some reason, treat it as there's
            // no title bar.
            return false;
        }
        QRect windowRect = {QPoint(0, 0), m_windowHandle->size()};
        QRect titleBarRect = m_delegate->mapGeometryToScene(m_titleBar);
        if (!titleBarRect.intersects(windowRect)) {
            // The title bar is totally outside the window for some reason,
            // also treat it as there's no title bar.
            return false;
        }

        if (!titleBarRect.contains(pos)) {
            return false;
        }

        for (int i = WindowAgentBase::WindowIcon; i <= WindowAgentBase::Close; ++i) {
            auto currentButton = m_systemButtons[i];
            if (currentButton && m_delegate->isVisible(currentButton) &&
                m_delegate->isEnabled(currentButton) &&
                m_delegate->mapGeometryToScene(currentButton).contains(pos)) {
                return false;
            }
        }

        for (auto widget : m_hitTestVisibleItems) {
            if (widget && m_delegate->isVisible(widget) && m_delegate->isEnabled(widget) &&
                m_delegate->mapGeometryToScene(widget).contains(pos)) {
                return false;
            }
        }

        return true;
    }

    QString AbstractWindowContext::key() const {
        return {};
    }

    QWK_USED static constexpr const struct {
        const quint32 activeLight = MAKE_RGBA_COLOR(210, 233, 189, 226);
        const quint32 activeDark = MAKE_RGBA_COLOR(177, 205, 190, 240);
        const quint32 inactiveLight = MAKE_RGBA_COLOR(193, 195, 211, 203);
        const quint32 inactiveDark = MAKE_RGBA_COLOR(240, 240, 250, 255);
    } kSampleColorSet;

    void AbstractWindowContext::virtual_hook(int id, void *data) {
        switch (id) {
            case CentralizeHook: {
                if (!m_windowId)
                    return;

                QRect windowGeometry = m_delegate->getGeometry(m_host);
                QRect screenGeometry = m_windowHandle->screen()->geometry();
                int x = (screenGeometry.width() - windowGeometry.width()) / 2;
                int y = (screenGeometry.height() - windowGeometry.height()) / 2;
                QPoint pos(x, y);
                pos += screenGeometry.topLeft();
                m_delegate->setGeometry(m_host, QRect(pos, windowGeometry.size()));
                return;
            }

            case RaiseWindowHook: {
                if (!m_windowId)
                    return;

                m_delegate->setWindowVisible(m_host, true);
                Qt::WindowStates state = m_delegate->getWindowState(m_host);
                if (state & Qt::WindowMinimized) {
                    m_delegate->setWindowState(m_host, state & ~Qt::WindowMinimized);
                }
                m_delegate->bringWindowToTop(m_host);
                return;
            }

            case DefaultColorsHook: {
                auto &map = *static_cast<QMap<QString, QColor> *>(data);
                map.clear();
                map.insert(QStringLiteral("activeLight"), kSampleColorSet.activeLight);
                map.insert(QStringLiteral("activeDark"), kSampleColorSet.activeDark);
                map.insert(QStringLiteral("inactiveLight"), kSampleColorSet.inactiveLight);
                map.insert(QStringLiteral("inactiveDark"), kSampleColorSet.inactiveDark);
                return;
            }

            default:
                break;
        }
    }

    void AbstractWindowContext::showSystemMenu(const QPoint &pos) {
        virtual_hook(ShowSystemMenuHook, &const_cast<QPoint &>(pos));
    }

    void AbstractWindowContext::notifyWinIdChange() {
        auto oldWinId = m_windowId;
        m_windowId = m_winIdChangeEventFilter->winId();

        // In Qt6, after QWidget::close() is called, the related QWindow's all surfaces and the
        // platform window will be removed, and the WinId will be set to 0. After that, when the
        // QWidget is shown again, the whole things will be recreated again.
        // As a result, we must update our WindowContext each time the WinId changes.
        if (m_windowHandle) {
            removeEventFilter(m_windowHandle);
        }
        m_windowHandle = m_delegate->hostWindow(m_host);

        if (oldWinId != m_windowId) {
            winIdChanged(m_windowId, oldWinId);
        }

        if (m_windowHandle) {
            m_windowHandle->installEventFilter(this);

            // Refresh window attributes
            auto attributes = m_windowAttributes;
            m_windowAttributes.clear();
            for (auto it = attributes.begin(); it != attributes.end(); ++it) {
                if (!windowAttributeChanged(it.key(), it.value(), {})) {
                    continue;
                }
                m_windowAttributes.insert(it.key(), it.value());
            }
        }

        // Send to shared dispatchers
        if (oldWinId != m_windowId) {
            QEvent e(QEvent::WinIdChange);
            sharedDispatch(m_host, &e);
        }
    }

    QVariant AbstractWindowContext::windowAttribute(const QString &key) const {
        return m_windowAttributes.value(key);
    }

    bool AbstractWindowContext::setWindowAttribute(const QString &key, const QVariant &attribute) {
        auto it = m_windowAttributes.find(key);
        if (it == m_windowAttributes.end()) {
            if (!attribute.isValid()) {
                return true;
            }
            if (!m_windowHandle || !windowAttributeChanged(key, attribute, {})) {
                return false;
            }
            m_windowAttributes.insert(key, attribute);
            return true;
        }

        if (it.value() == attribute)
            return true;
        if (!m_windowHandle || !windowAttributeChanged(key, attribute, it.value())) {
            return false;
        }

        if (attribute.isValid()) {
            it.value() = attribute;
        } else {
            m_windowAttributes.erase(it);
        }
        return true;
    }

    bool AbstractWindowContext::eventFilter(QObject *obj, QEvent *event) {
        if (obj == m_windowHandle && sharedDispatch(obj, event)) {
            return true;
        }
        return QObject::eventFilter(obj, event);
    }

    bool AbstractWindowContext::windowAttributeChanged(const QString &key,
                                                       const QVariant &attribute,
                                                       const QVariant &oldAttribute) {
        return false;
    }

    void AbstractWindowContext::removeSystemButtonsAndHitTestItems() {
        for (auto &button : m_systemButtons) {
            if (!button) {
                continue;
            }
            disconnect(button, &QObject::destroyed, this,
                       &AbstractWindowContext::_q_systemButtonDestroyed);
            button = nullptr;
        }
        for (auto &item : m_hitTestVisibleItems) {
            disconnect(item, &QObject::destroyed, this,
                       &AbstractWindowContext::_q_hitTestVisibleItemDestroyed);
        }
        m_hitTestVisibleItems.clear();
    }

    void AbstractWindowContext::_q_titleBarDistroyed(QObject *obj) {
        Q_UNUSED(obj)
        removeSystemButtonsAndHitTestItems();
        m_titleBar = nullptr;
    }

    void AbstractWindowContext::_q_hitTestVisibleItemDestroyed(QObject *obj) {
        m_hitTestVisibleItems.remove(obj);
    }

    void AbstractWindowContext::_q_systemButtonDestroyed(QObject *obj) {
        for (auto &item : m_systemButtons) {
            if (item == obj) {
                item = nullptr;
            }
        }
    }

}
