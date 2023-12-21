#include "styleagent.h"
#include "styleagent_p.h"

#include <QtCore/QVariant>

namespace QWK {

    StyleAgentPrivate::StyleAgentPrivate() {
    }

    StyleAgentPrivate::~StyleAgentPrivate() = default;

    void StyleAgentPrivate::init() {
    }

    void StyleAgentPrivate::notifyThemeChanged(StyleAgent::SystemTheme theme) {
        if (theme == systemTheme)
            return;
        systemTheme = theme;

        Q_Q(StyleAgent);
        Q_EMIT q->systemThemeChanged();
    }

    void StyleAgentPrivate::_q_windowDestroyed() {
        windowAttributes.remove(static_cast<QWindow *>(sender()));
    }

    StyleAgent::StyleAgent(QObject *parent) : StyleAgent(*new StyleAgentPrivate(), parent) {
        Q_D(StyleAgent);
        d->setupSystemThemeHook();
    }

    StyleAgent::~StyleAgent() {
        Q_D(StyleAgent);
        d->removeSystemThemeHook();
    }

    StyleAgent::SystemTheme StyleAgent::systemTheme() const {
        Q_D(const StyleAgent);
        return d->systemTheme;
    }

    QVariant StyleAgent::windowAttribute(QWindow *window, const QString &key) const {
        Q_D(const StyleAgent);
        return d->windowAttributes.value(window).value(key);
    }

    bool StyleAgent::setWindowAttribute(QWindow *window, const QString &key,
                                        const QVariant &attribute) {
        Q_D(StyleAgent);
        if (!window)
            return false;

        auto it = d->windowAttributes.find(window);
        if (it == d->windowAttributes.end()) {
            if (!attribute.isValid())
                return true;
            if (!d->updateWindowAttribute(window, key, attribute, {}))
                return false;
            connect(window, &QWindow::destroyed, d, &StyleAgentPrivate::_q_windowDestroyed);
            d->windowAttributes.insert(window, QVariantHash{
                                                   {key, attribute}
            });
        } else {
            auto &attributes = it.value();
            auto oldAttribute = attributes.value(key);
            if (oldAttribute == attribute)
                return true;
            if (!d->updateWindowAttribute(window, key, attribute, oldAttribute))
                return false;
            attributes.insert(key, attribute);
        }
        return true;
    }

    StyleAgent::StyleAgent(StyleAgentPrivate &d, QObject *parent) : QObject(parent), d_ptr(&d) {
        d.q_ptr = this;

        d.init();
    }

}
