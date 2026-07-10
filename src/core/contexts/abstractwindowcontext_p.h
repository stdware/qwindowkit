// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

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
#include <list>
#include <memory>
#include <utility>

#include <QtCore/QSet>
#include <QtCore/QPointer>
#include <QtGui/QRegion>
#include <QtGui/QWindow>

#include <QWKCore/windowagentbase.h>
#include <QWKCore/private/nativeeventfilter_p.h>
#include <QWKCore/private/sharedeventfilter_p.h>
#include <QWKCore/private/windowitemdelegate_p.h>
#include <QWKCore/private/winidchangeeventfilter_p.h>

namespace QWK {

    class QWK_CORE_EXPORT AbstractWindowContext : public QObject,
                                                  public NativeEventDispatcher,
                                                  public SharedEventDispatcher {
        Q_OBJECT
    public:
        AbstractWindowContext();
        ~AbstractWindowContext() override;

    public:
        void setup(QObject *host, WindowItemDelegate *delegate);

        inline QObject *host() const;
        inline QWindow *window() const;
        inline WId windowId() const;
        inline WindowItemDelegate *delegate() const;

        bool isHitTestVisible(QObject *titleBar, const QObject *obj) const;
        bool setHitTestVisible(QObject *titleBar, QObject *obj, bool visible);

        inline QObject *systemButton(WindowAgentBase::SystemButton button) const;
        bool setSystemButton(WindowAgentBase::SystemButton button, QObject *obj);

        QList<QObject *> titleBars() const;

        bool addTitleBar(QObject *titleBar);
        bool removeTitleBar(QObject *titleBar);
        void clearTitleBars();

        bool isTitleBarRegistered(const QObject *titleBar) const;

#ifdef Q_OS_MAC
        inline ScreenRectCallback systemButtonAreaCallback() const;
        void setSystemButtonAreaCallback(const ScreenRectCallback &callback);
#endif

        bool isInSystemButtons(const QPoint &pos, WindowAgentBase::SystemButton *button) const;
        bool isInTitleBarDraggableArea(const QPoint &pos) const;

        inline bool isHostWidthFixed() const;
        inline bool isHostHeightFixed() const;
        inline bool isHostSizeFixed() const;

        virtual QString key() const;

        enum WindowContextHook {
            CentralizeHook = 1,
            RaiseWindowHook,
            ShowSystemMenuHook,
            DefaultColorsHook,
            DrawWindows10BorderHook_Emulated, // Only works on Windows 10, emulated workaround
            DrawWindows10BorderHook_Native,   // Only works on Windows 10, native workaround
            SystemButtonAreaChangedHook,      // Only works on Mac
        };
        virtual void virtual_hook(int id, void *data);

        void showSystemMenu(const QPoint &pos);
        void notifyWinIdChange();

        virtual QVariant windowAttribute(const QString &key) const;
        virtual bool setWindowAttribute(const QString &key, const QVariant &attribute);

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override;

    protected:
        virtual void winIdChanged(WId winId, WId oldWinId) = 0;
        virtual bool windowAttributeChanged(const QString &key, const QVariant &attribute,
                                            const QVariant &oldAttribute);

    protected:
        QObject *m_host{};
        std::unique_ptr<WindowItemDelegate> m_delegate;
        QPointer<QWindow> m_windowHandle;
        WId m_windowId{};

        struct TitleBarRecord {
            QPointer<QObject> titleBar;
            QVector<QPointer<QObject>> hitTestVisibleItems;
        };

        /*
            A hit-test-visible item is meaningful only inside the title bar that owns it.

             Keeping hit-test items together with their owner title bar avoids stale global
             state and makes title bar removal deterministic: removing a title bar removes
             all interactive exceptions registered for that title bar.
         */
        QVector<TitleBarRecord> m_titleBars;
#ifdef Q_OS_MAC
        ScreenRectCallback m_systemButtonAreaCallback;
#endif
        std::array<QPointer<QObject>, WindowAgentBase::Close + 1> m_systemButtons{};

        std::list<std::pair<QString, QVariant>> m_windowAttributesOrder;
        QHash<QString, decltype(m_windowAttributesOrder)::iterator> m_windowAttributes;

        std::unique_ptr<WinIdChangeEventFilter> m_winIdChangeEventFilter;

        TitleBarRecord *findTitleBarRecord(const QObject *titleBar);
        const TitleBarRecord *findTitleBarRecord(const QObject *titleBar) const;
    };

    inline QObject *AbstractWindowContext::host() const {
        return m_host;
    }

    inline QWindow *AbstractWindowContext::window() const {
        return m_windowHandle;
    }

    inline WId AbstractWindowContext::windowId() const {
        return m_windowId;
    }

    inline WindowItemDelegate *AbstractWindowContext::delegate() const {
        return m_delegate.get();
    }

    inline QObject *
        AbstractWindowContext::systemButton(WindowAgentBase::SystemButton button) const {
        return m_systemButtons[button];
    }

#ifdef Q_OS_MAC
    inline ScreenRectCallback AbstractWindowContext::systemButtonAreaCallback() const {
        return m_systemButtonAreaCallback;
    }
#endif

    inline bool AbstractWindowContext::isHostWidthFixed() const {
        return m_windowHandle
                   ? ((m_windowHandle->flags() & Qt::MSWindowsFixedSizeDialogHint) ||
                      m_windowHandle->minimumWidth() == m_windowHandle->maximumWidth())
                   : false;
    }

    inline bool AbstractWindowContext::isHostHeightFixed() const {
        return m_windowHandle
                   ? ((m_windowHandle->flags() & Qt::MSWindowsFixedSizeDialogHint) ||
                      m_windowHandle->minimumHeight() == m_windowHandle->maximumHeight())
                   : false;
    }

    inline bool AbstractWindowContext::isHostSizeFixed() const {
        return m_windowHandle ? ((m_windowHandle->flags() & Qt::MSWindowsFixedSizeDialogHint) ||
                                 m_windowHandle->minimumSize() == m_windowHandle->maximumSize())
                              : false;
    }

}

#endif // ABSTRACTWINDOWCONTEXT_P_H
