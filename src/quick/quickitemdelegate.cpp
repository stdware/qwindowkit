#include "quickitemdelegate_p.h"

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

namespace QWK {

    QuickItemDelegate::QuickItemDelegate() : WindowItemDelegate() {
    }

    QuickItemDelegate::~QuickItemDelegate() = default;

    QWindow *QuickItemDelegate::window(const QObject *obj) const {
        return static_cast<const QQuickItem *>(obj)->window();
    }

    bool QuickItemDelegate::isEnabled(const QObject *obj) const {
        return static_cast<const QQuickItem *>(obj)->isEnabled();
    }

    bool QuickItemDelegate::isVisible(const QObject *obj) const {
        return static_cast<const QQuickItem *>(obj)->isVisible();
    }

    QRect QuickItemDelegate::mapGeometryToScene(const QObject *obj) const {
        auto item = static_cast<const QQuickItem *>(obj);
        const QPointF originPoint = item->mapToScene(QPointF(0.0, 0.0));
        const QSizeF size = item->size();
        return QRectF(originPoint, size).toRect();
    }

    QWindow *QuickItemDelegate::hostWindow(const QObject *host) const {
        return static_cast<QQuickWindow *>(const_cast<QObject *>(host));
    }

    bool QuickItemDelegate::isHostSizeFixed(const QObject *host) const {
        // ### TOOD
        return false;
    }

    bool QuickItemDelegate::isWindowActive(const QObject *host) const {
        return static_cast<const QQuickWindow *>(host)->isActive();
    }

}