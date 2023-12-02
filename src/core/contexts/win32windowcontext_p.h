#ifndef WIN32WINDOWCONTEXT_P_H
#define WIN32WINDOWCONTEXT_P_H

#include <QWKCore/qwindowkit_windows.h>
#include <QWKCore/private/abstractwindowcontext_p.h>

namespace QWK {

    class QWK_CORE_EXPORT Win32WindowContext : public AbstractWindowContext {
        Q_OBJECT
        Q_DISABLE_COPY(Win32WindowContext)

    public:
        Win32WindowContext(QWindow *window, WindowItemDelegatePtr delegate);
        ~Win32WindowContext() override;

    public:
        bool setup() override;

        bool windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *result);

    protected:
        WId windowId;
    };

}

#endif // WIN32WINDOWCONTEXT_P_H
