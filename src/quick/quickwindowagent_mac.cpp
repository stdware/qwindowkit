#include "quickwindowagent_p.h"

namespace QWK {

    class SystemButtonAreaItemHandler : public QObject {
    public:
        SystemButtonAreaItemHandler(QQuickItem *item, AbstractWindowContext *ctx,
                                    QObject *parent = nullptr);
        ~SystemButtonAreaItemHandler() override = default;

        void updateSystemButtonArea();

    protected:
        QQuickItem *item;
        AbstractWindowContext *ctx;
    };

    SystemButtonAreaItemHandler::SystemButtonAreaItemHandler(QQuickItem *item,
                                                             AbstractWindowContext *ctx,
                                                             QObject *parent)
        : QObject(parent), item(item), ctx(ctx) {
        connect(item, &QQuickItem::xChanged, this,
                &SystemButtonAreaItemHandler::updateSystemButtonArea);
        connect(item, &QQuickItem::yChanged, this,
                &SystemButtonAreaItemHandler::updateSystemButtonArea);
        connect(item, &QQuickItem::widthChanged, this,
                &SystemButtonAreaItemHandler::updateSystemButtonArea);
        connect(item, &QQuickItem::heightChanged, this,
                &SystemButtonAreaItemHandler::updateSystemButtonArea);
        updateSystemButtonArea();
    }

    void SystemButtonAreaItemHandler::updateSystemButtonArea() {
        ctx->setSystemButtonArea(QRectF(item->mapToScene(QPointF(0, 0)), item->size()).toRect());
    }

    QQuickItem *QuickWindowAgent::systemButtonArea() const {
        Q_D(const QuickWindowAgent);
        return d->systemButtonAreaItem;
    }

    void QuickWindowAgent::setSystemButtonArea(QQuickItem *item) {
        Q_D(QuickWindowAgent);
        if (d->systemButtonAreaItem == item)
            return;

        auto ctx = d->context.get();
        d->systemButtonAreaItem = item;
        if (!item) {
            d->systemButtonAreaItemHandler.reset();
            ctx->setSystemButtonArea({});
            return;
        }
        d->systemButtonAreaItemHandler = std::make_unique<SystemButtonAreaItemHandler>(item, ctx);
    }

}
