#include "quickwindowagent.h"
#include "quickwindowagent_p.h"
#include "quickitemdelegate_p.h"

#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickPaintedItem>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickanchors_p.h>

namespace QWK {

    class BorderItem : public QQuickPaintedItem {
        Q_OBJECT

    public:
        explicit BorderItem(AbstractWindowContext *ctx, QQuickItem *parent = nullptr);
        ~BorderItem() override;

        void updateHeight();

        void paint(QPainter *painter) override;

        void itemChange(ItemChange change, const ItemChangeData &data) override;

    private:
        AbstractWindowContext *context;
    };

    BorderItem::BorderItem(AbstractWindowContext *ctx, QQuickItem *parent) : QQuickPaintedItem(parent), context(ctx) {
        setAntialiasing(true); // ### FIXME: do we need to enable or disable this?
        setMipmap(true); // ### FIXME: do we need to enable or disable this?
        setFillColor({}); // Will improve the performance a little bit.
        setOpaquePainting(true); // Will also improve the performance, we don't draw semi-transparent borders of course.

        auto parentPri = QQuickItemPrivate::get(parent);
        auto anchors = QQuickItemPrivate::get(this)->anchors();
        anchors->setTop(parentPri->top());
        anchors->setLeft(parentPri->left());
        anchors->setRight(parentPri->right());

        setZ(std::numeric_limits<qreal>::max());
    }

    BorderItem::~BorderItem() = default;

    void BorderItem::updateHeight() {
        bool native = false;
        quint32 thickness = 0;
        void *args[] = { &native, &thickness };
        context->virtual_hook(AbstractWindowContext::QueryBorderThicknessHook, &args);
        setHeight(thickness);
    }

    void BorderItem::paint(QPainter *painter) {
        auto rect = QRect{ QPoint{ 0, 0}, size().toSize() };
        auto region = QRegion{ rect };
        void *args[] = {
            painter,
            &rect,
            &region
        };
        context->virtual_hook(AbstractWindowContext::DrawBordersHook, args);
    }

    void BorderItem::itemChange(ItemChange change, const ItemChangeData &data) {
        QQuickPaintedItem::itemChange(change, data);
        switch (change) {
            case ItemSceneChange:
                if (data.window) {
                    connect(data.window, &QQuickWindow::activeChanged, this, [this](){ update(); });
                }
                Q_FALLTHROUGH();
            case ItemVisibleHasChanged:
            case ItemDevicePixelRatioHasChanged:
                updateHeight();
                break;
            default:
                break;
        }
    }

    QuickWindowAgentPrivate::QuickWindowAgentPrivate() {
    }

    QuickWindowAgentPrivate::~QuickWindowAgentPrivate() {
    }

    void QuickWindowAgentPrivate::init() {
        borderItem = std::make_unique<BorderItem>(context.get(), hostWindow->contentItem());
    }

    QuickWindowAgent::QuickWindowAgent(QObject *parent)
        : QuickWindowAgent(*new QuickWindowAgentPrivate(), parent) {
    }

    QuickWindowAgent::~QuickWindowAgent() {
    }

    bool QuickWindowAgent::setup(QQuickWindow *window) {
        Q_ASSERT(window);
        if (!window) {
            return false;
        }

        Q_D(QuickWindowAgent);
        if (d->hostWindow) {
            return false;
        }

        if (!d->setup(window, new QuickItemDelegate())) {
            return false;
        }
        d->hostWindow = window;
        return true;
    }

    QQuickItem *QuickWindowAgent::titleBar() const {
        Q_D(const QuickWindowAgent);
        return static_cast<QQuickItem *>(d->context->titleBar());
    }

    void QuickWindowAgent::setTitleBar(QQuickItem *item) {
        Q_D(QuickWindowAgent);
        if (!d->context->setTitleBar(item)) {
            return;
        }
        Q_EMIT titleBarWidgetChanged(item);
    }

    QQuickItem *QuickWindowAgent::systemButton(SystemButton button) const {
        Q_D(const QuickWindowAgent);
        return static_cast<QQuickItem *>(d->context->systemButton(button));
    }

    void QuickWindowAgent::setSystemButton(SystemButton button, QQuickItem *item) {
        Q_D(QuickWindowAgent);
        if (!d->context->setSystemButton(button, item)) {
            return;
        }
        Q_EMIT systemButtonChanged(button, item);
    }

    bool QuickWindowAgent::isHitTestVisible(const QQuickItem *item) const {
        Q_D(const QuickWindowAgent);
        return d->context->isHitTestVisible(item);
    }

    void QuickWindowAgent::setHitTestVisible_item(const QQuickItem *item, bool visible) {
        Q_D(QuickWindowAgent);
        d->context->setHitTestVisible(item, visible);
    }

    void QuickWindowAgent::setHitTestVisible_rect(const QRect &rect, bool visible) {
        Q_D(QuickWindowAgent);
        d->context->setHitTestVisible(rect, visible);
    }

    QuickWindowAgent::QuickWindowAgent(QuickWindowAgentPrivate &d, QObject *parent)
        : WindowAgentBase(d, parent) {
        d.init();
    }

}

#include "quickwindowagent.moc"