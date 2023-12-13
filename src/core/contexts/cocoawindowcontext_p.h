#ifndef COCOAWINDOWCONTEXT_P_H
#define COCOAWINDOWCONTEXT_P_H


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
        bool setupHost() override;
    };

}

#endif // COCOAWINDOWCONTEXT_P_H
