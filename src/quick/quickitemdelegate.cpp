#include "quickitemdelegate_p.h"

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

namespace QWK {

    QuickItemDelegate::QuickItemDelegate() : WindowItemDelegate() {
    }

    QuickItemDelegate::~QuickItemDelegate() = default;

    QWindow *QuickItemDelegate::window(QObject *obj) const {
        return qobject_cast<QQuickItem *>(obj)->window();
    }

    bool QuickItemDelegate::isEnabled(QObject *obj) const {
        return qobject_cast<QQuickItem *>(obj)->isEnabled();
    }

    bool QuickItemDelegate::isVisible(QObject *obj) const {
        return qobject_cast<QQuickItem *>(obj)->isVisible();
    }

}