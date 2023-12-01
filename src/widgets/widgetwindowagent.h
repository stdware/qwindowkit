#ifndef WIDGETWINDOWAGENT_H
#define WIDGETWINDOWAGENT_H

#include <QtWidgets/QWidget>

#include <QWKCore/corewindowagent.h>
#include <QWKWidgets/qwkwidgetsglobal.h>

namespace QWK {

    class WidgetWindowAgentPrivate;

    class QWK_WIDGETS_EXPORT WidgetWindowAgent : public CoreWindowAgent {
        Q_OBJECT
        Q_DECLARE_PRIVATE(WidgetWindowAgent)
    public:
        explicit WidgetWindowAgent(QObject *parent = nullptr);
        ~WidgetWindowAgent();

    public:
        void setup(QWidget *w);

        bool isHitTestVisible(QWidget *w) const;
        void setHitTestVisible(QWidget *w, bool visible);
        void setHitTestVisible(const QRect &rect, bool visible);

        QWidget *systemButton(SystemButton button) const;
        void setSystemButton(SystemButton button, QWidget *w);

        QWidget *titleBar() const;
        void setTitleBar(QWidget *w);

    Q_SIGNALS:
        void titleBarWidgetChanged(QWidget *w);
        void systemButtonChanged(SystemButton button, QWidget *w);

    protected:
        WidgetWindowAgent(WidgetWindowAgentPrivate &d, QObject *parent = nullptr);
    };

}

#endif // WIDGETWINDOWAGENT_H