// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "quickwindowagent_p.h"

#include <QtQuick/QQuickPaintedItem>
#include <QtQuick/private/qquickitem_p.h>

#include <QWKCore/qwindowkit_windows.h>
#include <QWKCore/private/windows10borderhandler_p.h>

namespace QWK {

    static inline bool isWindows1022H2OrGreater() {
        RTL_OSVERSIONINFOW rovi = Private::GetRealOSVersion();
        return (rovi.dwMajorVersion > 10) ||
               (rovi.dwMajorVersion == 10 &&
                (rovi.dwMinorVersion > 0 || rovi.dwBuildNumber >= 19045));
    }

#if QWINDOWKIT_CONFIG(ENABLE_WINDOWS_SYSTEM_BORDERS)

    class BorderItem : public QQuickPaintedItem, public Windows10BorderHandler {
    public:
        explicit BorderItem(QQuickItem *parent, AbstractWindowContext *context);
        ~BorderItem() override;

        bool shouldEnableEmulatedPainter() const;
        void updateGeometry() override;

    public:
        void paint(QPainter *painter) override;
        void itemChange(ItemChange change, const ItemChangeData &data) override;

    protected:
        bool sharedEventFilter(QObject *obj, QEvent *event) override;
        bool nativeEventFilter(const QByteArray &eventType, void *message,
                               QT_NATIVE_EVENT_RESULT_TYPE *result) override;

    private:
        volatile bool needPaint = false;

        void _q_afterSynchronizing();
        void _q_windowActivityChanged();
    };

    bool BorderItem::shouldEnableEmulatedPainter() const {
#  if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const QQuickWindow* win = window();
        if (!win) {
            return true;
        }
        auto api = win->rendererInterface()->graphicsApi();
        switch (api) {
            case QSGRendererInterface::OpenGL:
                // FIXME: experimental, try to find the exact fixed version.
                return !isWindows1022H2OrGreater();
            case QSGRendererInterface::Direct3D11:
#    if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
            case QSGRendererInterface::Direct3D12:
#    endif
                return false;
            default:
                break;
        }
#  endif
        return true;
    }

    BorderItem::BorderItem(QQuickItem *parent, AbstractWindowContext *context)
        : QQuickPaintedItem(parent), Windows10BorderHandler(context) {
        setAntialiasing(true);   // We need anti-aliasing to give us better result.
        setFillColor({});        // Will improve the performance a little bit.
        setOpaquePainting(true); // Will also improve the performance, we don't draw
                                 // semi-transparent borders of course.

        auto parentPri = QQuickItemPrivate::get(parent);
        auto anchors = QQuickItemPrivate::get(this)->anchors();
        anchors->setTop(parentPri->top());
        anchors->setLeft(parentPri->left());
        anchors->setRight(parentPri->right());

        setZ(std::numeric_limits<qreal>::max()); // Make sure our fake border always above
                                                 // everything in the window.

#  if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        connect(window(), &QQuickWindow::afterSynchronizing, this,
                &BorderItem::_q_afterSynchronizing, Qt::DirectConnection);
#  endif
        connect(window(), &QQuickWindow::activeChanged, this,
                &BorderItem::_q_windowActivityChanged);

        // First update
        if (context->windowId()) {
            setupNecessaryAttributes();
        }
        BorderItem::updateGeometry();
    }

    BorderItem::~BorderItem() = default;

    void BorderItem::updateGeometry() {
        const QQuickWindow* win = window();
        if (!win) {
            return;
        }
        setHeight(borderThickness() / win->effectiveDevicePixelRatio());
        setVisible(isNormalWindow());
    }

    void BorderItem::paint(QPainter *painter) {
        Q_UNUSED(painter)
        if (shouldEnableEmulatedPainter()) {
            drawBorderEmulated(painter, QRect({0, 0}, size().toSize()));
        } else {
            needPaint = true;
        }
    }

    void BorderItem::itemChange(ItemChange change, const ItemChangeData &data) {
        QQuickPaintedItem::itemChange(change, data);
        switch (change) {
            case ItemVisibleHasChanged:
            case ItemDevicePixelRatioHasChanged: {
                updateGeometry();
                break;
            }
            default:
                break;
        }
    }

    bool BorderItem::sharedEventFilter(QObject *obj, QEvent *event) {
        Q_UNUSED(obj)

        switch (event->type()) {
            case QEvent::WindowStateChange: {
                updateGeometry();
            }
            default:
                break;
        }
        return Windows10BorderHandler::sharedEventFilter(obj, event);
    }

    bool BorderItem::nativeEventFilter(const QByteArray &eventType, void *message,
                                       QT_NATIVE_EVENT_RESULT_TYPE *result) {
        const auto msg = static_cast<const MSG *>(message);
        switch (msg->message) {
            case WM_THEMECHANGED:
            case WM_SYSCOLORCHANGE:
            case WM_DWMCOLORIZATIONCOLORCHANGED: {
                update();
                break;
            }

            case WM_SETTINGCHANGE: {
                if (isImmersiveColorSetChange(msg->wParam, msg->lParam)) {
                    update();
                }
                break;
            }

            default:
                break;
        }
        return Windows10BorderHandler::nativeEventFilter(eventType, message, result);
    }

    void BorderItem::_q_afterSynchronizing() {
        if (needPaint) {
            needPaint = false;
            drawBorderNative();
        }
    }

    void BorderItem::_q_windowActivityChanged() {
        update();
    }

    void QuickWindowAgentPrivate::setupWindows10BorderWorkaround() {
        // Install painting hook
        auto ctx = context.get();
        if (ctx->windowAttribute(QStringLiteral("win10-border-needed")).toBool()) {
            std::ignore = new BorderItem(hostWindow->contentItem(), ctx);
        }
    }
#endif

}
