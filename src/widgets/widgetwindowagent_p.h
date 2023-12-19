#ifndef WIDGETWINDOWAGENTPRIVATE_H
#define WIDGETWINDOWAGENTPRIVATE_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the QWindowKit API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

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
#ifdef Q_OS_MAC
        QWidget *systemButtonAreaWidget{};
        std::unique_ptr<QObject> systemButtonAreaWidgetEventFilter;
#endif

#ifdef Q_OS_WINDOWS
        void setupWindows10BorderWorkaround();
#endif
    };

}

#endif // WIDGETWINDOWAGENTPRIVATE_H
