#ifndef QUICKWINDOWAGENTPRIVATE_H
#define QUICKWINDOWAGENTPRIVATE_H

#include <QWKCore/private/windowagentbase_p.h>
#include <QWKQuick/quickwindowagent.h>

namespace QWK {

    class QuickWindowAgentPrivate : public WindowAgentBasePrivate {
        Q_DECLARE_PUBLIC(QuickWindowAgent)
    public:
        QuickWindowAgentPrivate();
        ~QuickWindowAgentPrivate() override;

        void init();

        // Host
        QQuickWindow *hostWindow{};
    };

}

#endif // QUICKWINDOWAGENTPRIVATE_H