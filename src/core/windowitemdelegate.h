#ifndef WINDOWITEMDELEGATE_H
#define WINDOWITEMDELEGATE_H

#include <QtCore/QObject>
#include <QtGui/QWindow>

#include <QWKCore/qwkcoreglobal.h>

namespace QWK {

    class WindowItemDelegate {
        Q_DISABLE_COPY(WindowItemDelegate)

    public:
        WindowItemDelegate() = default;
        virtual ~WindowItemDelegate() = default;

    public:
        virtual QWindow *window(QObject *obj) const = 0;

        virtual bool isEnabled(QObject *obj) const = 0;
        virtual bool isVisible(QObject *obj) const = 0;
    };

    using WindowItemDelegatePtr = std::shared_ptr<WindowItemDelegate>;

}

#endif // WINDOWITEMDELEGATE_H
