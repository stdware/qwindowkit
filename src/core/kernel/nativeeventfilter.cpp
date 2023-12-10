#include "nativeeventfilter.h"

#include <QtCore/QAbstractNativeEventFilter>
#include <QtGui/QGuiApplication>

namespace QWK {

    // Avoid adding multiple global native event filters to QGuiApplication
    // in this library.
    class MasterNativeEventFilter : public QAbstractNativeEventFilter {
    public:
        MasterNativeEventFilter() {
            qApp->installNativeEventFilter(this);
        }

        ~MasterNativeEventFilter() override {
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

        inline void addChild(NativeEventFilter *child) {
            m_children.append(child);
        }

        inline void removeChild(NativeEventFilter *child) {
            m_children.removeOne(child);
        }

        static MasterNativeEventFilter *instance;

    protected:
        QVector<NativeEventFilter *> m_children;
    };

    MasterNativeEventFilter *MasterNativeEventFilter::instance = nullptr;

    NativeEventFilter::NativeEventFilter() {
        if (!MasterNativeEventFilter::instance) {
            MasterNativeEventFilter::instance = new MasterNativeEventFilter();
        }
        MasterNativeEventFilter::instance->addChild(this);
    }

    NativeEventFilter::~NativeEventFilter() {
        MasterNativeEventFilter::instance->removeChild(this);
        if (MasterNativeEventFilter::instance->count() == 0) {
            delete MasterNativeEventFilter::instance;
            MasterNativeEventFilter::instance = nullptr;
        }
    }

}