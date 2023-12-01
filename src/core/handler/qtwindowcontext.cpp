#include "qtwindowcontext_p.h"

namespace QWK {

    QtWindowContext::QtWindowContext(QWindow *window, WindowItemDelegate *delegate)
        : AbstractWindowContext(window, delegate) {
    }

    QtWindowContext::~QtWindowContext() {
    }

}