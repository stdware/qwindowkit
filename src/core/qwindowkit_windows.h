// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
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

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#  include <QtCore/private/qwinregistry_p.h>
#endif

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

namespace QWK {

    namespace Private {

        QWK_CORE_EXPORT RTL_OSVERSIONINFOW GetRealOSVersion();

        inline bool IsWindows1122H2OrGreater_Real() {
            RTL_OSVERSIONINFOW rovi = GetRealOSVersion();
            return (rovi.dwMajorVersion > 10) ||
                   (rovi.dwMajorVersion == 10 &&
                    (rovi.dwMinorVersion > 0 || rovi.dwBuildNumber >= 22621));
        }

        inline bool IsWindows11OrGreater_Real() {
            RTL_OSVERSIONINFOW rovi = GetRealOSVersion();
            return (rovi.dwMajorVersion > 10) ||
                   (rovi.dwMajorVersion == 10 &&
                    (rovi.dwMinorVersion > 0 || rovi.dwBuildNumber >= 22000));
        }

        inline bool IsWindows1020H1OrGreater_Real() {
            RTL_OSVERSIONINFOW rovi = GetRealOSVersion();
            return (rovi.dwMajorVersion > 10) ||
                   (rovi.dwMajorVersion == 10 &&
                    (rovi.dwMinorVersion > 0 || rovi.dwBuildNumber >= 19041));
        }

        inline bool IsWindows102004OrGreater_Real() {
            return IsWindows1020H1OrGreater_Real();
        }

        inline bool IsWindows101903OrGreater_Real() {
            RTL_OSVERSIONINFOW rovi = GetRealOSVersion();
            return (rovi.dwMajorVersion > 10) ||
                   (rovi.dwMajorVersion == 10 &&
                    (rovi.dwMinorVersion > 0 || rovi.dwBuildNumber >= 18362));
        }

        inline bool IsWindows1019H1OrGreater_Real() {
            return IsWindows101903OrGreater_Real();
        }

        inline bool IsWindows101809OrGreater_Real() {
            RTL_OSVERSIONINFOW rovi = GetRealOSVersion();
            return (rovi.dwMajorVersion > 10) ||
                   (rovi.dwMajorVersion == 10 &&
                    (rovi.dwMinorVersion > 0 || rovi.dwBuildNumber >= 17763));
        }

        inline bool IsWindows10RS5OrGreater_Real() {
            return IsWindows101809OrGreater_Real();
        }

        inline bool IsWindows10OrGreater_Real() {
            RTL_OSVERSIONINFOW rovi = GetRealOSVersion();
            return rovi.dwMajorVersion >= 10;
        }

        inline bool IsWindows8Point1OrGreater_Real() {
            RTL_OSVERSIONINFOW rovi = GetRealOSVersion();
            return (rovi.dwMajorVersion > 6) ||
                   (rovi.dwMajorVersion == 6 && rovi.dwMinorVersion >= 3);
        }

        inline bool IsWindows8OrGreater_Real() {
            RTL_OSVERSIONINFOW rovi = GetRealOSVersion();
            return (rovi.dwMajorVersion > 6) ||
                   (rovi.dwMajorVersion == 6 && rovi.dwMinorVersion >= 2);
        }

        inline bool IsWindows10Only_Real() {
            return IsWindows10OrGreater_Real() && !IsWindows11OrGreater_Real();
        }

    }

    //
    // Registry Helpers
    //

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    class QWK_CORE_EXPORT WindowsRegistryKey {
    public:
        WindowsRegistryKey(HKEY parentHandle, QStringView subKey, REGSAM permissions = KEY_READ,
                           REGSAM access = 0);

        ~WindowsRegistryKey();

        bool isValid() const;

        void close();
        QString stringValue(QStringView subKey) const;
        std::pair<DWORD, bool> dwordValue(QStringView subKey) const;

    private:
        HKEY m_key;

        Q_DISABLE_COPY(WindowsRegistryKey)
    };

    inline bool WindowsRegistryKey::isValid() const {
        return m_key != nullptr;
    }
#elif QT_VERSION < QT_VERSION_CHECK(6, 8, 1)
    using WindowsRegistryKey = QWinRegistryKey;
#else
    class WindowsRegistryKey : public QWinRegistryKey {
    public:
        WindowsRegistryKey(HKEY parentHandle, QStringView subKey, REGSAM permissions = KEY_READ,
                           REGSAM access = 0)
            : QWinRegistryKey(parentHandle, subKey, permissions, access) {
        }

        inline std::pair<DWORD, bool> dwordValue(QStringView subKey) const;
    };

    inline std::pair<DWORD, bool> WindowsRegistryKey::dwordValue(QStringView subKey) const {
        const auto val = value<DWORD>(subKey);
        if (!val) {
            return {0, false};
        }
        return {val.value(), true};
    }
#endif

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

    inline bool isWin10RS5OrGreater() {
        return isWin101809OrGreater();
    }

    inline bool isWin101903OrGreater() {
        static const bool result = Private::IsWindows101903OrGreater_Real();
        return result;
    }

    inline bool isWin1019H1OrGreater() {
        return isWin101903OrGreater();
    }

    inline bool isWin1020H1OrGreater() {
        static const bool result = Private::IsWindows1020H1OrGreater_Real();
        return result;
    }

    inline bool isWin102004OrGreater() {
        return isWin1020H1OrGreater();
    }

    inline bool isWin11OrGreater() {
        static const bool result = Private::IsWindows11OrGreater_Real();
        return result;
    }

    inline bool isWin1122H2OrGreater() {
        static const bool result = Private::IsWindows1122H2OrGreater_Real();
        return result;
    }

    inline bool isWin10Only() {
        static const bool result = Private::IsWindows10Only_Real();
        return result;
    };

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
