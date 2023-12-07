#include "widgetitemdelegate_p.h"

#include <QtGui/QMouseEvent>
#include <QtWidgets/QWidget>
#include <QtWidgets/QApplication>

extern Q_WIDGETS_EXPORT QWidget *qt_button_down;

namespace QWK {

    WidgetItemDelegate::WidgetItemDelegate() = default;

    WidgetItemDelegate::~WidgetItemDelegate() = default;

    QWindow *WidgetItemDelegate::window(const QObject *obj) const {
        return static_cast<const QWidget *>(obj)->windowHandle();
    }

    bool WidgetItemDelegate::isEnabled(const QObject *obj) const {
        return static_cast<const QWidget *>(obj)->isEnabled();
    }

    bool WidgetItemDelegate::isVisible(const QObject *obj) const {
        return static_cast<const QWidget *>(obj)->isVisible();
    }

    QRect WidgetItemDelegate::mapGeometryToScene(const QObject *obj) const {
        auto widget = static_cast<const QWidget *>(obj);
        const QPoint originPoint = widget->mapTo(widget->window(), QPoint(0, 0));
        const QSize size = widget->size();
        return {originPoint, size};
    }

    QWindow *WidgetItemDelegate::hostWindow(const QObject *host) const {
        return static_cast<const QWidget *>(host)->windowHandle();
    }

    bool WidgetItemDelegate::isHostSizeFixed(const QObject *host) const {
        const auto widget = static_cast<const QWidget *>(host);
        // "Qt::MSWindowsFixedSizeDialogHint" is used cross-platform actually.
        if (widget->windowFlags() & Qt::MSWindowsFixedSizeDialogHint) {
            return true;
        }
        // Caused by setFixedWidth/Height/Size().
        const QSize minSize = widget->minimumSize();
        const QSize maxSize = widget->maximumSize();
        if (!minSize.isEmpty() && !maxSize.isEmpty() && (minSize == maxSize)) {
            return true;
        }
        // Usually set by the user.
        const QSizePolicy sizePolicy = widget->sizePolicy();
        if ((sizePolicy.horizontalPolicy() == QSizePolicy::Fixed)
            && (sizePolicy.verticalPolicy() == QSizePolicy::Fixed)) {
            return true;
        }
        return false;
    }

    void WidgetItemDelegate::resetQtGrabbedControl() const {
        if (!qt_button_down) {
            return;
        }
        static constexpr const auto invalidPos = QPoint{std::numeric_limits<int>::lowest(), std::numeric_limits<int>::lowest()};
        const auto event =
            new QMouseEvent(QEvent::MouseButtonRelease, invalidPos, invalidPos, invalidPos,
                            Qt::LeftButton, QGuiApplication::mouseButtons() ^ Qt::LeftButton,
                            QGuiApplication::keyboardModifiers());
        QApplication::postEvent(qt_button_down, event);
        qt_button_down = nullptr;
    }

}