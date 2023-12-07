#ifndef QUICKWINDOWCONTEXT_P_H
#define QUICKWINDOWCONTEXT_P_H

#include <QtGlobal>

#ifdef Q_OS_WINDOWS
#  include <QWKCore/private/win32windowcontext_p.h>
#else
#  include <QWKCore/private/qtwindowcontext_p.h>
#endif

namespace QWK {

    using CoreWindowContext =
#ifdef Q_OS_WINDOWS
        Win32WindowContext
#else
        QtWindowContext
#endif
        ;

    class QuickWindowContext : public CoreWindowContext {
        Q_OBJECT
    public:
        QuickWindowContext() = default;
        ~QuickWindowContext() override = default;

    protected:
        bool hostEventFilter(QEvent *event) override;
    };

}

#endif // QUICKWINDOWCONTEXT_P_H
