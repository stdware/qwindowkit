#ifndef WINDOWBUTTONPRIVATE_H
#define WINDOWBUTTONPRIVATE_H

#include "windowbutton.h"

namespace QWK {

    class WindowButtonPrivate {
        Q_DECLARE_PUBLIC(WindowButton)
    public:
        WindowButtonPrivate();
        virtual ~WindowButtonPrivate();

        void init();

        WindowButton *q_ptr;

        QIcon iconNormal;
        QIcon iconChecked;
        QIcon iconDisabled;

        void reloadIcon();
    };

}

#endif // WINDOWBUTTONPRIVATE_H