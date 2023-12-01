#include "corewindowagent.h"
#include "corewindowagent_p.h"

#ifdef Q_OS_WINDOWS
#  include "handler/win32windowcontext_p.h"
#else
#  include "handler/qtwindowcontext_p.h"
#endif

namespace QWK {

    CoreWindowAgentPrivate::CoreWindowAgentPrivate() {
    }

    CoreWindowAgentPrivate::~CoreWindowAgentPrivate() {
    }

    void CoreWindowAgentPrivate::init() {
    }

    void CoreWindowAgentPrivate::setup(QWindow *window, WindowItemDelegate *delegate) {
#ifdef Q_OS_WINDOWS
        m_eventHandler = new Win32WindowContext(window, delegate);
#else
        m_eventHandler = new QtWindowContext(window, delegate);
#endif
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
