#ifndef WIDGETWINDOWAGENT_H
#define WIDGETWINDOWAGENT_H

#include <QtWidgets/QWidget>

#include <QWKCore/windowagentbase.h>
#include <QWKWidgets/qwkwidgetsglobal.h>

namespace QWK {

    class WidgetWindowAgentPrivate;

    class QWK_WIDGETS_EXPORT WidgetWindowAgent : public WindowAgentBase {
        Q_OBJECT
        Q_DECLARE_PRIVATE(WidgetWindowAgent)
    public:
        explicit WidgetWindowAgent(QObject *parent = nullptr);
        ~WidgetWindowAgent() override;

    public:
        bool setup(QWidget *w);

        const QWidget *titleBar() const;
        void setTitleBar(const QWidget *w);

        const QWidget *systemButton(SystemButton button) const;
        void setSystemButton(SystemButton button, const QWidget *w);

        bool isHitTestVisible(const QWidget *w) const;
        void setHitTestVisible(const QWidget *w, bool visible = true);
        void setHitTestVisible(const QRect &rect, bool visible = true);

    Q_SIGNALS:
        void titleBarWidgetChanged(const QWidget *w);
        void systemButtonChanged(SystemButton button, const QWidget *w);

    protected:
        WidgetWindowAgent(WidgetWindowAgentPrivate &d, QObject *parent = nullptr);
    };

}

#endif // WIDGETWINDOWAGENT_H