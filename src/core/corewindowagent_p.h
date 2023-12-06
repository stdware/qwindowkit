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

        bool setup(const QObject *host, const WindowItemDelegate *delegate);

        AbstractWindowContext *context;

    private:
        Q_DISABLE_COPY_MOVE(CoreWindowAgentPrivate)
    };

}

#endif // COREWINDOWAGENTPRIVATE_H