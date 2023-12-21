#include "styleagent_p.h"

#include <QtCore/QVariant>

namespace QWK {

    void StyleAgentPrivate::setupSystemThemeHook() {
    }

    bool StyleAgentPrivate::updateWindowAttribute(QWindow *window, const QString &key,
                                                  const QVariant &attribute,
                                                  const QVariant &oldAttribute) {
        if (key == QStringLiteral("no-system-buttons")) {
            if (attribute.toBool()) {
                // TODO: set off
            } else {
                // TODO: set on
            }
            return true;
        }
        return false;
    }

}