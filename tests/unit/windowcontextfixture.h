// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef QWK_TEST_WINDOWCONTEXTFIXTURE_H
#define QWK_TEST_WINDOWCONTEXTFIXTURE_H

#include <QtCore/QStringList>
#include <QWKCore/private/abstractwindowcontext_p.h>

namespace QwkTest {
    class Item : public QObject {
    public:
        QRect rect{0, 0, 200, 40};
        bool visible = true;
        bool enabled = true;
    };

    class HandleFilter : public QWK::WinIdChangeEventFilter {
    public:
        HandleFilter(QObject *host, QWK::AbstractWindowContext *context, const WId &id)
            : WinIdChangeEventFilter(host, context), id(id) {}
        WId winId() const override { return id; }
    private:
        const WId &id;
    };

    // Supply deterministic platform inputs. Attribute storage, replay, registration and hit
    // testing remain the production AbstractWindowContext implementation.
    class Delegate : public QWK::WindowItemDelegate {
    public:
        explicit Delegate(QWindow *window) : host(window) {}
        QWindow *host;
        WId id = 0;
        mutable Qt::WindowStates state;
        mutable Qt::WindowFlags flags = Qt::Window | Qt::WindowMaximizeButtonHint;
        mutable QStringList operations;
        QRect geometry{500, 600, 200, 100};
        QWindow *window(const QObject *) const override { return host; }
        QWindow *hostWindow(const QObject *) const override { return host; }
        bool isEnabled(const QObject *obj) const override { return static_cast<const Item *>(obj)->enabled; }
        bool isVisible(const QObject *obj) const override { return static_cast<const Item *>(obj)->visible; }
        QRect mapGeometryToScene(const QObject *obj) const override { return static_cast<const Item *>(obj)->rect; }
        bool isWindowActive(const QObject *) const override { return false; }
        Qt::WindowStates getWindowState(const QObject *) const override { return state; }
        Qt::WindowFlags getWindowFlags(const QObject *) const override { return flags; }
        QRect getGeometry(const QObject *) const override { return geometry; }
        void setWindowState(QObject *, Qt::WindowStates value) const override {
            state = value;
            operations.append("state");
        }
        void setCursorShape(QObject *, Qt::CursorShape) const override {}
        void restoreCursorShape(QObject *) const override {}
        void setWindowFlags(QObject *, Qt::WindowFlags value) const override {
            flags = value;
            operations.append("flags");
        }
        void setWindowVisible(QObject *, bool value) const override {
            operations.append(value ? "show" : "hide");
        }
        void setGeometry(QObject *, const QRect &value) override {
            geometry = value;
            operations.append("geometry");
        }
        void bringWindowToTop(QObject *) const override { operations.append("raise"); }
        QWK::WinIdChangeEventFilter *createWinIdEventFilter(
            QObject *host, QWK::AbstractWindowContext *context) const override {
            return new HandleFilter(host, context, id);
        }
    };

}

#endif
