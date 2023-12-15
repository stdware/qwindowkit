#ifndef QTWINDOWCONTEXT_P_H
#define QTWINDOWCONTEXT_P_H

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
        bool setupHost() override;
    };

}

#endif // QTWINDOWCONTEXT_P_H
