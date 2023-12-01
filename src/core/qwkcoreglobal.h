#ifndef QWKCOREGLOBAL_H
#define QWKCOREGLOBAL_H

#include <QtGlobal>

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

#endif // QWKCOREGLOBAL_H
