// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "styleagent_p.h"

#include <Cocoa/Cocoa.h>

#include <QtCore/QVariant>

namespace QWK {

    static StyleAgent::SystemTheme getSystemTheme() {
        NSString *osxMode =
            [[NSUserDefaults standardUserDefaults] stringForKey:@"AppleInterfaceStyle"];
        bool isDark = [osxMode isEqualToString:@"Dark"];
        return isDark ? StyleAgent::Dark : StyleAgent::Light;
    }

    static void notifyAllStyleAgents();

}

//
// Objective C++ Begin
//

@interface QWK_SystemThemeObserver : NSObject {
}
@end

@implementation QWK_SystemThemeObserver

- (id)init {
    self = [super init];
    if (self) {
        [[NSDistributedNotificationCenter defaultCenter]
            addObserver:self
               selector:@selector(interfaceModeChanged:)
                   name:@"AppleInterfaceThemeChangedNotification"
                 object:nil];
    }
    return self;
}

- (void)dealloc {
    [[NSDistributedNotificationCenter defaultCenter] removeObserver:self];
    [super dealloc];
}

- (void)interfaceModeChanged:(NSNotification *)notification {
    QWK::notifyAllStyleAgents();
}

@end

//
// Objective C++ End
//


namespace QWK {

    using StyleAgentSet = QSet<StyleAgentPrivate *>;
    Q_GLOBAL_STATIC(StyleAgentSet, g_styleAgentSet)

    static QWK_SystemThemeObserver *g_systemThemeObserver = nil;

    void notifyAllStyleAgents() {
        auto theme = getSystemTheme();
        for (auto &&ap : std::as_const(*g_styleAgentSet())) {
            ap->notifyThemeChanged(theme);
        }
    }

    void StyleAgentPrivate::setupSystemThemeHook() {
        systemTheme = getSystemTheme();

        // Alloc
        if (g_styleAgentSet->isEmpty()) {
            g_systemThemeObserver = [[QWK_SystemThemeObserver alloc] init];
        }

        g_styleAgentSet->insert(this);
    }

    void StyleAgentPrivate::removeSystemThemeHook() {
        if (!g_styleAgentSet->remove(this))
            return;

        if (g_styleAgentSet->isEmpty()) {
            // Delete
            [g_systemThemeObserver release];
            g_systemThemeObserver = nil;
        }
    }

}