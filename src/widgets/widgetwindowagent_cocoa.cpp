#include "widgetwindowagent_p.h"

#include <QtCore/QDebug>

namespace QWK {

    class TitleBarEventFilter : public QObject {
    public:
        explicit TitleBarEventFilter(AbstractWindowContext *context, QObject *parent = nullptr);
        ~TitleBarEventFilter() override;

    protected:
        bool eventFilter(QObject *object, QEvent *event) override;

    private:
        AbstractWindowContext *m_context;
        bool m_leftButtonPressed;
        bool m_moving;
    };

    TitleBarEventFilter::TitleBarEventFilter(AbstractWindowContext *context, QObject *parent)
        : QObject(parent), m_context(context), m_leftButtonPressed(false), m_moving(false) {
    }

    TitleBarEventFilter::~TitleBarEventFilter() = default;

    bool TitleBarEventFilter::eventFilter(QObject *object, QEvent *event) {
        const auto type = event->type();
        if (type < QEvent::MouseButtonPress || type > QEvent::MouseMove) {
            return false;
        }
        const auto me = static_cast<const QMouseEvent *>(event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const QPoint scenePos = mouseEvent->scenePosition().toPoint();
        const QPoint globalPos = mouseEvent->globalPosition().toPoint();
#else
        const QPoint scenePos = me->windowPos().toPoint();
        const QPoint globalPos = me->screenPos().toPoint();
#endif
        if (!m_context->isInTitleBarDraggableArea(scenePos)) {
            return false;
        }
        switch (type) {
            case QEvent::MouseButtonPress: {
                if (me->button() == Qt::LeftButton) {
                    m_leftButtonPressed = true;
                    m_moving = false;
                    event->accept();
                    return true;
                }
                break;
            }
            case QEvent::MouseButtonRelease: {
                if (me->button() == Qt::LeftButton) {
                    m_leftButtonPressed = false;
                    m_moving = false;
                    event->accept();
                    return true;
                }
                break;
            }
            case QEvent::MouseMove: {
                if (m_leftButtonPressed) {
                    if (!m_moving) {
                        m_moving = true;
                        m_context->window()->startSystemMove();
                    }
                    event->accept();
                    return true;
                }
                break;
            }
            case QEvent::MouseButtonDblClick: {
                if (me->button() == Qt::LeftButton) {
                    auto window = static_cast<QWidget *>(object)->window();
                    if (!window->isFullScreen()) {
                        if (window->isMaximized()) {
                            window->showNormal();
                        } else {
                            window->showMaximized();
                        }
                        event->accept();
                        return true;
                    }
                }
                break;
            }
            default: {
                Q_UNREACHABLE();
                return false;
            }
        }
        return false;
    }

    void WidgetWindowAgentPrivate::setupMacOSTitleBar(QWidget *titleBar) {
        titleBar->installEventFilter(new TitleBarEventFilter(context.get(), titleBar));
    }

}
