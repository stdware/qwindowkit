#include "abstractwindowcontext_p.h"

namespace QWK {

    AbstractWindowContext::AbstractWindowContext(QObject *host, WindowItemDelegate *delegate)
        : m_host(host), m_delegate(delegate), m_windowHandle(delegate->hostWindow(host)) {
    }

    AbstractWindowContext::~AbstractWindowContext() = default;

    bool AbstractWindowContext::setHitTestVisible(QObject *obj, bool visible) {
        Q_ASSERT(obj);
        if (!obj) {
            return false;
        }

        if (visible) {
            m_hitTestVisibleItems.insert(obj);
        } else {
            m_hitTestVisibleItems.remove(obj);
        }
        return true;
    }

    bool AbstractWindowContext::setHitTestVisible(const QRect &rect, bool visible) {
        Q_ASSERT(rect.isValid());
        if (!rect.isValid()) {
            return false;
        }

        if (visible) {
            m_hitTestVisibleRects.append(rect);
        } else {
            m_hitTestVisibleRects.removeAll(rect);
        }
        hitTestVisibleShapeDirty = true;
        return true;
    }

    bool AbstractWindowContext::setSystemButton(CoreWindowAgent::SystemButton button,
                                                QObject *obj) {
        Q_ASSERT(obj);
        Q_ASSERT(button != CoreWindowAgent::Unknown);
        if (!obj || (button == CoreWindowAgent::Unknown)) {
            return false;
        }

        if (m_systemButtons[button] == obj) {
            return false;
        }
        m_systemButtons[button] = obj;
        return true;
    }

    bool AbstractWindowContext::setTitleBar(QObject *item) {
        Q_ASSERT(item);
        if (!item) {
            return false;
        }

        if (m_titleBar == item) {
            return false;
        }
        m_titleBar = item;
        return true;
    }

    void AbstractWindowContext::showSystemMenu(const QPoint &pos) {
        // ?
    }

    QRegion AbstractWindowContext::hitTestShape() const {
        if (hitTestVisibleShapeDirty) {
            hitTestVisibleShape = {};
            for (const auto &rect : m_hitTestVisibleRects) {
                hitTestVisibleShape += rect;
            }
            hitTestVisibleShapeDirty = false;
        }
        return hitTestVisibleShape;
    }

    bool AbstractWindowContext::isInSystemButtons(const QPoint &pos,
                                                  CoreWindowAgent::SystemButton *button) const {
        *button = CoreWindowAgent::Unknown;
        for (int i = CoreWindowAgent::WindowIcon; i <= CoreWindowAgent::Close; ++i) {
            auto currentButton = m_systemButtons[i];
            if (!currentButton || !m_delegate->isVisible(currentButton) ||
                !m_delegate->isEnabled(currentButton)) {
                continue;
            }
            if (m_delegate->mapGeometryToScene(currentButton).contains(pos)) {
                *button = CoreWindowAgent::WindowIcon;
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

        if (!m_delegate->mapGeometryToScene(m_titleBar).contains(pos)) {
            return false;
        }

        for (int i = CoreWindowAgent::WindowIcon; i <= CoreWindowAgent::Close; ++i) {
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

        if (!m_hitTestVisibleRects.isEmpty() && hitTestShape().contains(pos)) {
            return false;
        }
        return true;
    }

}