#ifndef QUICKITEMDELEGATE_P_H
#define QUICKITEMDELEGATE_P_H

#include <QtCore/QObject>
#include <QtGui/QWindow>

#include <QWKCore/windowitemdelegate.h>
#include <QWKQuick/qwkquickglobal.h>

namespace QWK {

    class QWK_QUICK_EXPORT QuickItemDelegate : public WindowItemDelegate {
    public:
        QuickItemDelegate();
        ~QuickItemDelegate() override;

    public:
        QWindow *window(const QObject *obj) const override;
        bool isEnabled(const QObject *obj) const override;
        bool isVisible(const QObject *obj) const override;
        QRect mapGeometryToScene(const QObject *obj) const override;

        QWindow * hostWindow(const QObject *host) const override;
        bool isHostSizeFixed(const QObject *host) const override;
    };

}

#endif // QUICKITEMDELEGATE_P_H
