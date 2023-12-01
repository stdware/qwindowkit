#ifndef WIN32WINDOWCONTEXT_P_H
#define WIN32WINDOWCONTEXT_P_H

#include <QWKCore/private/abstractwindowcontext_p.h>

namespace QWK {

    class QWK_CORE_EXPORT Win32WindowContext : public AbstractWindowContext {
        Q_OBJECT
    public:
        Win32WindowContext(QWindow *window, WindowItemDelegate *delegate);
        ~Win32WindowContext();
    };

}

#endif // WIN32WINDOWCONTEXT_P_H
