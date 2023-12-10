#ifndef ABSTRACTWINDOWCONTEXT_P_H
#define ABSTRACTWINDOWCONTEXT_P_H

#include <array>
#include <memory>

#include <QtCore/QSet>
#include <QtGui/QWindow>
#include <QtGui/QPolygon>

#include <QWKCore/windowagentbase.h>
#include <QWKCore/sharedeventfilter.h>
#include <QWKCore/private/windowitemdelegate_p.h>

namespace QWK {

    class QWK_CORE_EXPORT AbstractWindowContext : public QObject, public SharedEventDispatcher {
        Q_OBJECT
    public:
        AbstractWindowContext();
        ~AbstractWindowContext() override;

    public:
        bool setup(QObject *host, WindowItemDelegate *delegate);

        inline QObject *host() const;
        inline QWindow *window() const;

        inline bool isHitTestVisible(const QObject *obj) const;
        bool setHitTestVisible(const QObject *obj, bool visible);
        bool setHitTestVisible(const QRect &rect, bool visible);

        inline const QObject *systemButton(WindowAgentBase::SystemButton button) const;
        bool setSystemButton(WindowAgentBase::SystemButton button, const QObject *obj);

        inline const QObject *titleBar() const;
        bool setTitleBar(const QObject *obj);

        virtual void showSystemMenu(const QPoint &pos);

        QRegion hitTestShape() const;
        bool isInSystemButtons(const QPoint &pos, WindowAgentBase::SystemButton *button) const;
        bool isInTitleBarDraggableArea(const QPoint &pos) const;

    protected:
        virtual bool setupHost() = 0;

        QObject *target() const override;

    protected:
        QObject *m_host;
        std::unique_ptr<WindowItemDelegate> m_delegate;
        QWindow *m_windowHandle;

        QSet<const QObject *> m_hitTestVisibleItems;
        QList<QRect> m_hitTestVisibleRects;

        const QObject *m_titleBar{};
        std::array<const QObject *, WindowAgentBase::NumSystemButton> m_systemButtons{};

        // Cached shape
        mutable bool hitTestVisibleShapeDirty{};
        mutable QRegion hitTestVisibleShape;
    };

    inline QObject *AbstractWindowContext::host() const {
        return m_host;
    }

    inline QWindow *AbstractWindowContext::window() const {
        return m_windowHandle;
    }

    inline bool AbstractWindowContext::isHitTestVisible(const QObject *obj) const {
        return m_hitTestVisibleItems.contains(obj);
    }

    inline const QObject *
        AbstractWindowContext::systemButton(WindowAgentBase::SystemButton button) const {
        return m_systemButtons[button];
    }

    inline const QObject *AbstractWindowContext::titleBar() const {
        return m_titleBar;
    }

}

#endif // ABSTRACTWINDOWCONTEXT_P_H
