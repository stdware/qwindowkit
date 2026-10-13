// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include "abstractwindowcontext_p.h"

#include <QtGui/QPen>
#include <QtGui/QPainter>
#include <QtGui/QScreen>

#include "qwkglobal_p.h"

namespace QWK {

    AbstractWindowContext::AbstractWindowContext() = default;

    AbstractWindowContext::~AbstractWindowContext() = default;

    AbstractWindowContext::AttributeChange::AttributeChange(AbstractWindowContext *ctx,
                                                            const QString &attributeKey)
        : context(ctx), key(attributeKey), windowRevision(ctx->m_windowRevision),
          previous(ctx->m_attributeChange) {
        ctx->m_attributeChange = this;
    }

    AbstractWindowContext::AttributeChange::~AttributeChange() {
        if (context)
            context->m_attributeChange = previous;
    }

    void AbstractWindowContext::AttributeChange::supersedePrevious() {
        for (auto frame = previous; frame; frame = frame->previous) {
            if (frame->key == key)
                frame->superseded = true;
        }
    }

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

        if (visible) {
            m_hitTestVisibleItems.removeAll(nullptr);
            m_hitTestVisibleItems.removeAll(obj);
            m_hitTestVisibleItems.append(obj);
        } else {
            for (auto &item : m_hitTestVisibleItems) {
                if (item == obj) {
                    item = nullptr;
                }
            }
        }
        return true;
    }

    bool AbstractWindowContext::setSystemButton(WindowAgentBase::SystemButton button,
                                                QObject *obj) {
        if (!isValidSystemButton(button)) {
            return false;
        }

        auto org = m_systemButtons[button];
        if (org == obj) {
            return false;
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

        if (m_titleBarAssigned) {
            // QPointer is already null after the old title is destroyed, but its
            // registered buttons/exclusions may still live elsewhere in the window.
            removeSystemButtonsAndHitTestItems();
        }
        m_titleBar = item;
        m_titleBarAssigned = true;
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
            if (!currentButton || !m_delegate->isVisible(currentButton) || !m_delegate->isEnabled(currentButton)) {
                continue;
            }
            if (m_delegate->containsScenePoint(currentButton, pos)) {
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

        if (!m_delegate->containsScenePoint(m_titleBar, pos)) {
            return false;
        }

        WindowAgentBase::SystemButton button;
        if (isInSystemButtons(pos, &button)) {
            return false;
        }

        for (auto &&item : std::as_const(m_hitTestVisibleItems)) {
            if (item && m_delegate->isVisible(item) &&
                m_delegate->containsScenePoint(item, pos)) {
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
        const QPointer<AbstractWindowContext> self(this);
        auto oldWinId = m_windowId;
        m_windowId = m_winIdChangeEventFilter->winId();

        // In Qt6, after QWidget::close() is called, the related QWindow's all surfaces and the
        // platform window will be removed, and the WinId will be set to 0. After that, when the
        // QWidget is shown again, the whole things will be recreated again.
        // As a result, we must update our WindowContext each time the WinId changes.
        const auto oldWindow = m_windowHandle;
        if (m_windowHandle) {
            m_windowHandle->removeEventFilter(this);
        }
        m_windowHandle = m_delegate->hostWindow(m_host);
        if (m_windowHandle) {
            m_windowHandle->installEventFilter(this);
        }

        if (oldWinId != m_windowId || oldWindow != m_windowHandle)
            ++m_windowRevision;

        if (oldWinId != m_windowId) {
            const auto revision = m_windowRevision;
            winIdChanged(m_windowId, oldWinId);
            if (!self || m_windowRevision != revision)
                return;

            if (m_windowId) {
                // Each original revision is replayed at most once. Updates made by
                // callbacks apply themselves; never replay a superseded snapshot.
                const auto attributes = m_windowAttributesOrder;
                for (const auto &attribute : attributes) {
                    auto it = m_windowAttributes.constFind(attribute.key);
                    if (it == m_windowAttributes.cend() ||
                        it.value()->revision != attribute.revision)
                        continue;
                    AttributeChange change(this, attribute.key);
                    const bool accepted = windowAttributeChanged(attribute.key, attribute.value);
                    if (!self || m_windowRevision != revision)
                        return;
                    if (!accepted && change.isCurrent()) {
                        // Re-find after the callback: it may rehash, erase or splice.
                        it = m_windowAttributes.constFind(attribute.key);
                        if (it != m_windowAttributes.cend() &&
                            it.value()->revision == attribute.revision) {
                            m_windowAttributesOrder.erase(it.value());
                            m_windowAttributes.remove(attribute.key);
                        }
                    }
                }
            }

            // Send to shared dispatchers
            QEvent e(QEvent::WinIdChange);
            sharedDispatch(m_host, &e);
        }
    }

    QVariant AbstractWindowContext::windowAttribute(const QString &key) const {
        auto it = m_windowAttributes.find(key);
        if (it == m_windowAttributes.end()) {
            return {};
        }
        return it.value()->value;
    }

    bool AbstractWindowContext::setWindowAttribute(const QString &key, const QVariant &attribute) {
        // Copy inputs: callers may pass references into
        // storage that a synchronous platform callback changes or destroys.
        const QString name = key;
        const QVariant value = attribute;
        auto it = m_windowAttributes.constFind(name);
        const bool existed = it != m_windowAttributes.cend();
        AttributeChange change(this, name);
        if (!existed && !value.isValid()) {
            change.supersedePrevious();
            return true;
        }
        if (m_windowId && !windowAttributeChanged(name, value))
            return false;
        if (!change.isCurrent())
            return false;

        it = m_windowAttributes.constFind(name);
        if (value.isValid()) {
            const auto revision = ++m_attributeRevision;
            if (it == m_windowAttributes.cend()) {
                m_windowAttributes.insert(name, m_windowAttributesOrder.insert(
                    m_windowAttributesOrder.end(), {name, value, revision}));
            } else {
                const auto listIter = it.value();
                listIter->value = value;
                listIter->revision = revision;
                m_windowAttributesOrder.splice(m_windowAttributesOrder.end(), m_windowAttributesOrder,
                                               listIter);
            }
        } else if (it != m_windowAttributes.cend()) {
            m_windowAttributesOrder.erase(it.value());
            m_windowAttributes.remove(name);
        }
        change.supersedePrevious();
        return true;
    }

    bool AbstractWindowContext::eventFilter(QObject *obj, QEvent *event) {
        if (obj == m_windowHandle && sharedDispatch(obj, event)) {
            return true;
        }
        return QObject::eventFilter(obj, event);
    }

    bool AbstractWindowContext::windowAttributeChanged(const QString &key,
                                                       const QVariant &attribute) {
        return false;
    }

    void AbstractWindowContext::removeSystemButtonsAndHitTestItems() {
        for (auto &button : m_systemButtons) {
            if (!button) {
                continue;
            }
            button = nullptr;
        }
        m_hitTestVisibleItems.clear();
    }

}
