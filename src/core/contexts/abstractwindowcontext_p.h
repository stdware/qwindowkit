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
        inline AbstractWindowContext(QWindow *window, WindowItemDelegate *delegate)
            : m_windowHandle(window), m_delegate(delegate) {
        }
        ~AbstractWindowContext() override;

    public:
        virtual bool setup() = 0;

        inline QWindow *window() const;
        void setupWindow(QWindow *window);

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

    protected:
        QWindow *m_windowHandle;
        std::unique_ptr<WindowItemDelegate> m_delegate;

        QSet<QObject *> m_hitTestVisibleItems;
        QList<QRect> m_hitTestVisibleRects;

        QObject *m_titleBar{};
        std::array<QObject *, CoreWindowAgent::NumSystemButton> m_systemButtons{};

        // Cached shape
        mutable bool hitTestVisibleShapeDirty{};
        mutable QRegion hitTestVisibleShape;
    };

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
