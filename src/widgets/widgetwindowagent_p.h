#ifndef WIDGETWINDOWAGENTPRIVATE_H
#define WIDGETWINDOWAGENTPRIVATE_H

#include <QWKCore/private/windowagentbase_p.h>
#include <QWKWidgets/widgetwindowagent.h>

namespace QWK {

    class WidgetWindowAgentPrivate : public WindowAgentBasePrivate {
        Q_DECLARE_PUBLIC(WidgetWindowAgent)
    public:
        WidgetWindowAgentPrivate();
        ~WidgetWindowAgentPrivate();

        void init();

        // Host
        QWidget *hostWidget{};

#ifdef Q_OS_WINDOWS
        void setupWindows10BorderWorkaround();
#endif
    };

}

#endif // WIDGETWINDOWAGENTPRIVATE_H