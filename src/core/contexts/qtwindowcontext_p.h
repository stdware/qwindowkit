#ifndef QTWINDOWCONTEXT_P_H
#define QTWINDOWCONTEXT_P_H

#include <QWKCore/private/abstractwindowcontext_p.h>

namespace QWK {

    class QWK_CORE_EXPORT QtWindowContext : public AbstractWindowContext {
        Q_OBJECT
    public:
        QtWindowContext();
        ~QtWindowContext();

    public:
        bool setup(QObject *host, WindowItemDelegate *delegate) override;

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override;
    };

}

#endif // QTWINDOWCONTEXT_P_H
