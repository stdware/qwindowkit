#include "corewindowagent.h"
#include "corewindowagent_p.h"

#ifdef Q_OS_WINDOWS
#  include "win32windowcontext_p.h"
#else
#  include "qtwindowcontext_p.h"
#endif

Q_LOGGING_CATEGORY(qWindowKitLog, "qwindowkit")

namespace QWK {

    CoreWindowAgentPrivate::CoreWindowAgentPrivate() : m_eventHandler(nullptr) {
    }

    CoreWindowAgentPrivate::~CoreWindowAgentPrivate() {
        delete m_eventHandler;
    }

    void CoreWindowAgentPrivate::init() {
    }

    bool CoreWindowAgentPrivate::setup(QWindow *window, WindowItemDelegate *delegate) {
        auto handler =
#ifdef Q_OS_WINDOWS
            new Win32WindowContext(window, delegate)
#else
            new QtWindowContext(window, delegate)
#endif
            ;

        if (!handler->setup()) {
            delete handler;
            return false;
        }
        m_eventHandler = handler;
        return true;
    }

    CoreWindowAgent::~CoreWindowAgent() {
    }

    void CoreWindowAgent::showSystemMenu(const QPoint &pos) {
        Q_D(CoreWindowAgent);
        d->m_eventHandler->showSystemMenu(pos);
    }

    void CoreWindowAgent::startSystemMove(const QPoint &pos) {
        Q_D(CoreWindowAgent);
        auto win = d->m_eventHandler->window();
        if (!win) {
            return;
        }

        Q_UNUSED(pos)
        win->startSystemMove();
    }

    void CoreWindowAgent::startSystemResize(Qt::Edges edges, const QPoint &pos) {
        Q_D(CoreWindowAgent);
        auto win = d->m_eventHandler->window();
        if (!win) {
            return;
        }

        Q_UNUSED(pos)
        win->startSystemResize(edges);
    }

    void CoreWindowAgent::centralize() {
    }

    void CoreWindowAgent::raise() {
    }

    CoreWindowAgent::CoreWindowAgent(CoreWindowAgentPrivate &d, QObject *parent)
        : QObject(parent), d_ptr(&d) {
        d.q_ptr = this;

        d.init();
    }

}
