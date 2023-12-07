#ifndef WIDGETWINDOWCONTEXT_P_H
#define WIDGETWINDOWCONTEXT_P_H

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

    class WidgetWindowContext : public CoreWindowContext {
        Q_OBJECT
    public:
        WidgetWindowContext() = default;
        ~WidgetWindowContext() override = default;

    protected:
        bool hostEventFilter(QEvent *event) override;
    };

}

#endif // WIDGETWINDOWCONTEXT_P_H
