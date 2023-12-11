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
        Q_INVOKABLE bool setup(QQuickWindow *window);

        Q_INVOKABLE const QQuickItem *titleBar() const;
        Q_INVOKABLE void setTitleBar(const QQuickItem *item);

        Q_INVOKABLE const QQuickItem *systemButton(SystemButton button) const;
        Q_INVOKABLE void setSystemButton(SystemButton button, const QQuickItem *item);

        Q_INVOKABLE bool isHitTestVisible(const QQuickItem *item) const;
        Q_INVOKABLE inline void setHitTestVisible(const QQuickItem *item, bool visible = true) {
            setHitTestVisible_item(item, visible);
        }
        Q_INVOKABLE void setHitTestVisible_item(const QQuickItem *item, bool visible = true);
        Q_INVOKABLE void setHitTestVisible_rect(const QRect &rect, bool visible = true);

    Q_SIGNALS:
        void titleBarWidgetChanged(const QQuickItem *item);
        void systemButtonChanged(SystemButton button, const QQuickItem *item);

    protected:
        QuickWindowAgent(QuickWindowAgentPrivate &d, QObject *parent = nullptr);
    };

}

#endif // QUICKWINDOWAGENT_H