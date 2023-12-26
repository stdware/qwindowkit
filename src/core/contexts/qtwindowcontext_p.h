#ifndef QTWINDOWCONTEXT_P_H
#define QTWINDOWCONTEXT_P_H

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

    class QtWindowContext : public AbstractWindowContext {
        Q_OBJECT
    public:
        QtWindowContext();
        ~QtWindowContext() override;

        QString key() const override;
        void virtual_hook(int id, void *data) override;

    protected:
        void winIdChanged() override;

    protected:
        std::unique_ptr<SharedEventFilter> qtWindowEventFilter;
    };

}

#endif // QTWINDOWCONTEXT_P_H
