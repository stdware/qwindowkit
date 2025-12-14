// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "styleagent_p.h"

#include <QtGui/QPalette>
#include <QtGui/QGuiApplication>

namespace QWK {

    void StyleAgentPrivate::setupSystemThemeHook() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        systemAccentColor = QGuiApplication::palette().color(QPalette::Accent);
#else
        systemAccentColor = QGuiApplication::palette().color(QPalette::Highlight);
#endif
    }

    void StyleAgentPrivate::removeSystemThemeHook() {
    }

}