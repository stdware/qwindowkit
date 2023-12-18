#include "qtwindowcontext_p.h"

#include <QtCore/QDebug>

#include "qwkglobal_p.h"

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

    static inline Qt::Edges calculateWindowEdges(const QWindow *window, const QPoint &pos) {
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

#ifdef Q_OS_LINUX
    class WindowMoveManipulator : public QObject {
    public:
        explicit WindowMoveManipulator(QWindow *targetWindow)
            : QObject(targetWindow), target(targetWindow), initialMousePosition(QCursor::pos()),
              initialWindowPosition(targetWindow->position()) {
            target->installEventFilter(this);
        }

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override {
            switch (event->type()) {
                case QEvent::MouseMove: {
                    auto mouseEvent = static_cast<QMouseEvent *>(event);
                    QPoint delta = getMouseEventGlobalPos(mouseEvent) - initialMousePosition;
                    target->setPosition(initialWindowPosition + delta);
                    return true;
                }

                case QEvent::MouseButtonRelease: {
                    if (target->y() < 0) {
                        target->setPosition(target->x(), 0);
                    }
                    target->removeEventFilter(this);
                    this->deleteLater();
                }

                default:
                    break;
            }
            return false;
        }

    private:
        QWindow *target;
        QPoint initialMousePosition;
        QPoint initialWindowPosition;
    };
#endif

#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
    class WindowResizeManipulator : public QObject {
    public:
        WindowResizeManipulator(QWindow *targetWindow, Qt::Edges edges)
            : QObject(targetWindow), target(targetWindow), resizeEdges(edges),
              initialMousePosition(QCursor::pos()), initialWindowRect(target->geometry()) {
            target->installEventFilter(this);
        }

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override {
            switch (event->type()) {
                case QEvent::MouseMove: {
                    auto mouseEvent = static_cast<QMouseEvent *>(event);
                    QPoint globalMousePos = getMouseEventGlobalPos(mouseEvent);
                    QRect windowRect = initialWindowRect;

                    if (resizeEdges & Qt::LeftEdge) {
                        int delta = globalMousePos.x() - initialMousePosition.x();
                        windowRect.setLeft(initialWindowRect.left() + delta);
                    }
                    if (resizeEdges & Qt::RightEdge) {
                        int delta = globalMousePos.x() - initialMousePosition.x();
                        windowRect.setRight(initialWindowRect.right() + delta);
                    }
                    if (resizeEdges & Qt::TopEdge) {
                        int delta = globalMousePos.y() - initialMousePosition.y();
                        windowRect.setTop(initialWindowRect.top() + delta);
                    }
                    if (resizeEdges & Qt::BottomEdge) {
                        int delta = globalMousePos.y() - initialMousePosition.y();
                        windowRect.setBottom(initialWindowRect.bottom() + delta);
                    }

                    target->setGeometry(windowRect);
                    return true;
                }

                case QEvent::MouseButtonRelease: {
                    target->removeEventFilter(this);
                    this->deleteLater();
                }

                default:
                    break;
            }
            return false;
        }

    private:
        QWindow *target;
        QPoint initialMousePosition;
        QRect initialWindowRect;
        Qt::Edges resizeEdges;
    };
#endif

    static inline void startSystemMove(QWindow *window) {
#ifdef Q_OS_LINUX
        if (window->startSystemMove()) {
            return;
        }
        std::ignore = new WindowMoveManipulator(window);
#else
        window->startSystemMove();
#endif
    }

    static inline void startSystemResize(QWindow *window, Qt::Edges edges) {
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
        if (window->startSystemResize(edges)) {
            return;
        }
        std::ignore = new WindowResizeManipulator(window, edges);
#else
        window->startSystemResize(edges);
#endif
    }

    class QtWindowEventFilter : public QObject {
    public:
        explicit QtWindowEventFilter(AbstractWindowContext *context, QObject *parent = nullptr);
        ~QtWindowEventFilter() override;

        enum WindowStatus {
            Idle,
            WaitingRelease,
            PreparingMove,
            Moving,
            Resizing,
        };

    protected:
        bool eventFilter(QObject *object, QEvent *event) override;

    private:
        AbstractWindowContext *m_context;
        bool m_cursorShapeChanged;
        WindowStatus m_windowStatus;
    };

    QtWindowEventFilter::QtWindowEventFilter(AbstractWindowContext *context, QObject *parent)
        : QObject(parent), m_context(context), m_cursorShapeChanged(false), m_windowStatus(Idle) {
        m_context->window()->installEventFilter(this);
    }

    QtWindowEventFilter::~QtWindowEventFilter() = default;

    bool QtWindowEventFilter::eventFilter(QObject *obj, QEvent *event) {
        Q_UNUSED(obj)
        auto type = event->type();
        if (type < QEvent::MouseButtonPress || type > QEvent::MouseMove) {
            return false;
        }
        QObject *host = m_context->host();
        QWindow *window = m_context->window();
        WindowItemDelegate *delegate = m_context->delegate();
        bool fixedSize = delegate->isHostSizeFixed(host);
        auto me = static_cast<const QMouseEvent *>(event);

        QPoint scenePos = getMouseEventScenePos(me);
        QPoint globalPos = getMouseEventGlobalPos(me);

        bool inTitleBar = m_context->isInTitleBarDraggableArea(scenePos);
        switch (type) {
            case QEvent::MouseButtonPress: {
                switch (me->button()) {
                    case Qt::LeftButton: {
                        if (!fixedSize) {
                            Qt::Edges edges = calculateWindowEdges(window, scenePos);
                            if (edges != Qt::Edges()) {
                                m_windowStatus = Resizing;
                                startSystemResize(window, edges);
                                event->accept();
                                return true;
                            }
                        }
                        if (inTitleBar) {
                            // If we call startSystemMove() now but release the mouse without actual
                            // movement, there will be no MouseReleaseEvent, so we defer it when the
                            // mouse is actually moving for the first time
                            m_windowStatus = PreparingMove;
                            event->accept();
                            return true;
                        }
                        break;
                    }
                    case Qt::RightButton: {
                        m_context->showSystemMenu(globalPos);
                        break;
                    }
                    default:
                        break;
                }
                m_windowStatus = WaitingRelease;
                break;
            }

            case QEvent::MouseButtonRelease: {
                switch (m_windowStatus) {
                    case PreparingMove:
                    case Moving:
                    case Resizing: {
                        m_windowStatus = Idle;
                        event->accept();
                        return true;
                    }
                    case WaitingRelease: {
                        m_windowStatus = Idle;
                        break;
                    }
                    default: {
                        if (inTitleBar) {
                            event->accept();
                            return true;
                        }
                        break;
                    }
                }
                break;
            }

            case QEvent::MouseMove: {
                switch (m_windowStatus) {
                    case Moving: {
                        return true;
                    }
                    case PreparingMove: {
                        m_windowStatus = Moving;
                        startSystemMove(window);
                        event->accept();
                        return true;
                    }
                    case Idle: {
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
                        break;
                    }
                    default:
                        break;
                }
                break;
            }

            case QEvent::MouseButtonDblClick: {
                if (me->button() == Qt::LeftButton && inTitleBar && !fixedSize) {
                    Qt::WindowStates windowState = delegate->getWindowState(host);
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

            default:
                break;
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
        switch (id) {
            case ShowSystemMenuHook:
                return;
            default:
                break;
        }
        AbstractWindowContext::virtual_hook(id, data);
    }

    void QtWindowContext::winIdChanged(QWindow *oldWindow) {
        Q_UNUSED(oldWindow)
        m_delegate->setWindowFlags(m_host,
                                   m_delegate->getWindowFlags(m_host) | Qt::FramelessWindowHint);
        qtWindowEventFilter = std::make_unique<QtWindowEventFilter>(this);
    }

}
