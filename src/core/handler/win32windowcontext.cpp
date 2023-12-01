#include "win32windowcontext_p.h"

#include <windows.h>

namespace QWK {

    static LRESULT CALLBACK QWK_WindowsWndProc(const HWND hWnd, const UINT uMsg,
                                               const WPARAM wParam, const LPARAM lParam) {
        // Implement
        return 0;
    }

    static bool hookWindowProc(QObject *window, WId windowId) {
        Q_ASSERT(windowId);
        if (!windowId) {
            return false;
        }

        const auto hwnd = reinterpret_cast<HWND>(windowId);
        if (!extraData->qtWindowProc) {
            ::SetLastError(ERROR_SUCCESS);
            const auto qtWindowProc =
                reinterpret_cast<WNDPROC>(::GetWindowLongPtrW(hwnd, GWLP_WNDPROC));
            Q_ASSERT(qtWindowProc);
            if (!qtWindowProc) {
                WARNING << getSystemErrorMessage(kGetWindowLongPtrW);
                return false;
            }
            extraData->qtWindowProc = qtWindowProc;
        }
        if (!extraData->windowProcHooked) {
            ::SetLastError(ERROR_SUCCESS);
            if (::SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                    reinterpret_cast<LONG_PTR>(QWK_WindowsWndProc)) == 0) {
                WARNING << getSystemErrorMessage(kSetWindowLongPtrW);
                return false;
            }
            extraData->windowProcHooked = true;
        }
        return true;
    }

    Win32WindowContext::Win32WindowContext(QWindow *window, WindowItemDelegate *delegate)
        : AbstractWindowContext(window, delegate) {
        // Install windows window hook
    }

    Win32WindowContext::~Win32WindowContext() {
    }

}