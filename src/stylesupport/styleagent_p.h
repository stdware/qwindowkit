#ifndef STYLEAGENTPRIVATE_H
#define STYLEAGENTPRIVATE_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the QWindowKit API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <QWKStyleSupport/styleagent.h>
#include <QtCore/QHash>

namespace QWK {

    class StyleAgentPrivate : public QObject {
        Q_DECLARE_PUBLIC(StyleAgent)
    public:
        StyleAgentPrivate();
        ~StyleAgentPrivate() override;

        void init();

        StyleAgent *q_ptr;

        StyleAgent::SystemTheme systemTheme = StyleAgent::Dark;
        QHash<QWindow *, QVariantHash> windowAttributes;

        virtual void setupSystemThemeHook();
        virtual void removeSystemThemeHook();
        virtual bool updateWindowAttribute(QWindow *window, const QString &key,
                                           const QVariant &attribute, const QVariant &oldAttribute);

        void notifyThemeChanged(StyleAgent::SystemTheme theme);

    private:
        void _q_windowDestroyed();
    };

}

#endif // STYLEAGENTPRIVATE_H