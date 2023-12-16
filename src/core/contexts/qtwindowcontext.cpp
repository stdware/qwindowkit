#include "qtwindowcontext_p.h"

namespace QWK {

    static constexpr const quint8 kDefaultResizeBorderThickness = 8;

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

    static inline Qt::Edges calculateWindowEdges(const QWindow *window, const QPoint &pos)
    {
#ifdef Q_OS_MACOS
        Q_UNUSED(window);
        Q_UNUSED(pos);
        return {};
#else
        Q_ASSERT(window);
        if (!window) {
            return {};
        }
        if (window->visibility() != QWindow::Windowed) {
            return {};
        }
        Qt::Edges edges = {};
        const int x = pos.x();
        const int y = pos.y();
        if (x < kDefaultResizeBorderThickness) {
            edges |= Qt::LeftEdge;
        }
        if (x >= (window->width() - kDefaultResizeBorderThickness)) {
            edges |= Qt::RightEdge;
        }
        if (y < kDefaultResizeBorderThickness) {
            edges |= Qt::TopEdge;
        }
        if (y >= (window->height() - kDefaultResizeBorderThickness)) {
            edges |= Qt::BottomEdge;
        }
        return edges;
#endif
    }

    class WindowEventFilter : public QObject {
    public:
        explicit WindowEventFilter(AbstractWindowContext *context, QObject *parent = nullptr);
        ~WindowEventFilter() override;

    protected:
        bool eventFilter(QObject *object, QEvent *event) override;

    private:
        AbstractWindowContext *m_context;
        bool m_leftButtonPressed;
        bool m_cursorShapeChanged;
    };

    WindowEventFilter::WindowEventFilter(AbstractWindowContext *context, QObject *parent) : QObject(parent), m_context(context), m_leftButtonPressed(false), m_cursorShapeChanged(false) {}

    WindowEventFilter::~WindowEventFilter() = default;

    bool WindowEventFilter::eventFilter(QObject *object, QEvent *event) {
        const auto type = event->type();
        if (type < QEvent::MouseButtonPress || type > QEvent::MouseMove) {
            return false;
        }
        QObject *host = m_context->host();
        QWindow *window = m_context->window();
        WindowItemDelegate *delegate = m_context->delegate();
        const bool fixedSize = delegate->isHostSizeFixed(host);
        const auto mouseEvent = static_cast<const QMouseEvent *>(event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const QPoint scenePos = mouseEvent->scenePosition().toPoint();
        const QPoint globalPos = mouseEvent->globalPosition().toPoint();
#else
        const QPoint scenePos = mouseEvent->windowPos().toPoint();
        const QPoint globalPos = mouseEvent->screenPos().toPoint();
#endif
        const bool inTitleBar = m_context->isInTitleBarDraggableArea(scenePos);
        switch (type) {
            case QEvent::MouseButtonPress: {
                if (mouseEvent->button() == Qt::LeftButton) {
                    m_leftButtonPressed = true;
                    if (!fixedSize) {
                        const Qt::Edges edges = calculateWindowEdges(window, scenePos);
                        if (edges != Qt::Edges{}) {
                            window->startSystemResize(edges);
                            event->accept();
                            return true;
                        }
                    }
                    if (inTitleBar) {
                        event->accept();
                        return true;
                    }
                }
                break;
            }
            case QEvent::MouseButtonRelease: {
                if (mouseEvent->button() == Qt::LeftButton) {
                    m_leftButtonPressed = false;
                    if (inTitleBar) {
                        event->accept();
                        return true;
                    }
                }
                break;
            }
            case QEvent::MouseMove: {
                if (!fixedSize) {
                    const Qt::CursorShape shape = calculateCursorShape(window, scenePos);
                    if (shape == Qt::ArrowCursor) {
                        if (m_cursorShapeChanged) {
                            delegate->restoreCursorShape(host);
                            m_cursorShapeChanged = false;
                        }
                    } else {
                        delegate->setCursorShape(host, shape);
                        m_cursorShapeChanged = true;
                    }
                }
                if (m_leftButtonPressed && inTitleBar) {
                    window->startSystemMove();
                    event->accept();
                    return true;
                }
                break;
            }
            case QEvent::MouseButtonDblClick: {
                if (mouseEvent->button() == Qt::LeftButton && inTitleBar && !fixedSize) {
                    const Qt::WindowStates windowState = delegate->getWindowState(host);
                    if (!(windowState & Qt::WindowFullScreen)) {
                        if (windowState & Qt::WindowMaximized) {
                            delegate->setWindowState(host, windowState & ~Qt::WindowMaximized);
                        } else {
                            delegate->setWindowState(host, windowState | Qt::WindowMaximized);
                        }
                        event->accept();
                        return true;
                    }
                }
                break;
            }
            default: {
                Q_UNREACHABLE();
                return false;
            }
        }
        return false;
    }

    QtWindowContext::QtWindowContext() : AbstractWindowContext() {
    }

    QtWindowContext::~QtWindowContext() = default;

    QString QtWindowContext::key() const {
        return QStringLiteral("qt");
    }

    void QtWindowContext::virtual_hook(int id, void *data) {
    }

    bool QtWindowContext::setupHost() {
        m_delegate->setWindowFlags(m_host, Qt::FramelessWindowHint);
        m_windowHandle->installEventFilter(new WindowEventFilter(this, m_windowHandle));
        return true;
    }

}