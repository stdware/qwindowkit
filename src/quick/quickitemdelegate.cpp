#include "quickitemdelegate_p.h"

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

namespace QWK {

    QuickItemDelegate::QuickItemDelegate() : WindowItemDelegate() {
    }

    QuickItemDelegate::~QuickItemDelegate() = default;

    QWindow *QuickItemDelegate::window(QObject *obj) const {
        return static_cast<QQuickItem *>(obj)->window();
    }

    bool QuickItemDelegate::isEnabled(QObject *obj) const {
        return static_cast<QQuickItem *>(obj)->isEnabled();
    }

    bool QuickItemDelegate::isVisible(QObject *obj) const {
        return static_cast<QQuickItem *>(obj)->isVisible();
    }

    QRect QuickItemDelegate::mapGeometryToScene(const QObject *obj) const {
        auto item = static_cast<const QQuickItem *>(obj);
        const QPointF originPoint = item->mapToScene(QPointF(0.0, 0.0));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
        const QSizeF size = item->size();
#else
        const QSizeF size = {item->width(), item->height()};
#endif
        return QRectF(originPoint, size).toRect();
    }

    QWindow *QuickItemDelegate::hostWindow(QObject *host) const {
        return static_cast<QQuickWindow *>(host);
    }

    bool QuickItemDelegate::isHostSizeFixed(QObject *host) const {
        return false;
    }

}