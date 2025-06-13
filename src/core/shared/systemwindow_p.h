// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#ifndef SYSTEMWINDOW_P_H
#define SYSTEMWINDOW_P_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the QWindowKit API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <QtGui/QWindow>
#include <QtGui/QMouseEvent>

#include <QWKCore/private/qwkglobal_p.h>

namespace QWK {

    class WindowMoveManipulator : public QObject {
    public:
        explicit WindowMoveManipulator(QWindow *targetWindow)
            : QObject(targetWindow), target(targetWindow), operationComplete(false),
              initialMousePosition(QCursor::pos()),
              initialWindowPosition(targetWindow->position()) {
            target->installEventFilter(this);
        }

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override {
            if (operationComplete) {
                return false;
            }
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
                    operationComplete = true;
                    deleteLater();
                    break;
                }

                default:
                    break;
            }
            return false;
        }

    private:
        QWindow *target;
        bool operationComplete;
        QPoint initialMousePosition;
        QPoint initialWindowPosition;
    };

    class WindowResizeManipulator : public QObject {
    public:
        WindowResizeManipulator(QWindow *targetWindow, Qt::Edges edges)
            : QObject(targetWindow), target(targetWindow), operationComplete(false),
              initialMousePosition(QCursor::pos()), initialWindowRect(target->geometry()),
              resizeEdges(edges) {
            target->installEventFilter(this);
        }

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override {
            if (operationComplete) {
                return false;
            }
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
                    operationComplete = true;
                    deleteLater();
                    break;
                }

                default:
                    break;
            }
            return false;
        }

    private:
        QWindow *target;
        bool operationComplete;
        QPoint initialMousePosition;
        QRect initialWindowRect;
        Qt::Edges resizeEdges;
    };

    // QWindow::startSystemMove() and QWindow::startSystemResize() is first supported at Qt 5.15
    // QWindow::startSystemResize() returns false on macOS
    // QWindow::startSystemMove() and QWindow::startSystemResize() returns false on Linux Unity DE

    // When the new API fails, we emulate the window actions using the classical API.

    inline void startSystemMove(QWindow *window) {
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
        std::ignore = new WindowMoveManipulator(window);
#elif defined(Q_OS_LINUX)
        if (window->startSystemMove()) {
            return;
        }
        std::ignore = new WindowMoveManipulator(window);
#else
        window->startSystemMove();
#endif
    }

    inline void startSystemResize(QWindow *window, Qt::Edges edges) {
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
        std::ignore = new WindowResizeManipulator(window, edges);
#elif defined(Q_OS_MAC) || defined(Q_OS_LINUX)
        if (window->startSystemResize(edges)) {
            return;
        }
        std::ignore = new WindowResizeManipulator(window, edges);
#else
        window->startSystemResize(edges);
#endif
    }

}

#endif // SYSTEMWINDOW_P_H
