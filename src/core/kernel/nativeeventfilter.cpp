#include "nativeeventfilter_p.h"

#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QCoreApplication>

namespace QWK {

    NativeEventFilter::NativeEventFilter() : m_dispatcher(nullptr) {
    }

    NativeEventFilter::~NativeEventFilter() {
        if (m_dispatcher)
            m_dispatcher->removeNativeEventFilter(this);
    }

    NativeEventDispatcher::NativeEventDispatcher() = default;

    NativeEventDispatcher::~NativeEventDispatcher() {
        for (const auto &observer : std::as_const(m_nativeEventFilters)) {
            observer->m_dispatcher = nullptr;
        }
    }

    bool NativeEventDispatcher::dispatch(const QByteArray &eventType, void *message,
                                         QT_NATIVE_EVENT_RESULT_TYPE *result) {
        for (const auto &ef : std::as_const(m_nativeEventFilters)) {
            if (ef->nativeEventFilter(eventType, message, result))
                return true;
        }
        return false;
    }

    void NativeEventDispatcher::installNativeEventFilter(NativeEventFilter *filter) {
        if (!filter || filter->m_dispatcher)
            return;

        m_nativeEventFilters.append(filter);
        filter->m_dispatcher = this;
    }

    void NativeEventDispatcher::removeNativeEventFilter(NativeEventFilter *filter) {
        if (!m_nativeEventFilters.removeOne(filter)) {
            return;
        }
        filter->m_dispatcher = nullptr;
    }


    // Avoid adding multiple global native event filters to QGuiApplication
    // in this library.
    class AppMasterNativeEventFilter : public QAbstractNativeEventFilter,
                                       public NativeEventDispatcher {
    public:
        AppMasterNativeEventFilter() {
            qApp->installNativeEventFilter(this);
        }

        // The base class removes automatically
        ~AppMasterNativeEventFilter() override = default;

        bool nativeEventFilter(const QByteArray &eventType, void *message,
                               QT_NATIVE_EVENT_RESULT_TYPE *result) override {
            return dispatch(eventType, message, result);
        }

        static AppMasterNativeEventFilter *instance;

        friend class AppNativeEventFilter;
    };

    AppMasterNativeEventFilter *AppMasterNativeEventFilter::instance = nullptr;

    AppNativeEventFilter::AppNativeEventFilter() {
        if (!AppMasterNativeEventFilter::instance) {
            AppMasterNativeEventFilter::instance = new AppMasterNativeEventFilter();
        }
        AppMasterNativeEventFilter::instance->installNativeEventFilter(this);
    }

    AppNativeEventFilter::~AppNativeEventFilter() {
        AppMasterNativeEventFilter::instance->removeNativeEventFilter(this);
        if (AppMasterNativeEventFilter::instance->m_nativeEventFilters.isEmpty()) {
            delete AppMasterNativeEventFilter::instance;
            AppMasterNativeEventFilter::instance = nullptr;
        }
    }

}