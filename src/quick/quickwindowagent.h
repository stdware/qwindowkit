#ifndef QUICKWINDOWAGENT_H
#define QUICKWINDOWAGENT_H

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

#include <QWKCore/windowagentbase.h>
#include <QWKQuick/qwkquickglobal.h>

namespace QWK {

    class QuickWindowAgentPrivate;

    class QWK_QUICK_EXPORT QuickWindowAgent : public WindowAgentBase {
        Q_OBJECT
        Q_DECLARE_PRIVATE(QuickWindowAgent)
    public:
        explicit QuickWindowAgent(QObject *parent = nullptr);
        ~QuickWindowAgent() override;

    public:
        bool setup(QQuickWindow *window);

        const QQuickItem *titleBar() const;
        void setTitleBar(const QQuickItem *item);

        const QQuickItem *systemButton(SystemButton button) const;
        void setSystemButton(SystemButton button, const QQuickItem *item);

        bool isHitTestVisible(const QQuickItem *item) const;
        void setHitTestVisible(const QQuickItem *item, bool visible = true);
        void setHitTestVisible(const QRect &rect, bool visible = true);

    Q_SIGNALS:
        void titleBarWidgetChanged(const QQuickItem *item);
        void systemButtonChanged(SystemButton button, const QQuickItem *item);

    protected:
        QuickWindowAgent(QuickWindowAgentPrivate &d, QObject *parent = nullptr);
    };

}

#endif // QUICKWINDOWAGENT_H