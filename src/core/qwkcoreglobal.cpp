#include "qwkcoreglobal_p.h"

#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QCoreApplication>
#include <QtCore/QSet>

namespace QWK {

    // Avoid adding multiple global native event filters to QCoreApplication
    // in this library.
    class NativeEventFilterManager : public QAbstractNativeEventFilter {
    public:
        NativeEventFilterManager() {
            qApp->installNativeEventFilter(this);
        }

        ~NativeEventFilterManager() override {
            qApp->removeNativeEventFilter(this);
        }

        bool nativeEventFilter(const QByteArray &eventType, void *message,
                               QT_NATIVE_EVENT_RESULT_TYPE *result) override {
            for (const auto &child : qAsConst(m_children)) {
                if (child->nativeEventFilter(eventType, message, result)) {
                    return true;
                }
            }
            return false;
        }

        inline int count() const {
            return m_children.size();
        }

        inline void addChild(QAbstractNativeEventFilter *child) {
            m_children.append(child);
        }

        inline void removeChild(QAbstractNativeEventFilter *child) {
            m_children.removeOne(child);
        }

        static NativeEventFilterManager *instance;

    protected:
        QVector<QAbstractNativeEventFilter *> m_children;
    };

    NativeEventFilterManager *NativeEventFilterManager::instance = nullptr;

    void installNativeEventFilter(QAbstractNativeEventFilter *eventFilter) {
        if (!eventFilter) {
            return;
        }

        if (!NativeEventFilterManager::instance) {
            NativeEventFilterManager::instance = new NativeEventFilterManager();
        }
        NativeEventFilterManager::instance->addChild(eventFilter);
    }

    void removeNativeEventFilter(QAbstractNativeEventFilter *eventFilter) {
        if (!eventFilter || !NativeEventFilterManager::instance) {
            return;
        }
        NativeEventFilterManager::instance->removeChild(eventFilter);

        if (NativeEventFilterManager::instance->count() == 0) {
            delete NativeEventFilterManager::instance;
            NativeEventFilterManager::instance = nullptr;
        }
    }

}