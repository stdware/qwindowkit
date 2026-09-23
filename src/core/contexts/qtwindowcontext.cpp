// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include "qtwindowcontext_p.h"

#include <QtCore/QDebug>

#include "qwkglobal_p.h"
#include "systemwindow_p.h"

namespace QWK {

    static constexpr const quint8 kDefaultResizeBorderThickness = 8;

    static Qt::CursorShape calculateCursorShape(Qt::Edges edges) {
        const bool horizontal = edges & (Qt::LeftEdge | Qt::RightEdge);
        const bool vertical = edges & (Qt::TopEdge | Qt::BottomEdge);
        if (horizontal && vertical) {
            return edges == (Qt::LeftEdge | Qt::TopEdge) ||
                           edges == (Qt::RightEdge | Qt::BottomEdge)
                       ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor;
        }
        if (horizontal)
            return Qt::SizeHorCursor;
        if (vertical)
            return Qt::SizeVerCursor;
        return Qt::ArrowCursor;
    }

    static inline Qt::Edges calculateWindowEdges(const QWindow *window, const QPoint &pos,
                                                  bool widthFixed, bool heightFixed) {
#ifdef Q_OS_MACOS
        Q_UNUSED(window);
        Q_UNUSED(pos);
        Q_UNUSED(widthFixed);
        Q_UNUSED(heightFixed);
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
        if (widthFixed)
            edges &= ~(Qt::LeftEdge | Qt::RightEdge);
        if (heightFixed)
            edges &= ~(Qt::TopEdge | Qt::BottomEdge);
        return edges;
#endif
    }

    class QtWindowEventFilter : public QObject, public SharedEventFilter {
    public:
        explicit QtWindowEventFilter(QtWindowContext *context);
        ~QtWindowEventFilter() override;

        enum WindowStatus {
            Idle,
            WaitingRelease,
            PreparingMove,
            Moving,
            Resizing,
        };

    protected:
        bool sharedEventFilter(QObject *object, QEvent *event) override;
        bool eventFilter(QObject *object, QEvent *event) override;

    private:
        void observeCursorHost(QObject *host, QWindow *window);
        void restoreCursor();
        QtWindowContext *m_context;
        QPointer<QObject> m_cursorHost;
        QPointer<QWindow> m_cursorWindow;
        bool m_cursorShapeChanged;
        WindowStatus m_windowStatus;
    };

    QtWindowEventFilter::QtWindowEventFilter(QtWindowContext *context)
        : m_context(context), m_cursorShapeChanged(false), m_windowStatus(Idle) {
        m_context->installSharedEventFilter(this);
    }

    QtWindowEventFilter::~QtWindowEventFilter() {
        restoreCursor();
    }

    void QtWindowEventFilter::restoreCursor() {
        if (!m_cursorShapeChanged)
            return;
        m_cursorShapeChanged = false;
        if (m_cursorHost)
            m_context->delegate()->restoreCursorShape(m_cursorHost);
    }

    void QtWindowEventFilter::observeCursorHost(QObject *host, QWindow *window) {
        if (m_cursorHost != host) {
            m_cursorHost = host;
            host->installEventFilter(this);
        }
        if (m_cursorWindow == window)
            return;
        if (m_cursorWindow)
            disconnect(m_cursorWindow, nullptr, this, nullptr);
        m_cursorWindow = window;
        // QWindow constraints can change without a mouse or resize event. QWidget
        // updates them without signals; that path is rechecked on the next mouse event.
        connect(window, &QWindow::minimumWidthChanged, this, [this] { restoreCursor(); });
        connect(window, &QWindow::minimumHeightChanged, this, [this] { restoreCursor(); });
        connect(window, &QWindow::maximumWidthChanged, this, [this] { restoreCursor(); });
        connect(window, &QWindow::maximumHeightChanged, this, [this] { restoreCursor(); });
        connect(window, &QWindow::windowStateChanged, this, [this] {
            m_windowStatus = Idle;
            restoreCursor();
        });
    }

    bool QtWindowEventFilter::eventFilter(QObject *object, QEvent *event) {
        if (object == m_cursorHost) {
            if (event->type() == QEvent::Destroy) {
                // QWidget delivers Destroy before tearing down its cursor storage.
                // Do not try to restore into a host whose destruction has begun.
                m_cursorHost.clear();
                m_cursorShapeChanged = false;
                return false;
            }
            switch (event->type()) {
                case QEvent::Leave:
                case QEvent::Hide:
                case QEvent::WindowStateChange: {
                    const QPointer<QObject> receiver(object);
                    restoreCursor();
                    return !receiver;
                }
                default:
                    break;
            }
        }
        return false;
    }

    bool QtWindowEventFilter::sharedEventFilter(QObject *obj, QEvent *event) {
        auto type = event->type();
        if (type == QEvent::Leave || type == QEvent::Hide ||
            type == QEvent::WindowStateChange || type == QEvent::WinIdChange) {
            const QPointer<QtWindowEventFilter> self(this);
            const QPointer<QObject> receiver(obj);
            if (type != QEvent::Leave)
                m_windowStatus = Idle;
            restoreCursor();
            return !self || !receiver;
        }
        if (type < QEvent::MouseButtonPress || type > QEvent::MouseMove) {
            return false;
        }
        auto host = m_context->host();
        auto window = m_context->window();
        auto delegate = m_context->delegate();
        if (!host || !window)
            return false;
        const QPointer<QtWindowEventFilter> self(this);
        const QPointer<QObject> receiver(host);
        auto me = static_cast<const QMouseEvent *>(event);
        const bool widthFixed = m_context->isHostWidthFixed();
        const bool heightFixed = m_context->isHostHeightFixed();
        const bool fixedSize = widthFixed && heightFixed;

        QPoint scenePos = getMouseEventScenePos(me);
        QPoint globalPos = getMouseEventGlobalPos(me);

        bool inTitleBar = m_context->isInTitleBarDraggableArea(scenePos);

        const Qt::Edges edges = calculateWindowEdges(window, scenePos, widthFixed, heightFixed);
        const auto& updateCursorShape{ [&](){
            const Qt::CursorShape shape = calculateCursorShape(edges);
            if (shape == Qt::ArrowCursor) {
                restoreCursor();
            } else {
                observeCursorHost(host, window);
                m_cursorShapeChanged = true;
                delegate->setCursorShape(host, shape);
            }
        } };

        bool handled = false;

        switch (type) {
            case QEvent::MouseButtonPress: {
                m_windowStatus = WaitingRelease;
                switch (me->button()) {
                    case Qt::LeftButton: {
                        updateCursorShape();
                        if (!self || !receiver)
                            return true;
                        if (edges != Qt::Edges()) {
                            m_context->systemResize(edges);
                            m_windowStatus = Resizing;
                            handled = true;
                            break;
                        }
                        if (inTitleBar) {
                            // If we call startSystemMove() now but release the mouse without actual
                            // movement, there will be no MouseReleaseEvent, so we defer it when the
                            // mouse is actually moving for the first time
                            m_windowStatus = PreparingMove;
                            handled = true;
                        }
                        break;
                    }
                    case Qt::RightButton: {
                        if (inTitleBar) {
                            m_context->showSystemMenu(globalPos);
                            m_windowStatus = Idle;
                            handled = true;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }

            case QEvent::MouseButtonRelease: {
                switch (m_windowStatus) {
                    case Idle: {
                        if (inTitleBar) {
                            handled = true;
                        }
                        break;
                    }
                    case WaitingRelease:
                        break;
                    case PreparingMove:
                    case Moving:
                    case Resizing: {
                        handled = true;
                        break;
                    }
                }
                m_windowStatus = Idle;
                updateCursorShape();
                if (!self || !receiver)
                    return true;
                break;
            }

            case QEvent::MouseMove: {
                switch (m_windowStatus) {
                    case Idle:
                    case WaitingRelease: {
                        updateCursorShape();
                        if (!self || !receiver)
                            return true;
                        break;
                    }
                    case PreparingMove: {
                        m_context->systemMove();
                        m_windowStatus = Moving;
                        handled = true;
                        break;
                    }
                    case Moving:
                    case Resizing: {
                        handled = true;
                        break;
                    }
                }
                break;
            }

            case QEvent::MouseButtonDblClick: {
                if (me->button() == Qt::LeftButton && inTitleBar && !fixedSize) {
                    Qt::WindowFlags windowFlags = delegate->getWindowFlags(host);
                    Qt::WindowStates windowState = delegate->getWindowState(host);
                    if ((windowFlags & Qt::WindowMaximizeButtonHint) &&
                        !(windowState & Qt::WindowFullScreen)) {
                        if (windowState & Qt::WindowMaximized) {
                            delegate->setWindowState(host, windowState & ~Qt::WindowMaximized);
                        } else {
                            delegate->setWindowState(host, windowState | Qt::WindowMaximized);
                        }
                        handled = true;
                    }
                }
                break;
            }

            default:
                break;
        }

        if (handled) {
            event->accept();
            return true;
        }

        return false;
    }

    QtWindowContext::QtWindowContext() : AbstractWindowContext() {
        qtWindowEventFilter = std::make_unique<QtWindowEventFilter>(this);
    }

    QtWindowContext::~QtWindowContext() = default;

    void QtWindowContext::systemMove() {
        startSystemMove(window());
    }

    void QtWindowContext::systemResize(Qt::Edges edges) {
        startSystemResize(window(), edges);
    }

    QString QtWindowContext::key() const {
        return QStringLiteral("qt");
    }

    void QtWindowContext::winIdChanged(WId winId, WId oldWinId) {
        if (!m_windowHandle) {
            m_delegate->setWindowFlags(m_host, m_delegate->getWindowFlags(m_host) &
                                                   ~Qt::FramelessWindowHint);
            return;
        }

        // Allocate new resources
        m_delegate->setWindowFlags(m_host,
                                   m_delegate->getWindowFlags(m_host) | Qt::FramelessWindowHint);
    }

}
