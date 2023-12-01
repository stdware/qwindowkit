#ifndef QUICKWINDOWAGENT_H
#define QUICKWINDOWAGENT_H

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

#include <QWKCore/corewindowagent.h>
#include <QWKQuick/qwkquickglobal.h>

namespace QWK {

    class QuickWindowAgentPrivate;

    class QWK_QUICK_EXPORT QuickWindowAgent : public CoreWindowAgent {
        Q_OBJECT
        Q_DECLARE_PRIVATE(QuickWindowAgent)
    public:
        explicit QuickWindowAgent(QObject *parent = nullptr);
        ~QuickWindowAgent();

    public:
        bool setup(QQuickWindow *window);

        bool isHitTestVisible(QQuickItem *item) const;
        void setHitTestVisible(QQuickItem *item, bool visible);
        void setHitTestVisible(const QRect &rect, bool visible);

        QQuickItem *systemButton(SystemButton button) const;
        void setSystemButton(SystemButton button, QQuickItem *item);

        QQuickItem *titleBar() const;
        void setTitleBar(QQuickItem *item);

    Q_SIGNALS:
        void titleBarWidgetChanged(QQuickItem *item);
        void systemButtonChanged(SystemButton button, QQuickItem *item);

    protected:
        QuickWindowAgent(QuickWindowAgentPrivate &d, QObject *parent = nullptr);
    };

}

#endif // QUICKWINDOWAGENT_H