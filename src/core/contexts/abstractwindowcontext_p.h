#ifndef ABSTRACTWINDOWCONTEXT_P_H
#define ABSTRACTWINDOWCONTEXT_P_H

#include <array>
#include <memory>

#include <QtCore/QSet>
#include <QtGui/QWindow>
#include <QtGui/QPolygon>

#include <QWKCore/corewindowagent.h>
#include <QWKCore/windowitemdelegate.h>

namespace QWK {

    class QWK_CORE_EXPORT AbstractWindowContext : public QObject {
        Q_OBJECT
    public:
        AbstractWindowContext(QObject *host, WindowItemDelegate *delegate);
        ~AbstractWindowContext() override;

    public:
        virtual bool setup() = 0;

        inline QObject *host() const;
        inline QWindow *window() const;

        inline bool isHitTestVisible(QObject *obj) const;
        bool setHitTestVisible(QObject *obj, bool visible);
        bool setHitTestVisible(const QRect &rect, bool visible);

        inline QObject *systemButton(CoreWindowAgent::SystemButton button) const;
        bool setSystemButton(CoreWindowAgent::SystemButton button, QObject *obj);

        inline QObject *titleBar() const;
        bool setTitleBar(QObject *obj);

        void showSystemMenu(const QPoint &pos);

        QRegion hitTestShape() const;
        bool isInSystemButtons(const QPoint &pos, CoreWindowAgent::SystemButton *button) const;
        bool isInTitleBarDraggableArea(const QPoint &pos) const;

    protected:
        QObject *m_host;
        std::unique_ptr<WindowItemDelegate> m_delegate;
        QWindow *m_windowHandle;

        QSet<QObject *> m_hitTestVisibleItems;
        QList<QRect> m_hitTestVisibleRects;

        QObject *m_titleBar{};
        std::array<QObject *, CoreWindowAgent::NumSystemButton> m_systemButtons{};

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

    inline bool AbstractWindowContext::isHitTestVisible(QObject *obj) const {
        return m_hitTestVisibleItems.contains(obj);
    }

    inline QObject *
        AbstractWindowContext::systemButton(CoreWindowAgent::SystemButton button) const {
        return m_systemButtons[button];
    }

    inline QObject *AbstractWindowContext::titleBar() const {
        return m_titleBar;
    }

}

#endif // ABSTRACTWINDOWCONTEXT_P_H
