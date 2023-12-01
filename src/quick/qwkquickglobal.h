#ifndef QWKQUICKGLOBAL_H
#define QWKQUICKGLOBAL_H

#include <QtGlobal>

#ifndef QWK_QUICK_EXPORT
#  ifdef QWK_QUICK_STATIC
#    define QWK_QUICK_EXPORT
#  else
#    ifdef QWK_QUICK_LIBRARY
#      define QWK_QUICK_EXPORT Q_DECL_EXPORT
#    else
#      define QWK_QUICK_EXPORT Q_DECL_IMPORT
#    endif
#  endif
#endif

#endif // QWKQUICKGLOBAL_H
