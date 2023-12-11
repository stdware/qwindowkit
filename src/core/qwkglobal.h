#ifndef QWKGLOBAL_H
#define QWKGLOBAL_H

#include <QtCore/QEvent>
#include <QtGui/QtEvents>

#include <QWKCore/qwkcoreglobal.h>

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
using QT_NATIVE_EVENT_RESULT_TYPE = qintptr;
using QT_ENTER_EVENT_TYPE = QEnterEvent;
#else
using QT_NATIVE_EVENT_RESULT_TYPE = long;
using QT_ENTER_EVENT_TYPE = QEvent;
#endif

#endif // QWKGLOBAL_H
