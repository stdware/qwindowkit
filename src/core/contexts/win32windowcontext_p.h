// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
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

#include <QWKCore/qwindowkit_windows.h>
#include <QWKCore/private/abstractwindowcontext_p.h>

namespace QWK {

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
        void virtual_hook(int id, void *data) override;

        QVariant windowAttribute(const QString &key) const override;

    protected:
        void winIdChanged(WId winId, WId oldWinId) override;
        bool windowAttributeChanged(const QString &key, const QVariant &attribute,
                                    const QVariant &oldAttribute) override;

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
    };

}

#endif // WIN32WINDOWCONTEXT_P_H
