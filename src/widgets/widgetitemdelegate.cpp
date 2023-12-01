#include "widgetitemdelegate_p.h"

#include <QtWidgets/QWidget>

namespace QWK {
    
    WidgetItemDelegate::WidgetItemDelegate() {
    }

    WidgetItemDelegate::~WidgetItemDelegate() {
    }

    QWindow *WidgetItemDelegate::window(QObject *obj) const {
        return static_cast<QWidget *>(obj)->windowHandle();
    }

    bool WidgetItemDelegate::isEnabled(QObject *obj) const {
        return static_cast<QWidget *>(obj)->isEnabled();
    }

    bool WidgetItemDelegate::isVisible(QObject *obj) const {
        return static_cast<QWidget *>(obj)->isVisible();
    }

}