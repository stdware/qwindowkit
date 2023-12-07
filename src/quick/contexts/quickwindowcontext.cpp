#include "quickwindowcontext_p.h"

namespace QWK {

    bool QuickWindowContext::hostEventFilter(QEvent *event) {
        return false;
    }

}