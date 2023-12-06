#include "corewindowagent.h"
#include "corewindowagent_p.h"

#include "qwkcoreglobal_p.h"

#ifdef Q_OS_WINDOWS
#  include "win32windowcontext_p.h"
#else
#  include "qtwindowcontext_p.h"
#endif

Q_LOGGING_CATEGORY(qWindowKitLog, "qwindowkit")

namespace QWK {

    CoreWindowAgentPrivate::CoreWindowAgentPrivate() : q_ptr(nullptr), context(nullptr) {
    }

    CoreWindowAgentPrivate::~CoreWindowAgentPrivate() = default;

    void CoreWindowAgentPrivate::init() {
    }

    bool CoreWindowAgentPrivate::setup(QObject *host, WindowItemDelegate *delegate) {
        auto ctx =
#ifdef Q_OS_WINDOWS
            std::make_unique<Win32WindowContext>(host, delegate)
#else
            std::make_unique<QtWindowContext>(host, window, delegate)
#endif
            ;
        if (!ctx->setup()) {
            return false;
        }
        context = std::move(ctx);
        return true;
    }

    CoreWindowAgent::~CoreWindowAgent() = default;

    void CoreWindowAgent::showSystemMenu(const QPoint &pos) {
        Q_D(CoreWindowAgent);
        d->context->showSystemMenu(pos);
    }

    void CoreWindowAgent::startSystemMove(const QPoint &pos) {
        Q_D(CoreWindowAgent);
        auto win = d->context->window();
        if (!win) {
            return;
        }

        Q_UNUSED(pos)
        win->startSystemMove();
    }

    void CoreWindowAgent::startSystemResize(Qt::Edges edges, const QPoint &pos) {
        Q_D(CoreWindowAgent);
        auto win = d->context->window();
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
