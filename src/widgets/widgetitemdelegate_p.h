#ifndef WIDGETITEMDELEGATE_P_H
#define WIDGETITEMDELEGATE_P_H

#include <QtCore/QObject>
#include <QtGui/QWindow>

#include <QWKCore/private/windowitemdelegate_p.h>
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

        QWindow *hostWindow(const QObject *host) const override;
        bool isHostSizeFixed(const QObject *host) const override;
        bool isWindowActive(const QObject *host) const override;
        Qt::WindowStates getWindowState(const QObject *host) const override;
        Qt::WindowFlags getWindowFlags(const QObject *host) const override;

        void resetQtGrabbedControl(QObject *host) const override;
        void setWindowState(QObject *host, Qt::WindowStates state) const override;
        void setCursorShape(QObject *host, Qt::CursorShape shape) const override;
        void restoreCursorShape(QObject *host) const override;
        void setWindowFlags(QObject *host, Qt::WindowFlags flags) const override;
    };

}

#endif // WIDGETITEMDELEGATE_P_H
