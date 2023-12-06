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
        QWindow *window(const QObject *obj) const override;
        bool isEnabled(const QObject *obj) const override;
        bool isVisible(const QObject *obj) const override;
        QRect mapGeometryToScene(const QObject *obj) const override;

        QWindow * hostWindow(const QObject *host) const override;
        bool isHostSizeFixed(const QObject *host) const override;

        bool resetQtGrabbedControl() const override;
    };

}

#endif // WIDGETITEMDELEGATE_P_H
