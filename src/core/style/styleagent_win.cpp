#include "styleagent_p.h"

#include <QtCore/QSet>
#include <QtCore/QVariant>
#include <QtGui/QColor>

#include <QWKCore/private/qwkwindowsextra_p.h>
#include <QWKCore/private/nativeeventfilter_p.h>

namespace QWK {

    using StyleAgentSet = QSet<StyleAgentPrivate *>;
    Q_GLOBAL_STATIC(StyleAgentSet, g_styleAgentSet)

    class SystemSettingEventFilter : public AppNativeEventFilter {
    public:
        bool nativeEventFilter(const QByteArray &eventType, void *message,
                               QT_NATIVE_EVENT_RESULT_TYPE *result) override {
            Q_UNUSED(eventType)
            if (!result) {
                return false;
            }

            auto themeChanged = [message]() -> bool {
                const auto msg = static_cast<const MSG *>(message);
                switch (msg->message) {
                    case WM_THEMECHANGED:
                    case WM_SYSCOLORCHANGE:
                    case WM_DWMCOLORIZATIONCOLORCHANGED:
                        return true;

                    case WM_SETTINGCHANGE: {
                        if (!msg->wParam && msg->lParam &&
                            std::wcscmp(reinterpret_cast<LPCWSTR>(msg->lParam), L"ImmersiveColorSet") ==
                            0) {
                            return true;
                        }
                        break;
                    }

                    default:
                        break;
                }
                return false;
            }();

            if (themeChanged) {
                auto theme = []() -> StyleAgent::SystemTheme {
                    if (isHighContrastModeEnabled()) {
                        return StyleAgent::HighContrast;
                    } else if (isDarkThemeActive()) {
                        return StyleAgent::Dark;
                    } else {
                        return StyleAgent::Light;
                    }
                }();
                for (auto &&ap : std::as_const(*g_styleAgentSet())) {
                    if (ap->systemTheme != theme) {
                        ap->systemTheme = theme;
                        ap->notifyThemeChanged(theme);
                    }
                }
            }

            return false;
        }

        static SystemSettingEventFilter *instance;

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

    SystemSettingEventFilter *SystemSettingEventFilter::instance = nullptr;

    void StyleAgentPrivate::init() {
        if (isHighContrastModeEnabled()) {
            systemTheme = StyleAgent::HighContrast;
        } else if (isDarkThemeActive()) {
            systemTheme = StyleAgent::Dark;
        } else {
            systemTheme = StyleAgent::Light;
        }
    }

    void StyleAgentPrivate::setupSystemThemeHook() {
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