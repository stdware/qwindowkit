#ifndef STYLEAGENT_H
#define STYLEAGENT_H

#include <memory>

#include <QtCore/QObject>
#include <QtGui/QWindow>

#include <QWKStyleSupport/qwkstylesupportglobal.h>

namespace QWK {

    class StyleAgentPrivate;

    class QWK_STYLESUPPORT_EXPORT StyleAgent : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(StyleAgent)
    public:
        explicit StyleAgent(QObject *parent = nullptr);
        ~StyleAgent() override;

        enum SystemTheme {
            Unknown,
            Light,
            Dark,
            HighContrast,
        };
        Q_ENUM(SystemTheme)

    public:
        SystemTheme systemTheme() const;

        QVariant windowAttribute(QWindow *window, const QString &key) const;
        bool setWindowAttribute(QWindow *window, const QString &key, const QVariant &attribute);

    Q_SIGNALS:
        void systemThemeChanged(); // Do we need wallpaper change notify?

    protected:
        StyleAgent(StyleAgentPrivate &d, QObject *parent = nullptr);

        const std::unique_ptr<StyleAgentPrivate> d_ptr;
    };

}

#endif // STYLEAGENT_H