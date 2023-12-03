#ifndef WINDOWITEMDELEGATE_H
#define WINDOWITEMDELEGATE_H

#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtGui/QWindow>

#include <QWKCore/corewindowagent.h>

namespace QWK {

    class QWK_CORE_EXPORT WindowItemDelegate {
    public:
        WindowItemDelegate();
        virtual ~WindowItemDelegate();

    public:
        virtual QWindow *window(QObject *obj) const = 0;

        // Property query
        virtual bool isEnabled(QObject *obj) const = 0;
        virtual bool isVisible(QObject *obj) const = 0;
        virtual QRect mapGeometryToScene(const QObject *obj) const = 0;

        // Callbacks
        virtual bool resetQtGrabbedControl() const;

    private:
        Q_DISABLE_COPY_MOVE(WindowItemDelegate)
    };

}

#endif // WINDOWITEMDELEGATE_H
