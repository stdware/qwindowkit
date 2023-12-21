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

            const auto msg = static_cast<const MSG *>(message);
            switch (msg->message) {
                case WM_THEMECHANGED:
                case WM_SYSCOLORCHANGE:
                case WM_DWMCOLORIZATIONCOLORCHANGED: {
                    // TODO: walk through `g_styleAgentSet`
                    break;
                }

                case WM_SETTINGCHANGE: {
                    if (!msg->wParam && msg->lParam &&
                        std::wcscmp(reinterpret_cast<LPCWSTR>(msg->lParam), L"ImmersiveColorSet") ==
                            0) {
                        // TODO: walk through `g_styleAgentSet`
                    }
                    break;
                }

                default:
                    break;
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

    void StyleAgentPrivate::setupSystemThemeHook() {
        g_styleAgentSet->insert(this);
        SystemSettingEventFilter::install();

        // Initialize `systemTheme` variable
    }

    void StyleAgentPrivate::removeSystemThemeHook() {
        if (!g_styleAgentSet->remove(this))
            return;

        if (g_styleAgentSet->isEmpty()) {
            SystemSettingEventFilter::uninstall();
        }
    }

    bool StyleAgentPrivate::updateWindowAttribute(QWindow *window, const QString &key,
                                                  const QVariant &attribute,
                                                  const QVariant &oldAttribute) {
        Q_UNUSED(oldAttribute)

        const auto hwnd = reinterpret_cast<HWND>(window->winId());
        const DynamicApis &apis = DynamicApis::instance();

        if (key == QStringLiteral("mica")) {
            if (!isWin11OrGreater()) {
                return false;
            }
            if (attribute.toBool()) {
                // We need to extend the window frame into the whole client area to be able
                // to see the blurred window background.
                static constexpr const MARGINS margins = {-1, -1, -1, -1};
                apis.pDwmExtendFrameIntoClientArea(hwnd, &margins);
                if (isWin1122H2OrGreater()) {
                    // Use official DWM API to enable Mica, available since Windows 11 22H2
                    // (10.0.22621).
                    const _DWM_SYSTEMBACKDROP_TYPE backdropType = _DWMSBT_MAINWINDOW;
                    apis.pDwmSetWindowAttribute(hwnd, _DWMWA_SYSTEMBACKDROP_TYPE, &backdropType,
                                                sizeof(backdropType));
                } else {
                    // Use undocumented DWM API to enable Mica, available since Windows 11
                    // (10.0.22000).
                    const BOOL enable = TRUE;
                    apis.pDwmSetWindowAttribute(hwnd, _DWMWA_MICA_EFFECT, &enable, sizeof(enable));
                }
            } else {
                if (isWin1122H2OrGreater()) {
                    const _DWM_SYSTEMBACKDROP_TYPE backdropType = _DWMSBT_AUTO;
                    apis.pDwmSetWindowAttribute(hwnd, _DWMWA_SYSTEMBACKDROP_TYPE, &backdropType,
                                                sizeof(backdropType));
                } else {
                    const BOOL enable = FALSE;
                    apis.pDwmSetWindowAttribute(hwnd, _DWMWA_MICA_EFFECT, &enable, sizeof(enable));
                }
                static constexpr const MARGINS margins = {0, 0, 0, 0};
                apis.pDwmExtendFrameIntoClientArea(hwnd, &margins);
            }
            return true;
        } else if (key == QStringLiteral("mica-alt")) {
            if (!isWin1122H2OrGreater()) {
                return false;
            }
            if (attribute.toBool()) {
                // We need to extend the window frame into the whole client area to be able
                // to see the blurred window background.
                static constexpr const MARGINS margins = {-1, -1, -1, -1};
                apis.pDwmExtendFrameIntoClientArea(hwnd, &margins);
                // Use official DWM API to enable Mica Alt, available since Windows 11 22H2
                // (10.0.22621).
                const _DWM_SYSTEMBACKDROP_TYPE backdropType = _DWMSBT_TABBEDWINDOW;
                apis.pDwmSetWindowAttribute(hwnd, _DWMWA_SYSTEMBACKDROP_TYPE, &backdropType,
                                            sizeof(backdropType));
            } else {
                const _DWM_SYSTEMBACKDROP_TYPE backdropType = _DWMSBT_AUTO;
                apis.pDwmSetWindowAttribute(hwnd, _DWMWA_SYSTEMBACKDROP_TYPE, &backdropType,
                                            sizeof(backdropType));
                static constexpr const MARGINS margins = {0, 0, 0, 0};
                apis.pDwmExtendFrameIntoClientArea(hwnd, &margins);
            }
            return true;
        } else if (key == QStringLiteral("acrylic-material")) {
            if (!isWin10OrGreater()) {
                return false;
            }
            if (attribute.userType() == QMetaType::QColor) {
                // We need to extend the window frame into the whole client area to be able
                // to see the blurred window background.
                static constexpr const MARGINS margins = {-1, -1, -1, -1};
                apis.pDwmExtendFrameIntoClientArea(hwnd, &margins);
                if (isWin11OrGreater()) {
                    const _DWM_SYSTEMBACKDROP_TYPE backdropType = _DWMSBT_TRANSIENTWINDOW;
                    apis.pDwmSetWindowAttribute(hwnd, _DWMWA_SYSTEMBACKDROP_TYPE, &backdropType,
                                                sizeof(backdropType));
                } else {
                    auto gradientColor = attribute.value<QColor>();

                    ACCENT_POLICY policy{};
                    policy.dwAccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
                    policy.dwAccentFlags = ACCENT_ENABLE_ACRYLIC_WITH_LUMINOSITY;
                    // This API expects the #AABBGGRR format.
                    policy.dwGradientColor =
                        DWORD(qRgba(gradientColor.blue(), gradientColor.green(),
                                    gradientColor.red(), gradientColor.alpha()));
                    WINDOWCOMPOSITIONATTRIBDATA wcad{};
                    wcad.Attrib = WCA_ACCENT_POLICY;
                    wcad.pvData = &policy;
                    wcad.cbData = sizeof(policy);
                    apis.pSetWindowCompositionAttribute(hwnd, &wcad);
                }
            } else {
                if (isWin11OrGreater()) {
                    const _DWM_SYSTEMBACKDROP_TYPE backdropType = _DWMSBT_AUTO;
                    apis.pDwmSetWindowAttribute(hwnd, _DWMWA_SYSTEMBACKDROP_TYPE, &backdropType,
                                                sizeof(backdropType));
                } else {
                    ACCENT_POLICY policy{};
                    policy.dwAccentState = ACCENT_DISABLED;
                    policy.dwAccentFlags = ACCENT_NONE;
                    WINDOWCOMPOSITIONATTRIBDATA wcad{};
                    wcad.Attrib = WCA_ACCENT_POLICY;
                    wcad.pvData = &policy;
                    wcad.cbData = sizeof(policy);
                    apis.pSetWindowCompositionAttribute(hwnd, &wcad);
                }
                static constexpr const MARGINS margins = {0, 0, 0, 0};
                apis.pDwmExtendFrameIntoClientArea(hwnd, &margins);
            }
            return true;
        } else if (key == QStringLiteral("dwm-blur")) {
            if (attribute.toBool()) {
                // We need to extend the window frame into the whole client area to be able
                // to see the blurred window background.
                static constexpr const MARGINS margins = {-1, -1, -1, -1};
                apis.pDwmExtendFrameIntoClientArea(hwnd, &margins);
                if (isWin8OrGreater()) {
                    ACCENT_POLICY policy{};
                    policy.dwAccentState = ACCENT_ENABLE_BLURBEHIND;
                    policy.dwAccentFlags = ACCENT_NONE;
                    WINDOWCOMPOSITIONATTRIBDATA wcad{};
                    wcad.Attrib = WCA_ACCENT_POLICY;
                    wcad.pvData = &policy;
                    wcad.cbData = sizeof(policy);
                    apis.pSetWindowCompositionAttribute(hwnd, &wcad);
                } else {
                    DWM_BLURBEHIND bb{};
                    bb.fEnable = TRUE;
                    bb.fTransitionOnMaximized = TRUE;
                    bb.dwFlags = DWM_BB_ENABLE | DWM_BB_TRANSITIONONMAXIMIZED;
                    apis.pDwmEnableBlurBehindWindow(hwnd, &bb);
                }
            } else {
                if (isWin8OrGreater()) {
                    ACCENT_POLICY policy{};
                    policy.dwAccentState = ACCENT_DISABLED;
                    policy.dwAccentFlags = ACCENT_NONE;
                    WINDOWCOMPOSITIONATTRIBDATA wcad{};
                    wcad.Attrib = WCA_ACCENT_POLICY;
                    wcad.pvData = &policy;
                    wcad.cbData = sizeof(policy);
                    apis.pSetWindowCompositionAttribute(hwnd, &wcad);
                } else {
                    DWM_BLURBEHIND bb{};
                    bb.fEnable = FALSE;
                    bb.dwFlags = DWM_BB_ENABLE;
                    apis.pDwmEnableBlurBehindWindow(hwnd, &bb);
                }
                static constexpr const MARGINS margins = {0, 0, 0, 0};
                apis.pDwmExtendFrameIntoClientArea(hwnd, &margins);
            }
            return true;
        }
        return false;
    }

}