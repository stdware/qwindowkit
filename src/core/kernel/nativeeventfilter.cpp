#include "nativeeventfilter.h"

#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QCoreApplication>

namespace QWK {

    // Avoid adding multiple global native event filters to QGuiApplication
    // in this library.
    class MasterNativeEventFilter : public QAbstractNativeEventFilter {
    public:
        MasterNativeEventFilter() {
            qApp->installNativeEventFilter(this);
        }

        ~MasterNativeEventFilter() override {
            // The base class removes automatically
        }

        bool nativeEventFilter(const QByteArray &eventType, void *message,
                               QT_NATIVE_EVENT_RESULT_TYPE *result) override {
            for (const auto &child : qAsConst(children)) {
                if (child->nativeEventFilter(eventType, message, result)) {
                    return true;
                }
            }
            return false;
        }

        QVector<NativeEventFilter *> children;

        static MasterNativeEventFilter *instance;
    };

    MasterNativeEventFilter *MasterNativeEventFilter::instance = nullptr;

    NativeEventFilter::NativeEventFilter() {
        if (!MasterNativeEventFilter::instance) {
            MasterNativeEventFilter::instance = new MasterNativeEventFilter();
        }
        MasterNativeEventFilter::instance->children.append(this);
    }

    NativeEventFilter::~NativeEventFilter() {
        MasterNativeEventFilter::instance->children.removeOne(this);
        if (MasterNativeEventFilter::instance->children.isEmpty()) {
            delete MasterNativeEventFilter::instance;
            MasterNativeEventFilter::instance = nullptr;
        }
    }

}