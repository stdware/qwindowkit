// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "qwindowkit_windows.h"

using QWK_NTSTATUS = long;

namespace QWK {

    static QWK_OSVERSIONINFOW GetRealOSVersionImpl() {
        HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
        Q_ASSERT(hMod);
        using RtlGetVersionPtr = QWK_NTSTATUS(WINAPI *)(QWK_OSVERSIONINFOW*);
        auto pRtlGetVersion =
            reinterpret_cast<RtlGetVersionPtr>(::GetProcAddress(hMod, "RtlGetVersion"));
        Q_ASSERT(pRtlGetVersion);
        QWK_OSVERSIONINFOW rovi{};
        rovi.dwOSVersionInfoSize = sizeof(rovi);
        pRtlGetVersion(&rovi);
        return rovi;
    }

    namespace Private {

        std::optional<DWORD> readRegistryDword(HKEY parent, const wchar_t *subKey,
                                               const wchar_t *valueName) {
            HKEY key = nullptr;
            if (::RegOpenKeyExW(parent, subKey, 0, KEY_READ, &key) != ERROR_SUCCESS)
                return std::nullopt;

            DWORD value = 0;
            DWORD type = 0;
            DWORD size = sizeof(value);
            // Read type and data together: a value may change between separate queries.
            const auto status = ::RegQueryValueExW(key, valueName, nullptr, &type,
                                                   reinterpret_cast<BYTE *>(&value), &size);
            ::RegCloseKey(key);
            if (status != ERROR_SUCCESS || type != REG_DWORD || size != sizeof(value))
                return std::nullopt;
            return value;
        }

        QWK_OSVERSIONINFOW GetRealOSVersion() {
            static const auto result = GetRealOSVersionImpl();
            return result;
        }

    }
}
