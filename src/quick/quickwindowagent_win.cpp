// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "quickwindowagent_p.h"

#include <QtQuick/QQuickPaintedItem>
#include <QtQuick/QSGRectangleNode>
#include <QtQuick/QSGRendererInterface>

#include <QWKCore/private/qwkwindowsextra_p.h>
#include <QWKCore/private/windows10borderhandler_p.h>

namespace QWK {

#if QWINDOWKIT_CONFIG(ENABLE_WINDOWS_SYSTEM_BORDERS)

    // Retain the native path's transparent painted surface and subpixel offset until its
    // DWM seam workaround can be validated on Windows 10. Emulated borders do not use it.
    class NativeBorderSurface : public QQuickPaintedItem {
    public:
        explicit NativeBorderSurface(QQuickItem *parent) : QQuickPaintedItem(parent) {
            setAntialiasing(true);
            setFillColor(Qt::transparent);
            setOpaquePainting(true);
            setAcceptedMouseButtons(Qt::NoButton);
            setAcceptTouchEvents(false);
            setAcceptHoverEvents(false);
        }

        void paint(QPainter *) override {}
    };

    class BorderItem : public QQuickItem, public Windows10BorderHandler {
    public:
        explicit BorderItem(QQuickItem *parent, AbstractWindowContext *context);
        ~BorderItem() override;

        void updateGeometry() override;

    protected:
        QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
        void itemChange(ItemChange change, const ItemChangeData &data) override;
        bool sharedEventFilter(QObject *obj, QEvent *event) override;
        bool nativeEventFilter(const QByteArray &eventType, void *message,
                               QT_NATIVE_EVENT_RESULT_TYPE *result) override;

    private:
        bool shouldEnableNativePainter() const;
        void afterSynchronizing();

        // Written on the GUI thread; read only during synchronization while it is blocked.
        QColor borderColor;
        WId nativeWindowId = 0;
        bool nativePainting = false;
        std::unique_ptr<NativeBorderSurface> nativeSurface;
    };

    bool BorderItem::shouldEnableNativePainter() const {
#  if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        if (auto win = window()) {
            switch (win->rendererInterface()->graphicsApi()) {
                case QSGRendererInterface::OpenGL:
                case QSGRendererInterface::Direct3D11:
#    if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
                case QSGRendererInterface::Direct3D12:
#    endif
                    return true;
                default:
                    break;
            }
        }
#  endif
        return false;
    }

    BorderItem::BorderItem(QQuickItem *parent, AbstractWindowContext *context)
        : QQuickItem(parent), Windows10BorderHandler(context, this) {
        setFlag(ItemHasContents);
        setAntialiasing(false);
        setAcceptedMouseButtons(Qt::NoButton);
        setAcceptTouchEvents(false);
        setAcceptHoverEvents(false);
        setActiveFocusOnTab(false);
        setEnabled(false);
        // A decoration above ordinary sibling content, not above other native windows.
        setZ(std::numeric_limits<qreal>::max());

        connect(parent, &QQuickItem::widthChanged, this, &BorderItem::updateGeometry);
        if (auto win = window()) {
            connect(win, &QQuickWindow::activeChanged, this, &BorderItem::updateGeometry);
            connect(win, &QQuickWindow::screenChanged, this, &BorderItem::updateGeometry);
            // Backend selection may not be final before the first scene graph is initialized.
            connect(win, &QQuickWindow::sceneGraphInitialized, this,
                    &BorderItem::updateGeometry, Qt::QueuedConnection);
            connect(win, &QQuickWindow::afterSynchronizing, this,
                    &BorderItem::afterSynchronizing, Qt::DirectConnection);
        }

        if (context->windowId()) {
            setupNecessaryAttributes();
        }
        updateGeometry();
    }

    BorderItem::~BorderItem() {
        // Stop render callbacks before destroying members and unregistering context filters.
        if (auto win = window()) {
            disconnect(win, nullptr, this, nullptr);
        }
        // Qt owns the node returned by updatePaintNode and releases it on the render thread.
    }

    void BorderItem::updateGeometry() {
        auto win = window();
        if (!win) {
            nativeWindowId = 0;
            setVisible(false);
            return;
        }

        nativePainting = shouldEnableNativePainter();
        nativeWindowId = ctx->windowId();
        borderColor = {};
        if (!nativePainting) {
            ctx->virtual_hook(AbstractWindowContext::Windows10BorderColorHook, &borderColor);
        }

        setX(0);
        setWidth(parentItem() ? parentItem()->width() : 0);
        if (nativePainting) {
            // Preserve the original native workaround's surface geometry and opacity setup.
            setY(-0.49);
            setHeight(0.5);
            if (!nativeSurface) {
                nativeSurface = std::make_unique<NativeBorderSurface>(this);
            }
            nativeSurface->setSize(size());
            nativeSurface->update();
        } else {
            nativeSurface.reset();
            setY(0);
            const qreal dpr = win->effectiveDevicePixelRatio();
            setHeight(dpr > 0 ? 1 / dpr : 1);
        }
        setVisible(!(win->windowStates() &
                     (Qt::WindowMinimized | Qt::WindowMaximized | Qt::WindowFullScreen)));
        update();
    }

    QSGNode *BorderItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) {
        if (nativePainting || !borderColor.isValid() || width() <= 0 || height() <= 0) {
            delete oldNode;
            return nullptr;
        }
        auto node = static_cast<QSGRectangleNode *>(oldNode);
        if (!node) {
            node = window()->createRectangleNode();
            if (!node)
                return nullptr;
        }
        node->setRect(QRectF(0, 0, width(), height()));
        node->setColor(borderColor);
        return node;
    }

    void BorderItem::itemChange(ItemChange change, const ItemChangeData &data) {
        QQuickItem::itemChange(change, data);
        if (change == ItemDevicePixelRatioHasChanged || change == ItemSceneChange) {
            updateGeometry();
        }
    }

    bool BorderItem::sharedEventFilter(QObject *obj, QEvent *event) {
        const QPointer<BorderItem> guard(this);
        const bool filtered = Windows10BorderHandler::sharedEventFilter(obj, event);
        if (guard && (event->type() == QEvent::WindowStateChange || event->type() == QEvent::WinIdChange)) {
            updateGeometry();
        }
        return filtered;
    }

    bool BorderItem::nativeEventFilter(const QByteArray &eventType, void *message,
                                       QT_NATIVE_EVENT_RESULT_TYPE *result) {
        if (!message)
            return false;
        const QPointer<BorderItem> guard(this);
        const bool filtered = Windows10BorderHandler::nativeEventFilter(eventType, message, result);
        if (!guard)
            return filtered;
        const auto msg = static_cast<const MSG *>(message);
        switch (msg->message) {
            case WM_THEMECHANGED:
            case WM_SYSCOLORCHANGE:
            case WM_DWMCOLORIZATIONCOLORCHANGED:
                updateGeometry();
                break;
            case WM_SETTINGCHANGE:
                if (isImmersiveColorSetChange(msg->wParam, msg->lParam)) {
                    updateGeometry();
                }
                break;
            default:
                break;
        }
        return filtered;
    }

    void BorderItem::afterSynchronizing() {
        // The GUI thread is still blocked here. Paint independently of item dirtiness,
        // retaining the native timing without querying ctx or its host from the render thread.
        if (nativePainting && isVisible()) {
            drawWindows10BorderNative(reinterpret_cast<HWND>(nativeWindowId));
        }
    }

    void QuickWindowAgentPrivate::setupWindows10BorderWorkaround() {
        auto ctx = context.get();
        if (ctx->windowAttribute(QStringLiteral("win10-border-needed")).toBool()) {
            auto window = static_cast<QQuickWindow *>(ctx->host());
            borderItem = new BorderItem(window->contentItem(), ctx);
        }
    }
#endif

}
