#include "windowitemdelegate.h"

namespace QWK {

    WindowItemDelegate::WindowItemDelegate() = default;

    WindowItemDelegate::~WindowItemDelegate() = default;

    bool WindowItemDelegate::resetQtGrabbedControl() const {
        return true;
    }

}