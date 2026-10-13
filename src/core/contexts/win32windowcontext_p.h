// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#ifndef WIN32WINDOWCONTEXT_P_H
#define WIN32WINDOWCONTEXT_P_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the QWindowKit API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <QWKCore/qwkconfig.h>
#include <QWKCore/qwindowkit_windows.h>
#include <QWKCore/private/abstractwindowcontext_p.h>

namespace QWK {

    struct ACCENT_POLICY;

    class Win32WindowContext : public AbstractWindowContext {
        Q_OBJECT
    public:
        Win32WindowContext();
        ~Win32WindowContext() override;

        enum WindowPart {
            Outside,
            ClientArea,
            ChromeButton,
            ResizeBorder,
            FixedBorder,
            TitleBar,
        };
        Q_ENUM(WindowPart)

        QString key() const override;
        void raiseWindow() override;
        void showSystemMenu(const QPoint &pos) override;
#if QWINDOWKIT_CONFIG(ENABLE_WINDOWS_SYSTEM_BORDERS)
        QColor windows10BorderColor() const override;
        void setWindows10BorderActive(bool active) override;
        void drawWindows10Border() override;
#endif

        QVariant windowAttribute(const QString &key) const override;

    protected:
        void winIdChanged(WId winId, WId oldWinId) override;
        bool windowAttributeChanged(const QString &key, const QVariant &attribute) override;
        QMargins effectiveExtraMargins(QMargins margins) const;
        virtual bool extendFrameMargins(const QMargins &margins);
        bool applyFrameMargins(const QMargins &margins);
        // Narrow DWM boundary for backdrop capability and failure-path regression tests.
        virtual bool supportsSystemBackdrop() const;
        virtual HRESULT querySystemBackdrop(int *type) const;
        virtual HRESULT setSystemBackdrop(int type);
        virtual bool supportsLegacyMica() const;
        virtual HRESULT setWindowDwmAttribute(DWORD attribute, const void *value, DWORD size);
        virtual bool setBlurBehind(bool enable);
        virtual bool supportsLegacyAcrylic() const;
        virtual bool setAccentPolicy(const ACCENT_POLICY &policy);

    public:
        bool windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *result);

        bool systemMenuHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam,
                               LRESULT *result);

        // In order to perfectly apply Windows 11 Snap Layout into the Qt window, we need to
        // intercept and emulate most of the  mouse events, so that the processing logic
        // is quite complex. Simultaneously, in order to make the handling code of other
        // Windows messages clearer, we have separated them into this function.
        bool snapLayoutHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam,
                               LRESULT *result);

        bool customWindowHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam,
                                 LRESULT *result);

        bool nonClientCalcSizeHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam,
                                      LRESULT *result);

    protected:
        // The last hit test result, helpful to handle WM_MOUSEMOVE and WM_NCMOUSELEAVE.
        WindowPart lastHitTestResult = WindowPart::Outside;
        int lastHitTestResultRaw = HTNOWHERE;

        // Whether the last mouse leave message is blocked, mainly for handling the unexpected
        // WM_MOUSELEAVE.
        bool mouseLeaveBlocked = false;

        // For emulating traditional icon button behavior
        uint64_t iconButtonClickTime = 0;
        int iconButtonClickLevel = 0;

        // Attributes
        bool noSystemMenu = false;
        bool windows10BorderInactive = false;
        QMargins appliedFrameMargins;
        quint64 frameMarginsRevision = 0;
        quint64 materialRevision = 0;

        // Native HWNDs can be recreated while the logical QWindow stays alive. Keep the last
        // stable native frame rect so we can prevent Qt's recreate path from applying a stale
        // client-to-frame offset.
        RECT frameRectBeforeWinIdChange{};
        bool hasFrameRectBeforeWinIdChange = false;
        RECT pendingFrameRectAfterWinIdChange{};
        bool hasPendingFrameRectAfterWinIdChange = false;
        bool restoringFrameRectAfterWinIdChange = false;
    };

}

#endif // WIN32WINDOWCONTEXT_P_H
