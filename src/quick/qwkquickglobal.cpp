// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "qwkquickglobal.h"

#include <QtQml/QQmlEngine>

#include "quickwindowagent.h"

namespace QWK {

    static constexpr const char kModuleUri[] = "QWindowKit";

    void registerTypes(QQmlEngine *engine) {
        Q_UNUSED(engine);

        static bool once = false;
        if (once) {
            return;
        }
        once = true;

        // @uri QWindowKit
        qmlRegisterType<QuickWindowAgent>(kModuleUri, 1, 0, "WindowAgent");
        qmlRegisterModule(kModuleUri, 1, 0);
    }

}
