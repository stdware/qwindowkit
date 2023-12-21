#ifndef STYLEAGENTPRIVATE_H
#define STYLEAGENTPRIVATE_H

#include <QWKStyleSupport/styleagent.h>

namespace QWK {

    class StyleAgentPrivate : public QObject {
        Q_DECLARE_PUBLIC(StyleAgent)
    public:
        StyleAgentPrivate();
        ~StyleAgentPrivate() override;

        void init();

        StyleAgent *q_ptr;

        QHash<QWindow *, QVariantHash> windowAttributes;

        virtual void setupSystemThemeHook();
        virtual bool updateWindowAttribute(QWindow *window, const QString &key,
                                           const QVariant &attribute, const QVariant &oldAttribute);

    private:
        void _q_windowDestroyed();
    };

}

#endif // STYLEAGENTPRIVATE_H