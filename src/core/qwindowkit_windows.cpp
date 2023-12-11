#include "qwindowkit_windows.h"

namespace QWK {

    typedef NTSTATUS(WINAPI *RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

    RTL_OSVERSIONINFOW GetRealOSVersion() {
        HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
        if (hMod) {
            auto fxPtr = reinterpret_cast<RtlGetVersionPtr>(GetProcAddress(hMod, "RtlGetVersion"));
            if (fxPtr != nullptr) {
                RTL_OSVERSIONINFOW rovi = {0};
                rovi.dwOSVersionInfoSize = sizeof(rovi);
                if (0 == fxPtr(&rovi)) {
                    return rovi;
                }
            }
        }
        RTL_OSVERSIONINFOW rovi = {0};
        return rovi;
    }

}