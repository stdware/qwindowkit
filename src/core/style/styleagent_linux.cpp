// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "styleagent_p.h"

#include "portalsettings_p.h"

namespace QWK {
    void StyleAgentPrivate::setupSystemThemeHook() {
        systemThemeHook.reset(new PortalStyleObserver(this, createDBusPortalSettingsSource()));
    }

    void StyleAgentPrivate::removeSystemThemeHook() {
        systemThemeHook.reset();
    }
}
