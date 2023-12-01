#include "win32windowcontext_p.h"

#include <windows.h>

namespace QWK {

    static LRESULT CALLBACK QWKWindowsWndProc(const HWND hWnd, const UINT uMsg, const WPARAM wParam,
                                              const LPARAM lParam) {
        // Implement
        return 0;
    }

    Win32WindowContext::Win32WindowContext(QWindow *window,
                                                     WindowItemDelegate *delegate)
        : AbstractWindowContext(window, delegate) {
        // Install windows hook
    }

    Win32WindowContext::~Win32WindowContext() {
    }

}