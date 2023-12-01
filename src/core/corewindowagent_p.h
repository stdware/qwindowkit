#ifndef COREWINDOWAGENTPRIVATE_H
#define COREWINDOWAGENTPRIVATE_H

#include <QWKCore/corewindowagent.h>
#include <QWKCore/private/abstractwindowcontext_p.h>

namespace QWK {

    class QWK_CORE_EXPORT CoreWindowAgentPrivate {
        Q_DECLARE_PUBLIC(CoreWindowAgent)
    public:
        CoreWindowAgentPrivate();
        virtual ~CoreWindowAgentPrivate();

        void init();

        CoreWindowAgent *q_ptr; // no need to initialize

        bool setup(QWindow *window, WindowItemDelegate *delegate);

        AbstractWindowContext *m_eventHandler;
    };

}

#endif // COREWINDOWAGENTPRIVATE_H