#include "qtwindowcontext_p.h"

namespace QWK {

    static constexpr const int kDefaultResizeBorderThickness = 8;

    static Qt::CursorShape calculateCursorShape(const QWindow *window, const QPoint &pos) {
#ifdef Q_OS_MACOS
        Q_UNUSED(window);
        Q_UNUSED(pos);
        return Qt::ArrowCursor;
#else
        Q_ASSERT(window);
        if (!window) {
            return Qt::ArrowCursor;
        }
        if (window->visibility() != QWindow::Windowed) {
            return Qt::ArrowCursor;
        }
        const int x = pos.x();
        const int y = pos.y();
        const int w = window->width();
        const int h = window->height();
        if (((x < kDefaultResizeBorderThickness) && (y < kDefaultResizeBorderThickness)) ||
            ((x >= (w - kDefaultResizeBorderThickness)) &&
             (y >= (h - kDefaultResizeBorderThickness)))) {
            return Qt::SizeFDiagCursor;
        }
        if (((x >= (w - kDefaultResizeBorderThickness)) && (y < kDefaultResizeBorderThickness)) ||
            ((x < kDefaultResizeBorderThickness) && (y >= (h - kDefaultResizeBorderThickness)))) {
            return Qt::SizeBDiagCursor;
        }
        if ((x < kDefaultResizeBorderThickness) || (x >= (w - kDefaultResizeBorderThickness))) {
            return Qt::SizeHorCursor;
        }
        if ((y < kDefaultResizeBorderThickness) || (y >= (h - kDefaultResizeBorderThickness))) {
            return Qt::SizeVerCursor;
        }
        return Qt::ArrowCursor;
#endif
    }

    QtWindowContext::QtWindowContext(QWindow *window, WindowItemDelegatePtr delegate)
        : AbstractWindowContext(window, delegate) {
    }

    QtWindowContext::~QtWindowContext() {
    }

    bool QtWindowContext::setup() {
        return false;
    }

    bool QtWindowContext::eventFilter(QObject *obj, QEvent *event) {
        return AbstractWindowContext::eventFilter(obj, event);
    }

}