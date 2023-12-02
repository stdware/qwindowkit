#ifndef WIDGETWINDOWAGENTPRIVATE_H
#define WIDGETWINDOWAGENTPRIVATE_H

#include <QWKCore/private/corewindowagent_p.h>
#include <QWKWidgets/widgetwindowagent.h>

namespace QWK {

    class WidgetWindowAgentPrivate : public CoreWindowAgentPrivate {
        Q_DECLARE_PUBLIC(WidgetWindowAgent)
    public:
        WidgetWindowAgentPrivate();
        ~WidgetWindowAgentPrivate();

        void init();

        // Host
        QWidget *host{};
    };

}

#endif // WIDGETWINDOWAGENTPRIVATE_H