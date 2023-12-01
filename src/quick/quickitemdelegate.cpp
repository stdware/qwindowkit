#include "quickitemdelegate_p.h"

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

namespace QWK {

    QuickItemDelegate::QuickItemDelegate() {
    }

    QuickItemDelegate::~QuickItemDelegate() {
    }

    QWindow *QuickItemDelegate::window(QObject *obj) const {
        return static_cast<QQuickItem *>(obj)->window();
    }

    bool QuickItemDelegate::isEnabled(QObject *obj) const {
        return static_cast<QQuickItem *>(obj)->isEnabled();
    }

    bool QuickItemDelegate::isVisible(QObject *obj) const {
        return static_cast<QQuickItem *>(obj)->isVisible();
    }

}