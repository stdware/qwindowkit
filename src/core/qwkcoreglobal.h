#ifndef QWKCOREGLOBAL_H
#define QWKCOREGLOBAL_H

#include <QtCore/QLoggingCategory>

#ifndef QWK_CORE_EXPORT
#  ifdef QWK_CORE_STATIC
#    define QWK_CORE_EXPORT
#  else
#    ifdef QWK_CORE_LIBRARY
#      define QWK_CORE_EXPORT Q_DECL_EXPORT
#    else
#      define QWK_CORE_EXPORT Q_DECL_IMPORT
#    endif
#  endif
#endif

QWK_CORE_EXPORT Q_DECLARE_LOGGING_CATEGORY(qWindowKitLog)

#define QWK_INFO     qCInfo(qWindowKitLog)
#define QWK_DEBUG    qCDebug(qWindowKitLog)
#define QWK_WARNING  qCWarning(qWindowKitLog)
#define QWK_CRITICAL qCCritical(qWindowKitLog)

#endif // QWKCOREGLOBAL_H
