#include "quickwindowagent_p.h"

#include <QtQuick/QQuickPaintedItem>
#include <QtQuick/private/qquickitem_p.h>

#include <QWKCore/private/nativeeventfilter_p.h>

namespace QWK {

    class BorderItem : public QQuickPaintedItem, public NativeEventFilter {
    public:
        explicit BorderItem(QQuickItem *parent, AbstractWindowContext *context);
        ~BorderItem() override;

        void updateGeometry();

    public:
        void paint(QPainter *painter) override;
        void itemChange(ItemChange change, const ItemChangeData &data) override;

    protected:
        bool nativeEventFilter(const QByteArray &eventType, void *message,
                               QT_NATIVE_EVENT_RESULT_TYPE *result) override;

        AbstractWindowContext *context;

    private:
        void _q_windowActivityChanged();
    };

    BorderItem::BorderItem(QQuickItem *parent, AbstractWindowContext *context)
        : QQuickPaintedItem(parent), context(context) {
        setAntialiasing(true);   // We needs anti-aliasing to give us better result.
        setFillColor({});        // Will improve the performance a little bit.
        setOpaquePainting(true); // Will also improve the performance, we don't draw
                                 // semi-transparent borders of course.

        auto parentPri = QQuickItemPrivate::get(parent);
        auto anchors = QQuickItemPrivate::get(this)->anchors();
        anchors->setTop(parentPri->top());
        anchors->setLeft(parentPri->left());
        anchors->setRight(parentPri->right());

        setZ(9999); // Make sure our fake border always above everything in the window.

        context->installNativeEventFilter(this);
        connect(window(), &QQuickWindow::activeChanged, this,
                &BorderItem::_q_windowActivityChanged);
        updateGeometry();
    }

    BorderItem::~BorderItem() = default;

    void BorderItem::updateGeometry() {
        setHeight(context->property("borderThickness").toInt());
    }

    void BorderItem::paint(QPainter *painter) {
        QRect rect(QPoint(0, 0), size().toSize());
        QRegion region(rect);
        void *args[] = {
            painter,
            &rect,
            &region,
        };
        context->virtual_hook(AbstractWindowContext::DrawWindows10BorderHook, args);
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

    bool BorderItem::nativeEventFilter(const QByteArray &eventType, void *message,
                                       QT_NATIVE_EVENT_RESULT_TYPE *result) {
        Q_UNUSED(eventType)
        const auto msg = static_cast<const MSG *>(message);
        switch (msg->message) {
            case WM_THEMECHANGED:
            case WM_SYSCOLORCHANGE:
            case WM_DWMCOLORIZATIONCOLORCHANGED: {
                update();
                break;
            }

            case WM_SETTINGCHANGE: {
                if (!msg->wParam && msg->lParam &&
                    std::wcscmp(reinterpret_cast<LPCWSTR>(msg->lParam), L"ImmersiveColorSet") ==
                        0) {
                    update();
                }
                break;
            }

            default:
                break;
        }
        return false;
    }

    void BorderItem::_q_windowActivityChanged() {
        update();
    }

    void QuickWindowAgentPrivate::setupWindows10BorderWorkaround() {
        // Install painting hook
        auto ctx = context.get();
        if (ctx->property("needBorderPainter").toBool()) {
            std::ignore = new BorderItem(hostWindow->contentItem(), ctx);
        }
    }

}
