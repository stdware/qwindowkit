// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "qwkquickglobal.h"

#include <QtQml/QQmlEngine>

#include "quickwindowagent.h"

#ifdef QWK_QUICK_HAS_QML_MODULE
void qml_register_types_QWindowKit();
#endif

namespace QWK {

    void registerTypes(QQmlEngine *engine) {
        Q_UNUSED(engine);

        static bool once = false;
        if (once) {
            return;
        }
        once = true;

#ifdef QWK_QUICK_HAS_QML_MODULE
        // Explicitly reference the generated registration for static library consumers too.
        qml_register_types_QWindowKit();
#else
        static constexpr const char kModuleUri[] = "QWindowKit";
        // @uri QWindowKit
        qmlRegisterType<QuickWindowAgent>(kModuleUri, 1, 0, "WindowAgent");
        qmlRegisterModule(kModuleUri, 1, 0);
#endif
    }

}
