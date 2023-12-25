#include "qwindowkit_windows.h"

namespace QWK {

    static RTL_OSVERSIONINFOW GetRealOSVersionImpl() {
        HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
        using RtlGetVersionPtr = NTSTATUS(WINAPI *)(PRTL_OSVERSIONINFOW);
        auto pRtlGetVersion =
            reinterpret_cast<RtlGetVersionPtr>(::GetProcAddress(hMod, "RtlGetVersion"));
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

}