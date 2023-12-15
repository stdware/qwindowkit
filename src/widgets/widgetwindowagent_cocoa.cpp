#include "widgetwindowagent_p.h"

namespace QWK {

    class TitleBarEventFilter : public QObject {
        Q_OBJECT

    public:
        explicit TitleBarEventFilter(AbstractWindowContext *context, QObject *parent = nullptr);
        ~TitleBarEventFilter() override;

    protected:
        bool eventFilter(QObject *object, QEvent *event) override;

    private:
        AbstractWindowContext *m_context;
        bool m_leftButtonPressed;
    }

    TitleBarEventFilter::TitleBarEventFilter(QObject *parent) : QObject(parent), m_context(context), m_leftButtonPressed(false) {}

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
        const QPoint scenePos = mouseEvent->windowPos().toPoint();
        const QPoint globalPos = mouseEvent->screenPos().toPoint();
#endif
        if (!m_context->isInTitleBarDraggableArea(scenePos)) {
            return false;
        }
        switch (type) {
            case QEvent::MouseButtonPress: {
                if (me->button() == Qt::LeftButton) {
                    m_leftButtonPressed = true;
                    event->accept();
                    return true;
                }
                break;
            }
            case QEvent::MouseButtonRelease: {
                if (me->button() == Qt::LeftButton) {
                    m_leftButtonPressed = false;
                    event->accept();
                    return true;
                }
                break;
            }
            case QEvent::MouseMove: {
                if (m_leftButtonPressed) {
                    static_cast<QWidget *>(object)->windowHandle()->startSystemMove();
                    event->accept();
                    return true;
                }
                break;
            }
            case QEvent::MouseButtonDblClick: {
                if (me->button() == Qt::LeftButton) {
                    QWidget *window = static_cast<QWidget *>(object)->window();
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

#include "widgetwindowagent_cocoa.moc"
