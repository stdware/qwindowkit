#ifndef COREWINDOWAGENT_H
#define COREWINDOWAGENT_H

#include <QtCore/QObject>

#include <QWKCore/qwkcoreglobal.h>

namespace QWK {

    class CoreWindowAgentPrivate;

    class QWK_CORE_EXPORT CoreWindowAgent : public QObject {
        Q_OBJECT
        Q_DISABLE_COPY(CoreWindowAgent)
        Q_DECLARE_PRIVATE(CoreWindowAgent)

    public:
        ~CoreWindowAgent() override;

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
        explicit CoreWindowAgent(CoreWindowAgentPrivate &d, QObject *parent = nullptr);

        const std::unique_ptr<CoreWindowAgentPrivate> d_ptr;
    };

}

#endif // COREWINDOWAGENT_H