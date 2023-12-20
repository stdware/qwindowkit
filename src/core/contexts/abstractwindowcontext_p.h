#ifndef ABSTRACTWINDOWCONTEXT_P_H
#define ABSTRACTWINDOWCONTEXT_P_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the QWindowKit API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <array>
#include <memory>

#include <QtCore/QSet>
#include <QtCore/QPointer>
#include <QtGui/QRegion>
#include <QtGui/QWindow>

#include <QWKCore/windowagentbase.h>
#include <QWKCore/private/nativeeventfilter_p.h>
#include <QWKCore/private/windowitemdelegate_p.h>

namespace QWK {

    class QWK_CORE_EXPORT AbstractWindowContext : public QObject, public NativeEventDispatcher {
        Q_OBJECT
    public:
        AbstractWindowContext();
        ~AbstractWindowContext() override;

    public:
        void setup(QObject *host, WindowItemDelegate *delegate);

        inline QObject *host() const;
        inline QWindow *window() const;
        inline WindowItemDelegate *delegate() const;

        inline QVariant windowAttribute(const QString &key) const;
        void setWindowAttribute(const QString &key, const QVariant &var);

        inline bool isHitTestVisible(const QObject *obj) const;
        bool setHitTestVisible(const QObject *obj, bool visible);

        inline QObject *systemButton(WindowAgentBase::SystemButton button) const;
        bool setSystemButton(WindowAgentBase::SystemButton button, QObject *obj);

        inline QObject *titleBar() const;
        bool setTitleBar(QObject *obj);

#ifdef Q_OS_MAC
        inline QRect systemButtonArea() const;
        void setSystemButtonArea(const QRect &rect);
#endif

        bool isInSystemButtons(const QPoint &pos, WindowAgentBase::SystemButton *button) const;
        bool isInTitleBarDraggableArea(const QPoint &pos) const;

        virtual QString key() const;

        enum WindowContextHook {
            CentralizeHook = 1,
            RaiseWindowHook,
            ShowSystemMenuHook,
            DefaultColorsHook,
            WindowAttributeChangedHook,
            DrawWindows10BorderHook,     // Only works on Windows 10
            SystemButtonAreaChangedHook, // Only works on Mac
        };
        virtual void virtual_hook(int id, void *data);

        void showSystemMenu(const QPoint &pos);
        void notifyWinIdChange();

        inline bool isEnabled() const;
        void setEnabled(bool enabled);

    protected:
        virtual void winIdChanged(QWindow *oldWindow, bool isDestroyed) = 0;

    protected:
        QObject *m_host{};
        std::unique_ptr<WindowItemDelegate> m_delegate;
        QWindow *m_windowHandle{};

        QSet<const QObject *> m_hitTestVisibleItems;
#ifdef Q_OS_MAC
        QRect m_systemButtonArea;
#endif

        QObject *m_titleBar{};
        std::array<QObject *, WindowAgentBase::NumSystemButton> m_systemButtons{};

        QVariantHash m_windowAttributes;

    private:
        bool m_internalEnabled = false;
        QPointer<QWindow> m_windowHandleCache;
    };

    inline QObject *AbstractWindowContext::host() const {
        return m_host;
    }

    inline QWindow *AbstractWindowContext::window() const {
        return m_windowHandle;
    }

    inline WindowItemDelegate *AbstractWindowContext::delegate() const {
        return m_delegate.get();
    }

    inline QVariant AbstractWindowContext::windowAttribute(const QString &key) const {
        return m_windowAttributes.value(key);
    }

    inline bool AbstractWindowContext::isHitTestVisible(const QObject *obj) const {
        return m_hitTestVisibleItems.contains(obj);
    }

    inline QObject *
        AbstractWindowContext::systemButton(WindowAgentBase::SystemButton button) const {
        return m_systemButtons[button];
    }

    inline QObject *AbstractWindowContext::titleBar() const {
        return m_titleBar;
    }

#ifdef Q_OS_MAC
    inline QRect AbstractWindowContext::systemButtonArea() const {
        return m_systemButtonArea;
    }
#endif

    inline bool AbstractWindowContext::isEnabled() const {
        return m_internalEnabled;
    }

}

#endif // ABSTRACTWINDOWCONTEXT_P_H
