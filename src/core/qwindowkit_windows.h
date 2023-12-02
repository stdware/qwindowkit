#ifndef QWINDOWKIT_WINDOWS_H
#define QWINDOWKIT_WINDOWS_H

#include <QtCore/qt_windows.h>
#include <QtCore/qglobal.h>

// MOC can't handle C++ attributes before 5.15.
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
#  define Q_NODISCARD    [[nodiscard]]
#  define Q_MAYBE_UNUSED [[maybe_unused]]
#else
#  define Q_NODISCARD
#  define Q_MAYBE_UNUSED
#endif

#ifndef GET_X_LPARAM
#  define GET_X_LPARAM(lp) (static_cast<int>(static_cast<short>(LOWORD(lp))))
#endif

#ifndef GET_Y_LPARAM
#  define GET_Y_LPARAM(lp) (static_cast<int>(static_cast<short>(HIWORD(lp))))
#endif

#endif // QWINDOWKIT_WINDOWS_H
