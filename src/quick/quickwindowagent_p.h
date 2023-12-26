#ifndef QUICKWINDOWAGENTPRIVATE_H
#define QUICKWINDOWAGENTPRIVATE_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the QWindowKit API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <QWKCore/qwkconfig.h>
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

#if defined(Q_OS_WINDOWS) && QWINDOWKIT_CONFIG(ENABLE_WINDOWS_SYSTEM_BORDERS)
        void setupWindows10BorderWorkaround();
#endif
    };

}

#endif // QUICKWINDOWAGENTPRIVATE_H