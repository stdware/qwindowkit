#include "styleagent.h"
#include "styleagent_p.h"

#include <QtCore/QVariant>

namespace QWK {

    StyleAgentPrivate::StyleAgentPrivate() {
    }

    StyleAgentPrivate::~StyleAgentPrivate() {
        removeSystemThemeHook();
    }

    void StyleAgentPrivate::init() {
        setupSystemThemeHook();
    }

    void StyleAgentPrivate::notifyThemeChanged(StyleAgent::SystemTheme theme) {
        if (theme == systemTheme)
            return;
        systemTheme = theme;

        Q_Q(StyleAgent);
        Q_EMIT q->systemThemeChanged();
    }

    StyleAgent::StyleAgent(QObject *parent) : StyleAgent(*new StyleAgentPrivate(), parent) {
    }

    StyleAgent::~StyleAgent() {
    }

    StyleAgent::SystemTheme StyleAgent::systemTheme() const {
        Q_D(const StyleAgent);
        return d->systemTheme;
    }

    StyleAgent::StyleAgent(StyleAgentPrivate &d, QObject *parent) : QObject(parent), d_ptr(&d) {
        d.q_ptr = this;

        d.init();
    }

}
