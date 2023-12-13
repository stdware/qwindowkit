#ifndef WINDOWAGENTBASE_H
#define WINDOWAGENTBASE_H

#include <memory>

#include <QtCore/QObject>

#include <QWKCore/qwkglobal.h>

namespace QWK {

    class WindowAgentBasePrivate;

    class QWK_CORE_EXPORT WindowAgentBase : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(WindowAgentBase)
    public:
        ~WindowAgentBase() override;

        enum SystemButton {
            Unknown,
            WindowIcon,
            Help,
            Minimize,
            Maximize,
            Close,
            NumSystemButton,
        };
        Q_ENUM(SystemButton)

    public Q_SLOTS:
        void showSystemMenu(const QPoint &pos);
        void startSystemMove(const QPoint &pos);
        void startSystemResize(Qt::Edges edges, const QPoint &pos);
        void centralize();
        void raise();

    protected:
        explicit WindowAgentBase(WindowAgentBasePrivate &d, QObject *parent = nullptr);

        const std::unique_ptr<WindowAgentBasePrivate> d_ptr;
    };

}

#endif // WINDOWAGENTBASE_H