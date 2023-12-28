#include "quickwindowagent_p.h"

namespace QWK {

    /*!
        Returns the item that acts as the system button area.
    */
    QQuickItem *QuickWindowAgent::systemButtonArea() const {
        Q_D(const QuickWindowAgent);
        return d->systemButtonAreaItem;
    }

    /*!
        Sets the item that acts as the system button area. The system button will be centered in
        its area, it is recommended to place the item in a layout and set a fixed size policy.
    */
    void QuickWindowAgent::setSystemButtonArea(QQuickItem *item) {
        Q_D(QuickWindowAgent);
        if (d->systemButtonAreaItem == item)
            return;

        auto ctx = d->context.get();
        d->systemButtonAreaItem = item;
        if (!item) {
            disconnect(systemButtonAreaItemXChangedConnection);
            disconnect(systemButtonAreaItemYChangedConnection);
            disconnect(systemButtonAreaItemWidthChangedConnection);
            disconnect(systemButtonAreaItemHeightChangedConnection);
            systemButtonAreaItemXChangedConnection = {};
            systemButtonAreaItemYChangedConnection = {};
            systemButtonAreaItemWidthChangedConnection = {};
            systemButtonAreaItemHeightChangedConnection = {};
            ctx->setSystemButtonArea({});
            return;
        }
        const auto updateSystemButtonArea = [ctx, item]() -> void {
            ctx->setSystemButtonArea(QRectF(item->mapToScene(QPointF(0, 0)), item->size()).toRect());
        };
        systemButtonAreaItemXChangedConnection = connect(item, &QQuickItem::xChanged, this, updateSystemButtonArea);
        systemButtonAreaItemYChangedConnection = connect(item, &QQuickItem::yChanged, this, updateSystemButtonArea);
        systemButtonAreaItemWidthChangedConnection = connect(item, &QQuickItem::widthChanged, this, updateSystemButtonArea);
        systemButtonAreaItemHeightChangedConnection = connect(item, &QQuickItem::heightChanged, this, updateSystemButtonArea);
        updateSystemButtonArea();
    }

}
