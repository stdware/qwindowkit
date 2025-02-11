// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "qwindowkit_windows.h"

namespace QWK {

    static RTL_OSVERSIONINFOW GetRealOSVersionImpl() {
        HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
        Q_ASSERT(hMod);
        using RtlGetVersionPtr = NTSTATUS(WINAPI *)(PRTL_OSVERSIONINFOW);
        auto pRtlGetVersion =
            reinterpret_cast<RtlGetVersionPtr>(::GetProcAddress(hMod, "RtlGetVersion"));
        Q_ASSERT(pRtlGetVersion);
        RTL_OSVERSIONINFOW rovi{};
        rovi.dwOSVersionInfoSize = sizeof(rovi);
        pRtlGetVersion(&rovi);
        return rovi;
    }

    namespace Private {

        RTL_OSVERSIONINFOW GetRealOSVersion() {
            static const auto result = GetRealOSVersionImpl();
            return result;
        }

    }

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    WindowsRegistryKey::WindowsRegistryKey(HKEY parentHandle, QStringView subKey,
                                           REGSAM permissions, REGSAM access) {
        if (::RegOpenKeyExW(parentHandle, reinterpret_cast<const wchar_t *>(subKey.utf16()), 0,
                            permissions | access, &m_key) != ERROR_SUCCESS) {
            m_key = nullptr;
        }
    }

    WindowsRegistryKey::~WindowsRegistryKey() {
        close();
    }

    void WindowsRegistryKey::close() {
        if (isValid()) {
            ::RegCloseKey(m_key);
            m_key = nullptr;
        }
    }

    QString WindowsRegistryKey::stringValue(QStringView subKey) const {
        QString result;
        if (!isValid())
            return result;
        DWORD type;
        DWORD size;
        auto subKeyC = reinterpret_cast<const wchar_t *>(subKey.utf16());
        if (::RegQueryValueExW(m_key, subKeyC, nullptr, &type, nullptr, &size) != ERROR_SUCCESS ||
            (type != REG_SZ && type != REG_EXPAND_SZ) || size <= 2) {
            return result;
        }
        // Reserve more for rare cases where trailing '\0' are missing in registry.
        // Rely on 0-termination since strings of size 256 padded with 0 have been
        // observed (QTBUG-84455).
        size += 2;
        QVarLengthArray<unsigned char> buffer(static_cast<int>(size));
        std::fill(buffer.data(), buffer.data() + size, 0u);
        if (::RegQueryValueExW(m_key, subKeyC, nullptr, &type, buffer.data(), &size) ==
            ERROR_SUCCESS)
            result = QString::fromWCharArray(reinterpret_cast<const wchar_t *>(buffer.constData()));
        return result;
    }

    std::pair<DWORD, bool> WindowsRegistryKey::dwordValue(QStringView subKey) const {
        if (!isValid())
            return std::make_pair(0, false);
        DWORD type;
        auto subKeyC = reinterpret_cast<const wchar_t *>(subKey.utf16());
        if (::RegQueryValueExW(m_key, subKeyC, nullptr, &type, nullptr, nullptr) != ERROR_SUCCESS ||
            type != REG_DWORD) {
            return std::make_pair(0, false);
        }
        DWORD value = 0;
        DWORD size = sizeof(value);
        const bool ok =
            ::RegQueryValueExW(m_key, subKeyC, nullptr, nullptr,
                               reinterpret_cast<unsigned char *>(&value), &size) == ERROR_SUCCESS;
        return std::make_pair(value, ok);
    }
#endif
}