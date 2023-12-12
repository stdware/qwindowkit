#include "quickwindowagent.h"
#include "quickwindowagent_p.h"

#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickPaintedItem>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickanchors_p.h>

#ifdef Q_OS_WINDOWS
#  include <QWKCore/private/win10borderhandler_p.h>
#endif

#include "quickitemdelegate_p.h"

namespace QWK {

    class BorderItem : public QQuickPaintedItem, public Win10BorderHandler {
        Q_OBJECT
    public:
        explicit BorderItem(QQuickItem *parent = nullptr);
        ~BorderItem() override;

        void updateGeometry() override;
        void requestUpdate() override;

        bool isActive() const override;

    public:
        void paint(QPainter *painter) override;
    };

    BorderItem::BorderItem(QQuickItem *parent)
        : Win10BorderHandler(parent->window()), QQuickPaintedItem(parent) {
        setAntialiasing(true);   // ### FIXME: do we need to enable or disable this?
        setMipmap(true);         // ### FIXME: do we need to enable or disable this?
        setFillColor({});        // Will improve the performance a little bit.
        setOpaquePainting(true); // Will also improve the performance, we don't draw
                                 // semi-transparent borders of course.

        auto parentPri = QQuickItemPrivate::get(parent);
        auto anchors = QQuickItemPrivate::get(this)->anchors();
        anchors->setTop(parentPri->top());
        anchors->setLeft(parentPri->left());
        anchors->setRight(parentPri->right());

        setZ(10);
    }

    BorderItem::~BorderItem() = default;

    void BorderItem::updateGeometry() {
        setHeight(m_borderThickness);
    }

    void BorderItem::requestUpdate() {
        update();
    }

    bool BorderItem::isActive() const {
        return static_cast<QQuickWindow *>(m_window)->isActive();
    }

    void BorderItem::paint(QPainter *painter) {
        QRect rect(QPoint(0, 0), size().toSize());
        QRegion region(rect);
        Win10BorderHandler::paintBorder(*painter, rect, region);
    }

    QuickWindowAgentPrivate::QuickWindowAgentPrivate() {
    }

    QuickWindowAgentPrivate::~QuickWindowAgentPrivate() {
    }

    void QuickWindowAgentPrivate::init() {
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

#ifdef Q_OS_WINDOWS
        // Install painting hook
        if (bool needPaintBorder;
            QMetaObject::invokeMethod(d->context.get(), "needWin10BorderHandler",
                                      Qt::DirectConnection, Q_RETURN_ARG(bool, needPaintBorder)),
            needPaintBorder) {
            QMetaObject::invokeMethod(
                d->context.get(), "setWin10BorderHandler", Qt::DirectConnection,
                Q_ARG(Win10BorderHandler *, new BorderItem(window->contentItem())));
        }
#endif
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