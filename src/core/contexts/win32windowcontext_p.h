#ifndef WIN32WINDOWCONTEXT_P_H
#define WIN32WINDOWCONTEXT_P_H

#include <QWKCore/qwindowkit_windows.h>
#include <QWKCore/private/abstractwindowcontext_p.h>

namespace QWK {

    class QWK_CORE_EXPORT Win32WindowContext : public AbstractWindowContext {
        Q_OBJECT
    public:
        Win32WindowContext(QWindow *window, WindowItemDelegate *delegate);
        ~Win32WindowContext();

    public:
        bool setup() override;

    protected:
        WId windowId;
        WNDPROC qtWindowProc; // Original Qt window proc function

        static LRESULT windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    };

}

#endif // WIN32WINDOWCONTEXT_P_H
