#include "cocoawindowcontext_p.h"

namespace QWK {

    CocoaWindowContext::CocoaWindowContext() {
    }

    CocoaWindowContext::~CocoaWindowContext() {
    }

    QString CocoaWindowContext::key() const {
        return QStringLiteral("cocoa");
    }

    void CocoaWindowContext::virtual_hook(int id, void *data) {
    }

    bool CocoaWindowContext::setupHost() {
        return false;
    }

}
