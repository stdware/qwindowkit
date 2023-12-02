#ifndef WIDGETITEMDELEGATE_P_H
#define WIDGETITEMDELEGATE_P_H

#include <QtCore/QObject>
#include <QtGui/QWindow>

#include <QWKCore/windowitemdelegate.h>
#include <QWKWidgets/qwkwidgetsglobal.h>

namespace QWK {

    class QWK_WIDGETS_EXPORT WidgetItemDelegate : public WindowItemDelegate {
    public:
        WidgetItemDelegate();
        ~WidgetItemDelegate() override;

    public:
        QWindow *window(QObject *obj) const override;

        bool isEnabled(QObject *obj) const override;
        bool isVisible(QObject *obj) const override;
    };

}

#endif // WIDGETITEMDELEGATE_P_H
