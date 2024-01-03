// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "styleagent_p.h"

#include <QtCore/QSet>
#include <QtCore/QVariant>
#include <QtGui/QColor>

#include <QWKCore/private/qwkwindowsextra_p.h>
#include <QWKCore/private/nativeeventfilter_p.h>

namespace QWK {

    using StyleAgentSet = QSet<StyleAgentPrivate *>;
    Q_GLOBAL_STATIC(StyleAgentSet, g_styleAgentSet)

    static StyleAgent::SystemTheme getSystemTheme() {
        if (isHighContrastModeEnabled()) {
            return StyleAgent::HighContrast;
        } else if (isDarkThemeActive()) {
            return StyleAgent::Dark;
        } else {
            return StyleAgent::Light;
        }
    }

    static void notifyAllStyleAgents() {
        auto theme = getSystemTheme();
        for (auto &&ap : std::as_const(*g_styleAgentSet())) {
            ap->notifyThemeChanged(theme);
        }
    }

    class SystemSettingEventFilter : public AppNativeEventFilter {
    public:
        bool nativeEventFilter(const QByteArray &eventType, void *message,
                               QT_NATIVE_EVENT_RESULT_TYPE *result) override {
            Q_UNUSED(eventType)
            if (!result) {
                return false;
            }

            const auto msg = static_cast<const MSG *>(message);
            switch (msg->message) {
                case WM_THEMECHANGED:
                case WM_SYSCOLORCHANGE:
                case WM_DWMCOLORIZATIONCOLORCHANGED: {
                    notifyAllStyleAgents();
                    break;
                }

                case WM_SETTINGCHANGE: {
                    if (isImmersiveColorSetChange(msg->wParam, msg->lParam)) {
                        notifyAllStyleAgents();
                    }
                    break;
                }

                default:
                    break;
            }
            return false;
        }

        static inline SystemSettingEventFilter *instance = nullptr;

        static inline void install() {
            if (instance) {
                return;
            }
            instance = new SystemSettingEventFilter();
        }

        static inline void uninstall() {
            if (!instance) {
                return;
            }
            delete instance;
            instance = nullptr;
        }
    };

    void StyleAgentPrivate::setupSystemThemeHook() {
        systemTheme = getSystemTheme();

        g_styleAgentSet->insert(this);
        SystemSettingEventFilter::install();
    }

    void StyleAgentPrivate::removeSystemThemeHook() {
        if (!g_styleAgentSet->remove(this))
            return;

        if (g_styleAgentSet->isEmpty()) {
            SystemSettingEventFilter::uninstall();
        }
    }

}