// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#ifndef QWINDOWKIT_WINDOWS_H
#define QWINDOWKIT_WINDOWS_H

#ifndef _USER32_
#  define _USER32_
#endif

#ifndef _DWMAPI_
#  define _DWMAPI_
#endif

#include <QtCore/qt_windows.h>
#include <QtCore/qglobal.h>

#include <cwchar>
#include <optional>

#include <QWKCore/qwkglobal.h>

#ifndef GET_X_LPARAM
#  define GET_X_LPARAM(lp) (static_cast<int>(static_cast<short>(LOWORD(lp))))
#endif

#ifndef GET_Y_LPARAM
#  define GET_Y_LPARAM(lp) (static_cast<int>(static_cast<short>(HIWORD(lp))))
#endif

#ifndef RECT_WIDTH
#  define RECT_WIDTH(rect) ((rect).right - (rect).left)
#endif

#ifndef RECT_HEIGHT
#  define RECT_HEIGHT(rect) ((rect).bottom - (rect).top)
#endif

#ifndef USER_DEFAULT_SCREEN_DPI
#  define USER_DEFAULT_SCREEN_DPI (96)
#endif

// Maybe undocumented Windows messages
// https://github.com/tinysec/public/blob/master/win32k/MessageTable.md
// https://ulib.sourceforge.io/doxy/a00239.html
#ifndef WM_UAHDESTROYWINDOW
#  define WM_UAHDESTROYWINDOW (0x0090)
#endif

#ifndef WM_UNREGISTER_WINDOW_SERVICES
#  define WM_UNREGISTER_WINDOW_SERVICES (0x0272)
#endif

#ifndef WM_NCUAHDRAWCAPTION
#  define WM_NCUAHDRAWCAPTION (0x00AE)
#endif

#ifndef WM_NCUAHDRAWFRAME
#  define WM_NCUAHDRAWFRAME (0x00AF)
#endif

using QWK_OSVERSIONINFOW = struct _QWK_OSVERSIONINFOW {
    DWORD dwOSVersionInfoSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    wchar_t szCSDVersion[128];
};

namespace QWK {

    namespace Private {

        QWK_CORE_EXPORT QWK_OSVERSIONINFOW GetRealOSVersion();

        // A missing, unreadable or non-DWORD value is distinct from a valid zero.
        QWK_CORE_EXPORT std::optional<DWORD> readRegistryDword(HKEY parent, const wchar_t *subKey,
                                                               const wchar_t *valueName);

        inline bool IsWindows1122H2OrGreater_Real() {
            QWK_OSVERSIONINFOW rovi = GetRealOSVersion();
            return (rovi.dwMajorVersion > 10) ||
                   (rovi.dwMajorVersion == 10 &&
                    (rovi.dwMinorVersion > 0 || rovi.dwBuildNumber >= 22621));
        }

        inline bool IsWindows11OrGreater_Real() {
            QWK_OSVERSIONINFOW rovi = GetRealOSVersion();
            return (rovi.dwMajorVersion > 10) ||
                   (rovi.dwMajorVersion == 10 &&
                    (rovi.dwMinorVersion > 0 || rovi.dwBuildNumber >= 22000));
        }

        inline bool IsWindows1020H1OrGreater_Real() {
            QWK_OSVERSIONINFOW rovi = GetRealOSVersion();
            return (rovi.dwMajorVersion > 10) ||
                   (rovi.dwMajorVersion == 10 &&
                    (rovi.dwMinorVersion > 0 || rovi.dwBuildNumber >= 19041));
        }

        inline bool IsWindows101903OrGreater_Real() {
            QWK_OSVERSIONINFOW rovi = GetRealOSVersion();
            return (rovi.dwMajorVersion > 10) ||
                   (rovi.dwMajorVersion == 10 &&
                    (rovi.dwMinorVersion > 0 || rovi.dwBuildNumber >= 18362));
        }

        inline bool IsWindows101809OrGreater_Real() {
            QWK_OSVERSIONINFOW rovi = GetRealOSVersion();
            return (rovi.dwMajorVersion > 10) ||
                   (rovi.dwMajorVersion == 10 &&
                    (rovi.dwMinorVersion > 0 || rovi.dwBuildNumber >= 17763));
        }

        inline bool IsWindows10OrGreater_Real() {
            QWK_OSVERSIONINFOW rovi = GetRealOSVersion();
            return rovi.dwMajorVersion >= 10;
        }

        inline bool IsWindows8Point1OrGreater_Real() {
            QWK_OSVERSIONINFOW rovi = GetRealOSVersion();
            return (rovi.dwMajorVersion > 6) ||
                   (rovi.dwMajorVersion == 6 && rovi.dwMinorVersion >= 3);
        }

        inline bool IsWindows8OrGreater_Real() {
            QWK_OSVERSIONINFOW rovi = GetRealOSVersion();
            return (rovi.dwMajorVersion > 6) ||
                   (rovi.dwMajorVersion == 6 && rovi.dwMinorVersion >= 2);
        }

    }

    //
    // Version Helpers
    //

    inline bool isWin8OrGreater() {
        static const bool result = Private::IsWindows8OrGreater_Real();
        return result;
    }

    inline bool isWin8Point1OrGreater() {
        static const bool result = Private::IsWindows8Point1OrGreater_Real();
        return result;
    }

    inline bool isWin10OrGreater() {
        static const bool result = Private::IsWindows10OrGreater_Real();
        return result;
    }

    inline bool isWin101809OrGreater() {
        static const bool result = Private::IsWindows101809OrGreater_Real();
        return result;
    }

    inline bool isWin101903OrGreater() {
        static const bool result = Private::IsWindows101903OrGreater_Real();
        return result;
    }

    inline bool isWin1020H1OrGreater() {
        static const bool result = Private::IsWindows1020H1OrGreater_Real();
        return result;
    }

    inline bool isWin11OrGreater() {
        static const bool result = Private::IsWindows11OrGreater_Real();
        return result;
    }

    inline bool isWin1122H2OrGreater() {
        static const bool result = Private::IsWindows1122H2OrGreater_Real();
        return result;
    }

    //
    // Native Event Helpers
    //

    inline bool isImmersiveColorSetChange(WPARAM wParam, LPARAM lParam) {
        return !wParam && lParam &&
               std::wcscmp(reinterpret_cast<LPCWSTR>(lParam), L"ImmersiveColorSet") == 0;
    }

}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
Q_DECLARE_METATYPE(QMargins)
#endif

#endif // QWINDOWKIT_WINDOWS_H
