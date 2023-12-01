#include "qwindowkit_windows.h"

namespace QWK {

    QString winErrorMessage(DWORD code) {
        LPWSTR buf = nullptr;
        if (::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                 FORMAT_MESSAGE_IGNORE_INSERTS,
                             nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                             reinterpret_cast<LPWSTR>(&buf), 0, nullptr) == 0) {
            return {};
        }
        const QString &errorText = QString::fromWCharArray(buf).trimmed();
        ::LocalFree(buf);
        return errorText;
    }

    QString winLastErrorMessage() {
        return winErrorMessage(::GetLastError());
    }

}