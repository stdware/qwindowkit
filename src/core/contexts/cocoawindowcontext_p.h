#ifndef COCOAWINDOWCONTEXT_P_H
#define COCOAWINDOWCONTEXT_P_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the QWindowKit API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <QWKCore/private/abstractwindowcontext_p.h>

namespace QWK {

    class CocoaWindowContext : public AbstractWindowContext {
        Q_OBJECT
    public:
        CocoaWindowContext();
        ~CocoaWindowContext() override;

        QString key() const override;
        void virtual_hook(int id, void *data) override;

    protected:
        void winIdChanged(QWindow *oldWindow, bool isDestroyed) override;

    protected:
        WId windowId = 0;

        std::unique_ptr<QObject> cocoaWindowEventFilter;
    };

}

#endif // COCOAWINDOWCONTEXT_P_H
