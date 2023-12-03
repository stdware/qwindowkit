#ifndef WIN32WINDOWCONTEXT_P_H
#define WIN32WINDOWCONTEXT_P_H

#include <QWKCore/qwindowkit_windows.h>
#include <QWKCore/private/abstractwindowcontext_p.h>

namespace QWK {

    class QWK_CORE_EXPORT Win32WindowContext : public AbstractWindowContext {
        Q_OBJECT
    public:
        Win32WindowContext(QWindow *window, WindowItemDelegate *delegate);
        ~Win32WindowContext() override;

        enum WindowPart {
            Outside,
            ClientArea,
            ChromeButton,
            ResizeBorder,
            FixedBorder,
            TitleBar,
        };

    public:
        bool setup() override;

        bool windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *result);
        bool snapLayoutHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam,
                               LRESULT *result);

    protected:
        WId windowId = 0;

        // Store the last hit test result, it's helpful to handle WM_MOUSEMOVE and WM_NCMOUSELEAVE.
        WindowPart lastHitTestResult = WindowPart::Outside;

        // True if we blocked a WM_MOUSELEAVE when mouse moves on chrome button, false when a
        // WM_MOUSELEAVE comes or we manually call TrackMouseEvent().
        bool mouseLeaveBlocked = false;
    };

}

#endif // WIN32WINDOWCONTEXT_P_H
