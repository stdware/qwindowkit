// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
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
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>

#include <QWKCore/private/qwkglobal_p.h>

namespace QWK {

    // Keep the point used to grab the title area reachable, including custom title
    // bars away from the top edge. Use each screen separately: a virtual desktop's
    // bounding rectangle may contain gaps. The 16-DIP inset leaves room to grab
    // again; tiny screens use their center. Do not try to fit oversized windows.
    inline QPoint reachableWindowPosition(const QPoint &position, const QPoint &grabOffset,
                                          const QList<QRect> &availableAreas) {
        const QPoint grabPosition = position + grabOffset;
        QPoint bestPosition = position;
        double bestDistance = -1;
        for (const auto &area : availableAreas) {
            if (area.isEmpty())
                continue;
            const int insetX = qMin(16, (area.width() - 1) / 2);
            const int insetY = qMin(16, (area.height() - 1) / 2);
            const QPoint reachableGrab(qBound(area.left() + insetX, grabPosition.x(),
                                              area.right() - insetX),
                                       qBound(area.top() + insetY, grabPosition.y(),
                                              area.bottom() - insetY));
            const double dx = double(reachableGrab.x()) - grabPosition.x();
            const double dy = double(reachableGrab.y()) - grabPosition.y();
            const double distance = dx * dx + dy * dy;
            if (bestDistance < 0 || distance < bestDistance) {
                bestDistance = distance;
                bestPosition = position + (reachableGrab - grabPosition);
            }
        }
        // No valid screen data: preserve the position rather than invent an origin.
        return bestPosition;
    }

    class WindowMoveManipulator : public QObject {
    public:
        explicit WindowMoveManipulator(QWindow *targetWindow,
                                       const QPoint &mousePosition = QCursor::pos())
            : QObject(targetWindow), target(targetWindow), operationComplete(false),
              initialMousePosition(mousePosition),
              initialWindowPosition(targetWindow->position()) {
            const QPoint offset = initialMousePosition - initialWindowPosition;
            grabOffset = QPoint(qBound(0, offset.x(), qMax(0, target->width() - 1)),
                                qBound(0, offset.y(), qMax(0, target->height() - 1)));
            target->installEventFilter(this);
        }

    protected:
        virtual QList<QRect> availableScreenGeometries() const {
            QList<QRect> areas;
            const auto screen = target->screen();
            // Prefer the current screen on an exact distance tie. Query at release
            // so screen removal and work-area changes during the drag are reflected.
            if (screen)
                areas.append(screen->availableGeometry());
            const auto screens = screen ? screen->virtualSiblings() : QGuiApplication::screens();
            for (const auto sibling : screens) {
                if (sibling != screen)
                    areas.append(sibling->availableGeometry());
            }
            return areas;
        }

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
                    const QPoint position = reachableWindowPosition(
                        target->position(), grabOffset, availableScreenGeometries());
                    operationComplete = true;
                    deleteLater();
                    // Finish before moving: setPosition can deliver synchronous events.
                    if (position != target->position())
                        target->setPosition(position);
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
        QPoint grabOffset;
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
        Q_ASSERT(window);
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
        Q_ASSERT(window);
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
