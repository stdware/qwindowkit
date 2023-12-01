#ifndef QWINDOWKIT_WINDOWS_H
#define QWINDOWKIT_WINDOWS_H

#include <windows.h>

#include <QString>

#include <QWKCore/qwkcoreglobal.h>

namespace QWK {

    QWK_CORE_EXPORT QString winErrorMessage(DWORD code);

    QWK_CORE_EXPORT QString winLastErrorMessage();

}

#endif // QWINDOWKIT_WINDOWS_H
