#ifndef WINDOWITEMDELEGATE_P_H
#define WINDOWITEMDELEGATE_P_H

#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtGui/QWindow>

#include <QWKCore/qwkcoreglobal.h>

namespace QWK {

    class QWK_CORE_EXPORT WindowItemDelegate {
    public:
        WindowItemDelegate();
        virtual ~WindowItemDelegate();

    public:
        // Item property query
        virtual QWindow *window(QObject *obj) const = 0;
        virtual bool isEnabled(const QObject *obj) const = 0;
        virtual bool isVisible(const QObject *obj) const = 0;
        virtual QRect mapGeometryToScene(const QObject *obj) const = 0;

        // Host property query
        virtual QWindow *hostWindow(QObject *host) const = 0;
        virtual bool isHostSizeFixed(const QObject *host) const = 0;
        virtual bool isWindowActive(const QObject *host) const = 0;

        // Callbacks
        virtual void resetQtGrabbedControl() const;

    private:
        Q_DISABLE_COPY(WindowItemDelegate)
    };

}

#endif // WINDOWITEMDELEGATE_P_H
