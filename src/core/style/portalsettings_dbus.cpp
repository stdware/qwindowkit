// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include "portalsettings_p.h"
#include <QtCore/QUuid>
#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusMetaType>
#include <QtDBus/QDBusPendingCallWatcher>
#include <QtDBus/QDBusPendingReply>
#include <QtDBus/QDBusServiceWatcher>
#include <QtDBus/QDBusVariant>

namespace QWK {
    static const QString service = QStringLiteral("org.freedesktop.portal.Desktop");
    static const QString path = QStringLiteral("/org/freedesktop/portal/desktop");
    static const QString interface = QStringLiteral("org.freedesktop.portal.Settings");
    static const QString appearance = QStringLiteral("org.freedesktop.appearance");
    using SettingsMap = QMap<QString, QVariantMap>;

    class DBusPortalSettingsSource : public PortalSettingsSource {
        Q_OBJECT
    public:
        DBusPortalSettingsSource()
            : m_name(QStringLiteral("qwk-style-") + QUuid::createUuid().toString()),
              m_serviceWatcher(this) {
            qDBusRegisterMetaType<SettingsMap>();
            m_serviceWatcher.setWatchMode(QDBusServiceWatcher::WatchForOwnerChange);
            m_serviceWatcher.addWatchedService(service);
            connect(&m_serviceWatcher, &QDBusServiceWatcher::serviceOwnerChanged,
                    this, [this](const QString &, const QString &, const QString &) {
                ++m_generation; // Invalidate outstanding replies from the previous owner.
                Q_EMIT unavailable();
            });
        }
        ~DBusPortalSettingsSource() override { QDBusConnection::disconnectFromBus(m_name); }

        void read() override {
            auto bus = QDBusConnection(m_name);
            if (!bus.isConnected()) {
                QDBusConnection::disconnectFromBus(m_name);
                bus = QDBusConnection::connectToBus(QDBusConnection::SessionBus, m_name);
                ++m_generation;
                if (bus.isConnected()) {
                    m_serviceWatcher.setConnection(bus);
                    const bool subscribed = bus.connect(
                        service, path, interface, QStringLiteral("SettingChanged"),
                        this, SLOT(settingChanged(QString,QString,QDBusVariant)));
                    const bool watchingConnection = bus.connect(
                        QString(), QStringLiteral("/org/freedesktop/DBus/Local"),
                        QStringLiteral("org.freedesktop.DBus.Local"),
                        QStringLiteral("Disconnected"), this, SLOT(disconnected()));
                    if (!subscribed || !watchingConnection) {
                        QDBusConnection::disconnectFromBus(m_name);
                        Q_EMIT snapshot({}, false);
                        return;
                    }
                }
            }
            if (!bus.isConnected()) {
                Q_EMIT snapshot({}, false);
                return;
            }
            auto message = QDBusMessage::createMethodCall(service, path, interface,
                                                         QStringLiteral("ReadAll"));
            message << QStringList{appearance};
            const auto generation = m_generation;
            auto watcher = new QDBusPendingCallWatcher(bus.asyncCall(message, 1000), this);
            connect(watcher, &QDBusPendingCallWatcher::finished, this,
                    [this, generation](QDBusPendingCallWatcher *watcher) {
                QDBusPendingReply<SettingsMap> reply = *watcher;
                watcher->deleteLater();
                if (generation != m_generation)
                    return;
                QVariantMap settings;
                if (!reply.isError()) {
                    settings = reply.value().value(appearance);
                    const auto value = settings.value(QStringLiteral("accent-color"));
                    // The portal's accent is a D-Bus (ddd) structure, not a palette color.
                    if (value.userType() == qMetaTypeId<QDBusArgument>()) {
                        const auto argument = qvariant_cast<QDBusArgument>(value);
                        if (argument.currentSignature() == QStringLiteral("(ddd)")) {
                            double red, green, blue;
                            argument.beginStructure();
                            argument >> red >> green >> blue;
                            argument.endStructure();
                            settings.insert(QStringLiteral("accent-color"),
                                            QVariantList{red, green, blue});
                        }
                    }
                }
                Q_EMIT snapshot(settings, !reply.isError());
            });
        }
    private Q_SLOTS:
        void settingChanged(const QString &group, const QString &key, const QDBusVariant &) {
            if (group == appearance && (key == QStringLiteral("color-scheme") ||
                                       key == QStringLiteral("accent-color") ||
                                       key == QStringLiteral("contrast")))
                Q_EMIT changed(); // Read the whole snapshot; do not expose mixed values.
        }
        void disconnected() {
            ++m_generation;
            Q_EMIT unavailable();
        }
    private:
        QString m_name;
        QDBusServiceWatcher m_serviceWatcher;
        quint64 m_generation = 0;
    };

    PortalSettingsSource *createDBusPortalSettingsSource() {
        return new DBusPortalSettingsSource;
    }
}
#include "portalsettings_dbus.moc"
