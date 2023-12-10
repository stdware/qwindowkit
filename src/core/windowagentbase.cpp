#include "windowagentbase.h"
#include "windowagentbase_p.h"

#include "qwkglobal_p.h"

#ifdef Q_OS_WINDOWS
#  include "win32windowcontext_p.h"
#else
#  include "qtwindowcontext_p.h"
#endif

Q_LOGGING_CATEGORY(qWindowKitLog, "qwindowkit")

namespace QWK {

    WindowAgentBasePrivate::WindowAgentBasePrivate() : q_ptr(nullptr), context(nullptr) {
    }

    WindowAgentBasePrivate::~WindowAgentBasePrivate() = default;

    void WindowAgentBasePrivate::init() {
    }

    AbstractWindowContext *WindowAgentBasePrivate::createContext() const {
        return
#ifdef Q_OS_WINDOWS
            new Win32WindowContext()
#else
            new QtWindowContext()
#endif
                ;
    }


    bool WindowAgentBasePrivate::setup(QObject *host, WindowItemDelegate *delegate) {
        std::unique_ptr<AbstractWindowContext> ctx(createContext());
        if (!ctx->setup(host, delegate)) {
            return false;
        }
        context = std::move(ctx);
        return true;
    }

    WindowAgentBase::~WindowAgentBase() = default;

    void WindowAgentBase::showSystemMenu(const QPoint &pos) {
        Q_D(WindowAgentBase);
        d->context->showSystemMenu(pos);
    }

    void WindowAgentBase::startSystemMove(const QPoint &pos) {
        Q_D(WindowAgentBase);
        auto win = d->context->window();
        if (!win) {
            return;
        }

        Q_UNUSED(pos)
        win->startSystemMove();
    }

    void WindowAgentBase::startSystemResize(Qt::Edges edges, const QPoint &pos) {
        Q_D(WindowAgentBase);
        auto win = d->context->window();
        if (!win) {
            return;
        }

        Q_UNUSED(pos)
        win->startSystemResize(edges);
    }

    void WindowAgentBase::centralize() {
    }

    void WindowAgentBase::raise() {
    }

    WindowAgentBase::WindowAgentBase(WindowAgentBasePrivate &d, QObject *parent)
        : QObject(parent), d_ptr(&d) {
        d.q_ptr = this;

        d.init();
    }

}
