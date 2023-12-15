#include "windowitemdelegate_p.h"

namespace QWK {

    WindowItemDelegate::WindowItemDelegate() = default;

    WindowItemDelegate::~WindowItemDelegate() = default;

    void WindowItemDelegate::resetQtGrabbedControl(QObject *host) const {
        Q_UNUSED(host);
    }

}