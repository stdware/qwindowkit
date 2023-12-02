#ifndef QTWINDOWCONTEXT_P_H
#define QTWINDOWCONTEXT_P_H

#include <QWKCore/private/abstractwindowcontext_p.h>

namespace QWK {

    class QWK_CORE_EXPORT QtWindowContext : public AbstractWindowContext {
        Q_OBJECT
        Q_DISABLE_COPY(QtWindowContext)

    public:
        QtWindowContext(QWindow *window, WindowItemDelegatePtr delegate);
        ~QtWindowContext();

    public:
        bool setup() override;

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override;
    };

}

#endif // QTWINDOWCONTEXT_P_H
