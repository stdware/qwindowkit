#ifndef WIN10BORDERHANDLER_P_H
#define WIN10BORDERHANDLER_P_H

#include <QtGui/QPainter>

#include <QWKCore/qwkcoreglobal.h>

namespace QWK {

    class QWK_CORE_EXPORT Win10BorderHandler {
    public:
        Win10BorderHandler(QWindow *window);
        virtual ~Win10BorderHandler();

    public:
        virtual void updateGeometry() = 0;
        virtual void requestUpdate() = 0;

        virtual bool isActive() const = 0;

        inline int borderThickness() const;
        inline void setBorderThickness(int borderThickness);

    protected:
        // implemented in `win32windowcontext.cpp`
        void paintBorder(QPainter &painter, const QRect &rect, const QRegion &region);

    protected:
        QWindow *m_window;
        int m_borderThickness;

        Q_DISABLE_COPY(Win10BorderHandler)
    };

    inline int Win10BorderHandler::borderThickness() const {
        return m_borderThickness;
    }

    inline void Win10BorderHandler::setBorderThickness(int borderThickness) {
        m_borderThickness = borderThickness;
        updateGeometry();
    }

}

#endif // WIN10BORDERHANDLER_P_H
