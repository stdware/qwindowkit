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

        std::unique_ptr<QObject> paintFilter;
    };

}

#endif // WIDGETWINDOWAGENTPRIVATE_H