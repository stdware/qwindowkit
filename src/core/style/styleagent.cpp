#include "styleagent.h"
#include "styleagent_p.h"

#include <QtCore/QVariant>

namespace QWK {

    /*!
        \class StyleAgent
        \brief StyleAgent provides some features related to system theme.
    */

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

    /*!
        Constructor. Since it is not related to a concrete window instance, it is better to be used
        as a singleton.
    */
    StyleAgent::StyleAgent(QObject *parent) : StyleAgent(*new StyleAgentPrivate(), parent) {
    }

    /*!
        Destructor.
    */
    StyleAgent::~StyleAgent() {
    }

    /*!
        Returns the system theme.
    */
    StyleAgent::SystemTheme StyleAgent::systemTheme() const {
        Q_D(const StyleAgent);
        return d->systemTheme;
    }

    /*!
        \internal
    */
    StyleAgent::StyleAgent(StyleAgentPrivate &d, QObject *parent) : QObject(parent), d_ptr(&d) {
        d.q_ptr = this;

        d.init();
    }

    /*!
        \fn void StyleAgent::systemThemeChanged()

        This signal is emitted when the system theme changes.
    */

}
