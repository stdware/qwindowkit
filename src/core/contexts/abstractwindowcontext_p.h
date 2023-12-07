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
        AbstractWindowContext();
        ~AbstractWindowContext() override;

    public:
        bool setup(QObject *host, WindowItemDelegate *delegate);

        inline QObject *host() const;
        inline QWindow *window() const;

        inline bool isHitTestVisible(const QObject *obj) const;
        bool setHitTestVisible(const QObject *obj, bool visible);
        bool setHitTestVisible(const QRect &rect, bool visible);

        inline const QObject *systemButton(CoreWindowAgent::SystemButton button) const;
        bool setSystemButton(CoreWindowAgent::SystemButton button, const QObject *obj);

        inline const QObject *titleBar() const;
        bool setTitleBar(const QObject *obj);

        void showSystemMenu(const QPoint &pos);

        QRegion hitTestShape() const;
        bool isInSystemButtons(const QPoint &pos, CoreWindowAgent::SystemButton *button) const;
        bool isInTitleBarDraggableArea(const QPoint &pos) const;

    protected:
        virtual bool setupHost() = 0;
        virtual bool hostEventFilter(QEvent *event);

    protected:
        QObject *m_host;
        std::unique_ptr<WindowItemDelegate> m_delegate;
        QWindow *m_windowHandle;

        QSet<const QObject *> m_hitTestVisibleItems;
        QList<QRect> m_hitTestVisibleRects;

        const QObject *m_titleBar{};
        std::array<const QObject *, CoreWindowAgent::NumSystemButton> m_systemButtons{};

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
        AbstractWindowContext::systemButton(CoreWindowAgent::SystemButton button) const {
        return m_systemButtons[button];
    }

    inline const QObject *AbstractWindowContext::titleBar() const {
        return m_titleBar;
    }

}

#endif // ABSTRACTWINDOWCONTEXT_P_H
