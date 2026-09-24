// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "styleagentregistry_p.h"

#include <Cocoa/Cocoa.h>

namespace QWK {

    static StyleAgent::SystemTheme getSystemTheme() {
        NSString *osxMode =
            [[NSUserDefaults standardUserDefaults] stringForKey:@"AppleInterfaceStyle"];
        bool isDark = [osxMode isEqualToString:@"Dark"];
        return isDark ? StyleAgent::Dark : StyleAgent::Light;
    }

    static QColor getAccentColor() {
        if (@available(macOS 10.14, *)) {
            NSColor *color = [NSColor controlAccentColor];
            NSColor *rgbColor = [color colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
            if (rgbColor) {
                return QColor::fromRgbF(rgbColor.redComponent, rgbColor.greenComponent, rgbColor.blueComponent, rgbColor.alphaComponent);
            }
        }
        return {};
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
        NSDistributedNotificationCenter *center = [NSDistributedNotificationCenter defaultCenter];
        [center addObserver:self
               selector:@selector(interfaceModeChanged:)
                   name:@"AppleInterfaceThemeChangedNotification"
                 object:nil];
        [center addObserver:self
               selector:@selector(interfaceModeChanged:)
                   name:@"AppleColorPreferencesChangedNotification"
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

    Q_GLOBAL_STATIC(StyleAgentRegistry, g_styleAgents)

    static QWK_SystemThemeObserver *g_systemThemeObserver = nil;

    void notifyAllStyleAgents() {
        g_styleAgents->notify([] {
            return StyleAgentRegistry::Appearance{getSystemTheme(), getAccentColor()};
        });
    }

    void StyleAgentPrivate::setupSystemThemeHook() {
        systemTheme = getSystemTheme();
        systemAccentColor = getAccentColor();

        // Alloc
        if (g_styleAgents->isEmpty()) {
            g_systemThemeObserver = [[QWK_SystemThemeObserver alloc] init];
        }

        g_styleAgents->insert(this);
    }

    void StyleAgentPrivate::removeSystemThemeHook() {
        if (!g_styleAgents->remove(this))
            return;

        if (g_styleAgents->isEmpty()) {
            // Delete
            [g_systemThemeObserver release];
            g_systemThemeObserver = nil;
        }
    }

}
