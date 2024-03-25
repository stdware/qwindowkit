// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#ifndef QWKWINDOWSEXTRA_P_H
#define QWKWINDOWSEXTRA_P_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the QWindowKit API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <shellscalingapi.h>
#include <dwmapi.h>
#include <timeapi.h>

#include <QWKCore/qwindowkit_windows.h>
#include <QtCore/private/qsystemlibrary_p.h>

#include <QtGui/QGuiApplication>
#include <QtGui/QStyleHints>
#include <QtGui/QPalette>

// Don't include this header in any header files.

namespace QWK {

    enum _DWMWINDOWATTRIBUTE {
        // [set] BOOL, Allows the use of host backdrop brushes for the window.
        _DWMWA_USE_HOSTBACKDROPBRUSH = 17,

        // Undocumented, the same with DWMWA_USE_IMMERSIVE_DARK_MODE, but available on systems
        // before Win10 20H1.
        _DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 = 19,

        // [set] BOOL, Allows a window to either use the accent color, or dark, according to the
        // user Color Mode preferences.
        _DWMWA_USE_IMMERSIVE_DARK_MODE = 20,

        // [set] WINDOW_CORNER_PREFERENCE, Controls the policy that rounds top-level window corners
        _DWMWA_WINDOW_CORNER_PREFERENCE = 33,

        // [get] UINT, width of the visible border around a thick frame window
        _DWMWA_VISIBLE_FRAME_BORDER_THICKNESS = 37,

        // [get, set] SYSTEMBACKDROP_TYPE, Controls the system-drawn backdrop material of a window,
        // including behind the non-client area.
        _DWMWA_SYSTEMBACKDROP_TYPE = 38,

        // Undocumented, use this value to enable Mica material on Win11 21H2. You should use
        // DWMWA_SYSTEMBACKDROP_TYPE instead on Win11 22H2 and newer.
        _DWMWA_MICA_EFFECT = 1029
    };

    // Types used with DWMWA_SYSTEMBACKDROP_TYPE
    enum _DWM_SYSTEMBACKDROP_TYPE {
        _DWMSBT_AUTO, // [Default] Let DWM automatically decide the system-drawn backdrop for this
                      // window.
        _DWMSBT_NONE, // [Disable] Do not draw any system backdrop.
        _DWMSBT_MAINWINDOW,      // [Mica] Draw the backdrop material effect corresponding to a
                                 // long-lived window.
        _DWMSBT_TRANSIENTWINDOW, // [Acrylic] Draw the backdrop material effect corresponding to a
                                 // transient window.
        _DWMSBT_TABBEDWINDOW,    // [Mica Alt] Draw the backdrop material effect corresponding to a
                                 // window with a tabbed title bar.
    };

    enum WINDOWCOMPOSITIONATTRIB {
        WCA_UNDEFINED = 0,
        WCA_NCRENDERING_ENABLED = 1,
        WCA_NCRENDERING_POLICY = 2,
        WCA_TRANSITIONS_FORCEDISABLED = 3,
        WCA_ALLOW_NCPAINT = 4,
        WCA_CAPTION_BUTTON_BOUNDS = 5,
        WCA_NONCLIENT_RTL_LAYOUT = 6,
        WCA_FORCE_ICONIC_REPRESENTATION = 7,
        WCA_EXTENDED_FRAME_BOUNDS = 8,
        WCA_HAS_ICONIC_BITMAP = 9,
        WCA_THEME_ATTRIBUTES = 10,
        WCA_NCRENDERING_EXILED = 11,
        WCA_NCADORNMENTINFO = 12,
        WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
        WCA_VIDEO_OVERLAY_ACTIVE = 14,
        WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
        WCA_DISALLOW_PEEK = 16,
        WCA_CLOAK = 17,
        WCA_CLOAKED = 18,
        WCA_ACCENT_POLICY = 19,
        WCA_FREEZE_REPRESENTATION = 20,
        WCA_EVER_UNCLOAKED = 21,
        WCA_VISUAL_OWNER = 22,
        WCA_HOLOGRAPHIC = 23,
        WCA_EXCLUDED_FROM_DDA = 24,
        WCA_PASSIVEUPDATEMODE = 25,
        WCA_USEDARKMODECOLORS = 26,
        WCA_CORNER_STYLE = 27,
        WCA_PART_COLOR = 28,
        WCA_DISABLE_MOVESIZE_FEEDBACK = 29,
        WCA_LAST = 30
    };

    enum ACCENT_STATE {
        ACCENT_DISABLED = 0,
        ACCENT_ENABLE_GRADIENT = 1,
        ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
        ACCENT_ENABLE_BLURBEHIND = 3,        // Traditional DWM blur
        ACCENT_ENABLE_ACRYLICBLURBEHIND = 4, // RS4 1803
        ACCENT_ENABLE_HOST_BACKDROP = 5,     // RS5 1809
        ACCENT_INVALID_STATE = 6             // Using this value will remove the window background
    };

    enum ACCENT_FLAG {
        ACCENT_NONE = 0,
        ACCENT_ENABLE_ACRYLIC = 1,
        ACCENT_ENABLE_ACRYLIC_WITH_LUMINOSITY = 482
    };

    struct ACCENT_POLICY {
        DWORD dwAccentState;
        DWORD dwAccentFlags;
        DWORD dwGradientColor; // #AABBGGRR
        DWORD dwAnimationId;
    };
    using PACCENT_POLICY = ACCENT_POLICY *;

    struct WINDOWCOMPOSITIONATTRIBDATA {
        WINDOWCOMPOSITIONATTRIB Attrib;
        PVOID pvData;
        SIZE_T cbData;
    };
    using PWINDOWCOMPOSITIONATTRIBDATA = WINDOWCOMPOSITIONATTRIBDATA *;

    enum PREFERRED_APP_MODE {
        PAM_DEFAULT = 0, // Default behavior on systems before Win10 1809. It indicates the
                         // application doesn't support dark mode at all.
        PAM_AUTO =
            1, // Available since Win10 1809, let system decide whether to enable dark mode or not.
        PAM_DARK = 2, // Available since Win10 1903, force dark mode regardless of the system theme.
        PAM_LIGHT =
            3, // Available since Win10 1903, force light mode regardless of the system theme.
        PAM_MAX = 4
    };

    using SetWindowCompositionAttributePtr = BOOL(WINAPI *)(HWND, PWINDOWCOMPOSITIONATTRIBDATA);

    // Win10 1809 (10.0.17763)
    using RefreshImmersiveColorPolicyStatePtr = VOID(WINAPI *)(VOID); // Ordinal 104
    using AllowDarkModeForWindowPtr = BOOL(WINAPI *)(HWND, BOOL);     // Ordinal 133
    using AllowDarkModeForAppPtr = BOOL(WINAPI *)(BOOL);              // Ordinal 135
    using FlushMenuThemesPtr = VOID(WINAPI *)(VOID);                  // Ordinal 136
    // Win10 1903 (10.0.18362)
    using SetPreferredAppModePtr = PREFERRED_APP_MODE(WINAPI *)(PREFERRED_APP_MODE); // Ordinal 135

    namespace {

        struct DynamicApis {
            static const DynamicApis &instance() {
                static const DynamicApis inst;
                return inst;
            }

#define DYNAMIC_API_DECLARE(NAME) decltype(&::NAME) p##NAME = nullptr

            DYNAMIC_API_DECLARE(DwmFlush);
            DYNAMIC_API_DECLARE(DwmIsCompositionEnabled);
            DYNAMIC_API_DECLARE(DwmGetCompositionTimingInfo);
            DYNAMIC_API_DECLARE(DwmGetWindowAttribute);
            DYNAMIC_API_DECLARE(DwmSetWindowAttribute);
            DYNAMIC_API_DECLARE(DwmExtendFrameIntoClientArea);
            DYNAMIC_API_DECLARE(DwmEnableBlurBehindWindow);
            DYNAMIC_API_DECLARE(GetDpiForWindow);
            DYNAMIC_API_DECLARE(GetSystemMetricsForDpi);
            DYNAMIC_API_DECLARE(AdjustWindowRectExForDpi);
            DYNAMIC_API_DECLARE(GetDpiForMonitor);
            DYNAMIC_API_DECLARE(timeGetDevCaps);
            DYNAMIC_API_DECLARE(timeBeginPeriod);
            DYNAMIC_API_DECLARE(timeEndPeriod);

#undef DYNAMIC_API_DECLARE

            SetWindowCompositionAttributePtr pSetWindowCompositionAttribute = nullptr;
            RefreshImmersiveColorPolicyStatePtr pRefreshImmersiveColorPolicyState = nullptr;
            AllowDarkModeForWindowPtr pAllowDarkModeForWindow = nullptr;
            AllowDarkModeForAppPtr pAllowDarkModeForApp = nullptr;
            FlushMenuThemesPtr pFlushMenuThemes = nullptr;
            SetPreferredAppModePtr pSetPreferredAppMode = nullptr;

        private:
            DynamicApis() {
#define DYNAMIC_API_RESOLVE(DLL, NAME)                                                             \
    p##NAME = reinterpret_cast<decltype(p##NAME)>(DLL.resolve(#NAME))

                QSystemLibrary user32(QStringLiteral("user32"));
                DYNAMIC_API_RESOLVE(user32, GetDpiForWindow);
                DYNAMIC_API_RESOLVE(user32, GetSystemMetricsForDpi);
                DYNAMIC_API_RESOLVE(user32, SetWindowCompositionAttribute);
                DYNAMIC_API_RESOLVE(user32, AdjustWindowRectExForDpi);

                QSystemLibrary shcore(QStringLiteral("shcore"));
                DYNAMIC_API_RESOLVE(shcore, GetDpiForMonitor);

                QSystemLibrary dwmapi(QStringLiteral("dwmapi"));
                DYNAMIC_API_RESOLVE(dwmapi, DwmFlush);
                DYNAMIC_API_RESOLVE(dwmapi, DwmIsCompositionEnabled);
                DYNAMIC_API_RESOLVE(dwmapi, DwmGetCompositionTimingInfo);
                DYNAMIC_API_RESOLVE(dwmapi, DwmGetWindowAttribute);
                DYNAMIC_API_RESOLVE(dwmapi, DwmSetWindowAttribute);
                DYNAMIC_API_RESOLVE(dwmapi, DwmExtendFrameIntoClientArea);
                DYNAMIC_API_RESOLVE(dwmapi, DwmEnableBlurBehindWindow);

                QSystemLibrary winmm(QStringLiteral("winmm"));
                DYNAMIC_API_RESOLVE(winmm, timeGetDevCaps);
                DYNAMIC_API_RESOLVE(winmm, timeBeginPeriod);
                DYNAMIC_API_RESOLVE(winmm, timeEndPeriod);

#undef DYNAMIC_API_RESOLVE

#define UNDOC_API_RESOLVE(DLL, NAME, ORDINAL)                                                      \
    p##NAME = reinterpret_cast<decltype(p##NAME)>(DLL.resolve(MAKEINTRESOURCEA(ORDINAL)))

                QSystemLibrary uxtheme(QStringLiteral("uxtheme"));
                UNDOC_API_RESOLVE(uxtheme, RefreshImmersiveColorPolicyState, 104);
                UNDOC_API_RESOLVE(uxtheme, AllowDarkModeForWindow, 133);
                UNDOC_API_RESOLVE(uxtheme, AllowDarkModeForApp, 135);
                UNDOC_API_RESOLVE(uxtheme, FlushMenuThemes, 136);
                UNDOC_API_RESOLVE(uxtheme, SetPreferredAppMode, 135);

#undef UNDOC_API_RESOLVE
            }

            ~DynamicApis() = default;

            Q_DISABLE_COPY(DynamicApis)
        };

    }

    static inline constexpr bool operator==(const POINT &lhs, const POINT &rhs) noexcept {
        return ((lhs.x == rhs.x) && (lhs.y == rhs.y));
    }

    static inline constexpr bool operator!=(const POINT &lhs, const POINT &rhs) noexcept {
        return !operator==(lhs, rhs);
    }

    static inline constexpr bool operator==(const SIZE &lhs, const SIZE &rhs) noexcept {
        return ((lhs.cx == rhs.cx) && (lhs.cy == rhs.cy));
    }

    static inline constexpr bool operator!=(const SIZE &lhs, const SIZE &rhs) noexcept {
        return !operator==(lhs, rhs);
    }

    static inline constexpr bool operator>(const SIZE &lhs, const SIZE &rhs) noexcept {
        return ((lhs.cx * lhs.cy) > (rhs.cx * rhs.cy));
    }

    static inline constexpr bool operator>=(const SIZE &lhs, const SIZE &rhs) noexcept {
        return (operator>(lhs, rhs) || operator==(lhs, rhs));
    }

    static inline constexpr bool operator<(const SIZE &lhs, const SIZE &rhs) noexcept {
        return (operator!=(lhs, rhs) && !operator>(lhs, rhs));
    }

    static inline constexpr bool operator<=(const SIZE &lhs, const SIZE &rhs) noexcept {
        return (operator<(lhs, rhs) || operator==(lhs, rhs));
    }

    static inline constexpr bool operator==(const RECT &lhs, const RECT &rhs) noexcept {
        return ((lhs.left == rhs.left) && (lhs.top == rhs.top) && (lhs.right == rhs.right) &&
                (lhs.bottom == rhs.bottom));
    }

    static inline constexpr bool operator!=(const RECT &lhs, const RECT &rhs) noexcept {
        return !operator==(lhs, rhs);
    }

    static inline constexpr QPoint point2qpoint(const POINT &point) {
        return QPoint{int(point.x), int(point.y)};
    }

    static inline constexpr POINT qpoint2point(const QPoint &point) {
        return POINT{LONG(point.x()), LONG(point.y())};
    }

    static inline constexpr QSize size2qsize(const SIZE &size) {
        return QSize{int(size.cx), int(size.cy)};
    }

    static inline constexpr SIZE qsize2size(const QSize &size) {
        return SIZE{LONG(size.width()), LONG(size.height())};
    }

    static inline constexpr QRect rect2qrect(const RECT &rect) {
        return QRect{
            QPoint{int(rect.left),        int(rect.top)         },
            QSize{int(RECT_WIDTH(rect)), int(RECT_HEIGHT(rect))}
        };
    }

    static inline constexpr RECT qrect2rect(const QRect &qrect) {
        return RECT{LONG(qrect.left()), LONG(qrect.top()), LONG(qrect.right()),
                    LONG(qrect.bottom())};
    }

    static inline constexpr QMargins margins2qmargins(const MARGINS &margins) {
        return {margins.cxLeftWidth, margins.cyTopHeight, margins.cxRightWidth,
                margins.cyBottomHeight};
    }

    static inline constexpr MARGINS qmargins2margins(const QMargins &qmargins) {
        return {qmargins.left(), qmargins.right(), qmargins.top(), qmargins.bottom()};
    }

    static inline /*constexpr*/ QString hwnd2str(const WId windowId) {
        // NULL handle is allowed here.
        return QLatin1String("0x") +
               QString::number(windowId, 16).toUpper().rightJustified(8, u'0');
    }

    static inline /*constexpr*/ QString hwnd2str(HWND hwnd) {
        // NULL handle is allowed here.
        return hwnd2str(reinterpret_cast<WId>(hwnd));
    }

    static inline bool isDwmCompositionEnabled() {
        if (isWin8OrGreater()) {
            return true;
        }
        const DynamicApis &apis = DynamicApis::instance();
        if (!apis.pDwmIsCompositionEnabled) {
            return false;
        }
        BOOL enabled = FALSE;
        return SUCCEEDED(apis.pDwmIsCompositionEnabled(&enabled)) && enabled;
    }

    static inline bool isWindowFrameBorderColorized() {
        WindowsRegistryKey registry(HKEY_CURRENT_USER, LR"(Software\Microsoft\Windows\DWM)");
        if (!registry.isValid()) {
            return false;
        }
        auto value = registry.dwordValue(L"ColorPrevalence");
        if (!value.second) {
            return false;
        }
        return value.first;
    }

    static inline bool isHighContrastModeEnabled() {
        HIGHCONTRASTW hc{};
        hc.cbSize = sizeof(hc);
        ::SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, FALSE);
        return (hc.dwFlags & HCF_HIGHCONTRASTON);
    }

    static inline bool isDarkThemeActive() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
        WindowsRegistryKey registry(
            HKEY_CURRENT_USER, LR"(Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)");
        if (!registry.isValid()) {
            return false;
        }
        auto value = registry.dwordValue(L"AppsUseLightTheme");
        if (!value.second) {
            return false;
        }
        return !value.first;
#endif
    }

    static inline bool isDarkWindowFrameEnabled(HWND hwnd) {
        BOOL enabled = FALSE;
        const DynamicApis &apis = DynamicApis::instance();
        if (SUCCEEDED(apis.pDwmGetWindowAttribute(hwnd, _DWMWA_USE_IMMERSIVE_DARK_MODE, &enabled,
                                                  sizeof(enabled)))) {
            return enabled;
        } else if (SUCCEEDED(apis.pDwmGetWindowAttribute(hwnd,
                                                         _DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1,
                                                         &enabled, sizeof(enabled)))) {
            return enabled;
        }
        return false;
    }

    static inline QColor getAccentColor() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        return QGuiApplication::palette().color(QPalette::Accent);
#else
        WindowsRegistryKey registry(HKEY_CURRENT_USER, LR"(Software\Microsoft\Windows\DWM)");
        if (!registry.isValid()) {
            return {};
        }
        auto value = registry.dwordValue(L"AccentColor");
        if (!value.second) {
            return {};
        }
        // The retrieved value is in the #AABBGGRR format, we need to
        // convert it to the #AARRGGBB format which Qt expects.
        QColor color = QColor::fromRgba(value.first);
        if (!color.isValid()) {
            return {};
        }
        return QColor::fromRgb(color.blue(), color.green(), color.red(), color.alpha());
#endif
    }

    static inline quint32 getDpiForWindow(HWND hwnd) {
        const DynamicApis &apis = DynamicApis::instance();
        if (apis.pGetDpiForWindow) {         // Win10
            return apis.pGetDpiForWindow(hwnd);
        } else if (apis.pGetDpiForMonitor) { // Win8.1
            HMONITOR monitor = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            UINT dpiX{0};
            UINT dpiY{0};
            apis.pGetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
            return dpiX;
        } else { // Win2K
            HDC hdc = ::GetDC(nullptr);
            const int dpiX = ::GetDeviceCaps(hdc, LOGPIXELSX);
            // const int dpiY = ::GetDeviceCaps(hdc, LOGPIXELSY);
            ::ReleaseDC(nullptr, hdc);
            return quint32(dpiX);
        }
    }

    static inline quint32 getSystemMetricsForDpi(int index, quint32 dpi) {
        const DynamicApis &apis = DynamicApis::instance();
        if (apis.pGetSystemMetricsForDpi) {
            return apis.pGetSystemMetricsForDpi(index, dpi);
        }
        return ::GetSystemMetrics(index);
    }

    static inline quint32 getWindowFrameBorderThickness(HWND hwnd) {
        const DynamicApis &apis = DynamicApis::instance();
        if (UINT result = 0; SUCCEEDED(apis.pDwmGetWindowAttribute(
                hwnd, _DWMWA_VISIBLE_FRAME_BORDER_THICKNESS, &result, sizeof(result)))) {
            return result;
        }
        return getSystemMetricsForDpi(SM_CXBORDER, getDpiForWindow(hwnd));
    }

    static inline quint32 getResizeBorderThickness(HWND hwnd) {
        const quint32 dpi = getDpiForWindow(hwnd);
        return getSystemMetricsForDpi(SM_CXSIZEFRAME, dpi) +
               getSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
    }

    static inline quint32 getTitleBarHeight(HWND hwnd) {
        const quint32 dpi = getDpiForWindow(hwnd);
        return getSystemMetricsForDpi(SM_CYCAPTION, dpi) +
               getSystemMetricsForDpi(SM_CXSIZEFRAME, dpi) +
               getSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
    }

}

#endif // QWKWINDOWSEXTRA_P_H
