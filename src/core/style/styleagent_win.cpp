// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "styleagentregistry_p.h"

#include <QWKCore/private/qwkwindowsextra_p.h>
#include <QWKCore/private/nativeeventfilter_p.h>

namespace QWK {

    Q_GLOBAL_STATIC(StyleAgentRegistry, g_styleAgents)

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
        g_styleAgents->notify([] {
            return StyleAgentRegistry::Appearance{getSystemTheme(), getAccentColor()};
        });
    }

    class SystemSettingEventFilter : public AppNativeEventFilter {
    public:
        bool nativeEventFilter(const QByteArray &eventType, void *message,
                               QT_NATIVE_EVENT_RESULT_TYPE *result) override {
            Q_UNUSED(eventType)

            // It has been observed that the pointer that Qt gives us is sometimes null on some
            // machines. We need to guard against it in such scenarios.
            if (!result || !message) {
                return false;
            }

            const auto msg = static_cast<const MSG *>(message);
            bool notify = false;
            switch (msg->message) {
                case WM_THEMECHANGED:
                case WM_SYSCOLORCHANGE:
                case WM_DWMCOLORIZATIONCOLORCHANGED: {
                    notify = true;
                    break;
                }

                case WM_SETTINGCHANGE: {
                    notify = isImmersiveColorSetChange(msg->wParam, msg->lParam);
                    break;
                }

                default:
                    break;
            }

            if (notify) {
                // Notifying emits signals, so the last StyleAgent may well be destroyed from
                // inside the call below, which would ask us to delete ourselves while this very
                // stack frame is still alive. Defer that until we have unwound.
                ++dispatchDepth;
                notifyAllStyleAgents();
                --dispatchDepth;

                if (dispatchDepth == 0 && uninstallPending) {
                    uninstallPending = false;
                    // A new StyleAgent may have been created in the meantime, only leave if
                    // there is still nobody left to notify.
                    if (g_styleAgents->isEmpty()) {
                        uninstall(); // 'this' is deleted here, touch no members afterwards
                    }
                }
            }
            return false;
        }

        static inline SystemSettingEventFilter *instance = nullptr;
        static inline int dispatchDepth = 0;
        static inline bool uninstallPending = false;

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
            if (dispatchDepth > 0) {
                // We are inside our own nativeEventFilter(), deleting the object now would pull
                // the ground from under it. nativeEventFilter() picks this up when it unwinds.
                uninstallPending = true;
                return;
            }
            delete std::exchange(instance, nullptr);
        }
    };

    void StyleAgentPrivate::setupSystemThemeHook() {
        systemTheme = getSystemTheme();
        systemAccentColor = getAccentColor();

        g_styleAgents->insert(this);
        SystemSettingEventFilter::install();
    }

    void StyleAgentPrivate::removeSystemThemeHook() {
        if (!g_styleAgents->remove(this))
            return;

        if (g_styleAgents->isEmpty()) {
            SystemSettingEventFilter::uninstall();
        }
    }

}
