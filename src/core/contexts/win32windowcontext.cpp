#include "win32windowcontext_p.h"
#include "qwkcoreglobal_p.h"

#include <optional>

#include <QtCore/QHash>
#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QOperatingSystemVersion>
#include <QtCore/QScopeGuard>
#include <QtGui/QGuiApplication>

#include <QtCore/private/qsystemlibrary_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  include <QtGui/private/qguiapplication_p.h>
#endif
#include <QtGui/qpa/qplatformwindow.h>
#if QT_VERSION < QT_VERSION_CHECK(6, 2, 0)
#  include <QtGui/qpa/qplatformnativeinterface.h>
#else
#  include <QtGui/qpa/qplatformwindow_p.h>
#endif

#include <shellscalingapi.h>
#include <dwmapi.h>
#include <timeapi.h>

Q_DECLARE_METATYPE(QMargins)

namespace QWK {

    static constexpr const auto kAutoHideTaskBarThickness =
        quint8{2}; // The thickness of an auto-hide taskbar in pixels.

    using WndProcHash = QHash<HWND, Win32WindowContext *>; // hWnd -> context
    Q_GLOBAL_STATIC(WndProcHash, g_wndProcHash)

    static WNDPROC g_qtWindowProc = nullptr; // Original Qt window proc function

    static struct QWK_Hook {
        QWK_Hook() {
            qApp->setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
        }
    } g_hook{};

    struct DynamicApis {
        decltype(&::DwmFlush) pDwmFlush = nullptr;
        decltype(&::DwmIsCompositionEnabled) pDwmIsCompositionEnabled = nullptr;
        decltype(&::DwmGetCompositionTimingInfo) pDwmGetCompositionTimingInfo = nullptr;
        decltype(&::GetDpiForWindow) pGetDpiForWindow = nullptr;
        decltype(&::GetSystemMetricsForDpi) pGetSystemMetricsForDpi = nullptr;
        decltype(&::GetDpiForMonitor) pGetDpiForMonitor = nullptr;
        decltype(&::timeGetDevCaps) ptimeGetDevCaps = nullptr;
        decltype(&::timeBeginPeriod) ptimeBeginPeriod = nullptr;
        decltype(&::timeEndPeriod) ptimeEndPeriod = nullptr;

        DynamicApis() {
            QSystemLibrary user32(QStringLiteral("user32.dll"));
            pGetDpiForWindow =
                reinterpret_cast<decltype(pGetDpiForWindow)>(user32.resolve("GetDpiForWindow"));
            pGetSystemMetricsForDpi = reinterpret_cast<decltype(pGetSystemMetricsForDpi)>(
                user32.resolve("GetSystemMetricsForDpi"));

            QSystemLibrary shcore(QStringLiteral("shcore.dll"));
            pGetDpiForMonitor =
                reinterpret_cast<decltype(pGetDpiForMonitor)>(shcore.resolve("GetDpiForMonitor"));

            QSystemLibrary dwmapi(QStringLiteral("dwmapi.dll"));
            pDwmFlush = reinterpret_cast<decltype(pDwmFlush)>(dwmapi.resolve("DwmFlush"));
            pDwmIsCompositionEnabled = reinterpret_cast<decltype(pDwmIsCompositionEnabled)>(
                dwmapi.resolve("DwmIsCompositionEnabled"));
            pDwmGetCompositionTimingInfo = reinterpret_cast<decltype(pDwmGetCompositionTimingInfo)>(dwmapi.resolve("DwmGetCompositionTimingInfo"));

            QSystemLibrary winmm(QStringLiteral("winmm.dll"));
            ptimeGetDevCaps = reinterpret_cast<decltype(ptimeGetDevCaps)>(winmm.resolve("timeGetDevCaps"));
            ptimeBeginPeriod = reinterpret_cast<decltype(ptimeBeginPeriod)>(winmm.resolve("timeBeginPeriod"));
            ptimeEndPeriod = reinterpret_cast<decltype(ptimeEndPeriod)>(winmm.resolve("timeEndPeriod"));
        }

        ~DynamicApis() = default;

        static const DynamicApis &instance() {
            static const DynamicApis inst{};
            return inst;
        }

    private:
        Q_DISABLE_COPY_MOVE(DynamicApis)
    };

    static inline constexpr bool operator==(const POINT &lhs, const POINT &rhs) noexcept {
        return ((lhs.x == rhs.x) && (lhs.y == rhs.y));
    }

    static inline constexpr bool operator!=(const POINT &lhs, const POINT &rhs) noexcept {
        return !operator==(lhs, rhs);
    }

    static inline constexpr bool operator==(const SIZE &lhs, const SIZE &rhs) noexcept {
        return ((lhs.cx == rhs.cx) && (lhs.cy == rhs.cy));
    }

    static inline constexpr bool operator!=(const SIZE &lhs, const SIZE &rhs) noexcept {
        return !operator==(lhs, rhs);
    }

    static inline constexpr bool operator>(const SIZE &lhs, const SIZE &rhs) noexcept {
        return ((lhs.cx * lhs.cy) > (rhs.cx * rhs.cy));
    }

    static inline constexpr bool operator>=(const SIZE &lhs, const SIZE &rhs) noexcept {
        return (operator>(lhs, rhs) || operator==(lhs, rhs));
    }

    static inline constexpr bool operator<(const SIZE &lhs, const SIZE &rhs) noexcept {
        return (operator!=(lhs, rhs) && !operator>(lhs, rhs));
    }

    static inline constexpr bool operator<=(const SIZE &lhs, const SIZE &rhs) noexcept {
        return (operator<(lhs, rhs) || operator==(lhs, rhs));
    }

    static inline constexpr bool operator==(const RECT &lhs, const RECT &rhs) noexcept {
        return ((lhs.left == rhs.left) && (lhs.top == rhs.top) && (lhs.right == rhs.right) &&
                (lhs.bottom == rhs.bottom));
    }

    static inline constexpr bool operator!=(const RECT &lhs, const RECT &rhs) noexcept {
        return !operator==(lhs, rhs);
    }

    static inline constexpr QPoint point2qpoint(const POINT &point) {
        return QPoint{int(point.x), int(point.y)};
    }

    static inline constexpr POINT qpoint2point(const QPoint &point) {
        return POINT{LONG(point.x()), LONG(point.y())};
    }

    static inline constexpr QSize size2qsize(const SIZE &size) {
        return QSize{int(size.cx), int(size.cy)};
    }

    static inline constexpr SIZE qsize2size(const QSize &size) {
        return SIZE{LONG(size.width()), LONG(size.height())};
    }

    static inline constexpr QRect rect2qrect(const RECT &rect) {
        return QRect{
            QPoint{int(rect.left),        int(rect.top)         },
            QSize{int(RECT_WIDTH(rect)), int(RECT_HEIGHT(rect))}
        };
    }

    static inline constexpr RECT qrect2rect(const QRect &qrect) {
        return RECT{LONG(qrect.left()), LONG(qrect.top()), LONG(qrect.right()),
                    LONG(qrect.bottom())};
    }

    static inline /*constexpr*/ QString hwnd2str(const WId windowId) {
        // NULL handle is allowed here.
        return QLatin1String("0x") +
               QString::number(windowId, 16).toUpper().rightJustified(8, u'0');
    }

    static inline /*constexpr*/ QString hwnd2str(HWND hwnd) {
        // NULL handle is allowed here.
        return hwnd2str(reinterpret_cast<WId>(hwnd));
    }

    static inline bool isWin8OrGreater() {
        static const bool result =
            QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows8;
        return result;
    }

    static inline bool isWin8Point1OrGreater() {
        static const bool result =
            QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows8_1;
        return result;
    }

    static inline bool isWin10OrGreater() {
        static const bool result =
            QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10;
        return result;
    }

    static inline bool isDwmCompositionEnabled() {
        if (isWin8OrGreater()) {
            return true;
        }
        const DynamicApis &apis = DynamicApis::instance();
        if (!apis.pDwmIsCompositionEnabled) {
            return false;
        }
        BOOL enabled = FALSE;
        return SUCCEEDED(apis.pDwmIsCompositionEnabled(&enabled)) && enabled;
    }

    static inline void triggerFrameChange(HWND hwnd) {
        ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                       SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER |
                           SWP_FRAMECHANGED);
    }

    static inline quint32 getDpiForWindow(HWND hwnd) {
        const DynamicApis &apis = DynamicApis::instance();
        if (apis.pGetDpiForWindow) {         // Win10
            return apis.pGetDpiForWindow(hwnd);
        } else if (apis.pGetDpiForMonitor) { // Win8.1
            HMONITOR monitor = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            UINT dpiX{USER_DEFAULT_SCREEN_DPI};
            UINT dpiY{USER_DEFAULT_SCREEN_DPI};
            apis.pGetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
            return dpiX;
        } else { // Win2K
            HDC hdc = ::GetDC(nullptr);
            const int dpiX = ::GetDeviceCaps(hdc, LOGPIXELSX);
            const int dpiY = ::GetDeviceCaps(hdc, LOGPIXELSY);
            ::ReleaseDC(nullptr, hdc);
            return quint32(dpiX);
        }
    }

    static inline quint32 getResizeBorderThickness(HWND hwnd) {
        const DynamicApis &apis = DynamicApis::instance();
        if (apis.pGetSystemMetricsForDpi) {
            const quint32 dpi = getDpiForWindow(hwnd);
            return apis.pGetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi) +
                   apis.pGetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
        } else {
            return ::GetSystemMetrics(SM_CXSIZEFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER);
        }
    }

    static inline quint32 getTitleBarHeight(HWND hwnd) {
        const auto captionHeight = [hwnd]() -> int {
            const DynamicApis &apis = DynamicApis::instance();
            if (apis.pGetSystemMetricsForDpi) {
                const quint32 dpi = getDpiForWindow(hwnd);
                return apis.pGetSystemMetricsForDpi(SM_CYCAPTION, dpi);
            } else {
                return ::GetSystemMetrics(SM_CYCAPTION);
            }
        }();
        return captionHeight + getResizeBorderThickness(hwnd);
    }

    static inline void updateInternalWindowFrameMargins(HWND hwnd, QWindow *window) {
        const auto margins = [hwnd]() -> QMargins {
            const int titleBarHeight = getTitleBarHeight(hwnd);
            if (isWin10OrGreater()) {
                return {0, -titleBarHeight, 0, 0};
            } else {
                const int frameSize = getResizeBorderThickness(hwnd);
                return {-frameSize, -titleBarHeight, -frameSize, -frameSize};
            }
        }();
        const QVariant marginsVar = QVariant::fromValue(margins);
        window->setProperty("_q_windowsCustomMargins", marginsVar);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        if (QPlatformWindow *platformWindow = window->handle()) {
            if (const auto ni = QGuiApplication::platformNativeInterface()) {
                ni->setWindowProperty(platformWindow, QStringLiteral("_q_windowsCustomMargins"),
                                      marginsVar);
            }
        }
#else
        if (const auto platformWindow =
                dynamic_cast<QNativeInterface::Private::QWindowsWindow *>(window->handle())) {
            platformWindow->setCustomMargins(margins);
        }
#endif
    }

    static inline MONITORINFOEXW getMonitorForWindow(HWND hwnd) {
        // Use "MONITOR_DEFAULTTONEAREST" here so that we can still get the correct
        // monitor even if the window is minimized.
        HMONITOR monitor = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFOEXW monitorInfo{};
        monitorInfo.cbSize = sizeof(monitorInfo);
        ::GetMonitorInfoW(monitor, &monitorInfo);
        return monitorInfo;
    };

    static inline void moveToDesktopCenter(HWND hwnd) {
        const auto monitorInfo = getMonitorForWindow(hwnd);
        RECT windowRect{};
        ::GetWindowRect(hwnd, &windowRect);
        const auto newX = (RECT_WIDTH(monitorInfo.rcMonitor) - RECT_WIDTH(windowRect)) / 2;
        const auto newY = (RECT_HEIGHT(monitorInfo.rcMonitor) - RECT_HEIGHT(windowRect)) / 2;
        ::SetWindowPos(hwnd, nullptr, newX, newY, 0, 0,
                       SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
    }

    static inline bool isFullScreen(HWND hwnd) {
        RECT windowRect{};
        ::GetWindowRect(hwnd, &windowRect);
        // Compare to the full area of the screen, not the work area.
        return (windowRect == getMonitorForWindow(hwnd).rcMonitor);
    }

    static inline bool isWindowNoState(HWND hwnd) {
#if 0
        WINDOWPLACEMENT wp{};
        wp.length = sizeof(wp);
        ::GetWindowPlacement(hwnd, &wp);
        return ((wp.showCmd == SW_NORMAL) || (wp.showCmd == SW_RESTORE));
#else
        const auto style = static_cast<DWORD>(::GetWindowLongPtrW(hwnd, GWL_STYLE));
        return (!(style & (WS_MINIMIZE | WS_MAXIMIZE)));
#endif
    }

    static inline void syncPaintEventWithDwm() {
        // No need to sync with DWM if DWM composition is disabled.
        if (!isDwmCompositionEnabled()) {
            return;
        }
        const DynamicApis &apis = DynamicApis::instance();
        // Dirty hack to workaround the resize flicker caused by DWM.
        LARGE_INTEGER freq{};
        ::QueryPerformanceFrequency(&freq);
        TIMECAPS tc{};
        apis.ptimeGetDevCaps(&tc, sizeof(tc));
        const UINT ms_granularity = tc.wPeriodMin;
        apis.ptimeBeginPeriod(ms_granularity);
        LARGE_INTEGER now0{};
        ::QueryPerformanceCounter(&now0);
        // ask DWM where the vertical blank falls
        DWM_TIMING_INFO dti{};
        dti.cbSize = sizeof(dti);
        apis.pDwmGetCompositionTimingInfo(nullptr, &dti);
        LARGE_INTEGER now1{};
        ::QueryPerformanceCounter(&now1);
        // - DWM told us about SOME vertical blank
        //   - past or future, possibly many frames away
        // - convert that into the NEXT vertical blank
        const auto period = qreal(dti.qpcRefreshPeriod);
        const auto dt = qreal(dti.qpcVBlank - now1.QuadPart);
        const qreal ratio = (dt / period);
        auto w = qreal(0);
        auto m = qreal(0);
        if ((dt > qreal(0)) || qFuzzyIsNull(dt)) {
            w = ratio;
        } else {
            // reach back to previous period
            // - so m represents consistent position within phase
            w = (ratio - qreal(1));
        }
        m = (dt - (period * w));
        if ((m < qreal(0)) || qFuzzyCompare(m, period) || (m > period)) {
            return;
        }
        const qreal m_ms = (qreal(1000) * m / qreal(freq.QuadPart));
        ::Sleep(static_cast<DWORD>(std::round(m_ms)));
        apis.ptimeEndPeriod(ms_granularity);
    }

    static inline Win32WindowContext::WindowPart getHitWindowPart(int hitTestResult) {
        switch (hitTestResult) {
            case HTCLIENT:
                return Win32WindowContext::ClientArea;
            case HTCAPTION:
                return Win32WindowContext::TitleBar;
            case HTSYSMENU:
            case HTHELP:
            case HTREDUCE:
            case HTZOOM:
            case HTCLOSE:
                return Win32WindowContext::ChromeButton;
            case HTLEFT:
            case HTRIGHT:
            case HTTOP:
            case HTTOPLEFT:
            case HTTOPRIGHT:
            case HTBOTTOM:
            case HTBOTTOMLEFT:
            case HTBOTTOMRIGHT:
                return Win32WindowContext::ResizeBorder;
            case HTBORDER:
                return Win32WindowContext::FixedBorder;
            default:
                break; // unreachable
        }
        return Win32WindowContext::Outside;
    }

    static bool isValidWindow(HWND hWnd, bool checkVisible, bool checkTopLevel) {
        if (!::IsWindow(hWnd)) {
            return false;
        }
        const LONG_PTR styles = ::GetWindowLongPtrW(hWnd, GWL_STYLE);
        if (styles & WS_DISABLED) {
            return false;
        }
        const LONG_PTR exStyles = ::GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
        if (exStyles & WS_EX_TOOLWINDOW) {
            return false;
        }
        RECT rect{};
        if (!::GetWindowRect(hWnd, &rect)) {
            return false;
        }
        if ((rect.left >= rect.right) || (rect.top >= rect.bottom)) {
            return false;
        }
        if (checkVisible) {
            if (!::IsWindowVisible(hWnd)) {
                return false;
            }
        }
        if (checkTopLevel) {
            if (::GetAncestor(hWnd, GA_ROOT) != hWnd) {
                return false;
            }
        }
        return true;
    }

    // https://github.com/qt/qtbase/blob/e26a87f1ecc40bc8c6aa5b889fce67410a57a702/src/plugins/platforms/windows/qwindowscontext.cpp#L1556
    // In QWindowsContext::windowsProc(), the messages will be passed to all global native event
    // filters, but because we have already filtered the messages in the hook WndProc function for
    // convenience, Qt does not know we may have already processed the messages and thus will call
    // DefWindowProc(). Consequently, we have to add a global native filter that forwards the result
    // of the hook function, telling Qt whether we have filtered the events before. Since Qt only
    // handles Windows window messages in the main thread, it is safe to do so.
    class WindowsNativeEventFilter : public QAbstractNativeEventFilter {
    public:
        bool nativeEventFilter(const QByteArray &eventType, void *message,
                               QT_NATIVE_EVENT_RESULT_TYPE *result) override {
            Q_UNUSED(eventType)

            auto orgLastMessageContext = lastMessageContext;
            lastMessageContext = nullptr;

            // It has been observed that the pointer that Qt gives us is sometimes null on some
            // machines. We need to guard against it in such scenarios.
            if (!result) {
                return false;
            }

            // https://github.com/qt/qtbase/blob/e26a87f1ecc40bc8c6aa5b889fce67410a57a702/src/plugins/platforms/windows/qwindowscontext.cpp#L1546
            // Qt needs to refer to the WM_NCCALCSIZE message data that hasn't been processed, so we
            // have to process it after Qt acquired the initial data.
            auto msg = static_cast<const MSG *>(message);
            if (msg->message == WM_NCCALCSIZE && orgLastMessageContext) {
                LRESULT res;
                if (Win32WindowContext::nonClientCalcSizeHandler(msg->hwnd, msg->message,
                                                                 msg->wParam, msg->lParam, &res)) {
                    *result = decltype(*result)(res);
                    return true;
                }
            }
            return false;
        }

        static WindowsNativeEventFilter *instance;
        static Win32WindowContext *lastMessageContext;

        static inline void install() {
            instance = new WindowsNativeEventFilter();
            installNativeEventFilter(instance);
        }

        static inline void uninstall() {
            removeNativeEventFilter(instance);
            delete instance;
            instance = nullptr;
        }
    };

    WindowsNativeEventFilter *WindowsNativeEventFilter::instance = nullptr;
    Win32WindowContext *WindowsNativeEventFilter::lastMessageContext = nullptr;

    // https://github.com/qt/qtbase/blob/e26a87f1ecc40bc8c6aa5b889fce67410a57a702/src/plugins/platforms/windows/qwindowscontext.cpp#L1025
    // We can see from the source code that Qt will filter out some messages first and then send the
    // unfiltered messages to the event dispatcher. To activate the Snap Layout feature on Windows
    // 11, we must process some non-client area messages ourselves, but unfortunately these messages
    // have been filtered out already in that line, and thus we'll never have the chance to process
    // them ourselves. This is Qt's low level platform specific code, so we don't have any official
    // ways to change this behavior. But luckily we can replace the window procedure function of
    // Qt's windows, and in this hooked window procedure function, we finally have the chance to
    // process window messages before Qt touches them. So we reconstruct the MSG structure and send
    // it to our own custom native event filter to do all the magic works. But since the system menu
    // feature doesn't necessarily belong to the native implementation, we seperate the handling
    // code and always process the system menu part in this function for both implementations.
    //
    // Original event flow:
    //      [Entry]             Windows Message Queue
    //                          |
    //      [Qt Window Proc]    qwindowscontext.cpp#L1547: qWindowsWndProc()
    //                              ```
    //                              const bool handled = QWindowsContext::instance()->windowsProc
    //                                  (hwnd, message, et, wParam, lParam, &result,
    //                                  &platformWindow);
    //                              ```
    //                          |
    //      [Non-Input Filter]  qwindowscontext.cpp#L1025: QWindowsContext::windowsProc()
    //                              ```
    //                              if (!isInputMessage(msg.message) &&
    //                                  filterNativeEvent(&msg, result))
    //                                  return true;
    //                              ```
    //                          |
    //      [User Filter]       qwindowscontext.cpp#L1588: QWindowsContext::windowsProc()
    //                              ```
    //                              QAbstractEventDispatcher *dispatcher =
    //                              QAbstractEventDispatcher::instance();
    //                              qintptr filterResult = 0;
    //                              if (dispatcher &&
    //                              dispatcher->filterNativeEvent(nativeEventType(), msg,
    //                              &filterResult)) {
    //                                  *result = LRESULT(filterResult);
    //                                  return true;
    //                              }
    //                              ```
    //                          |
    //      [Extra work]        The rest of QWindowsContext::windowsProc() and qWindowsWndProc()
    //
    // Notice: Only non-input messages will be processed by the user-defined global native event
    // filter!!! These events are then passed to the widget class's own overridden
    // QWidget::nativeEvent() as a local filter, where all native events can be handled, but we must
    // create a new class derived from QWidget which we don't intend to. Therefore, we don't expect
    // to process events from the global native event filter, but instead hook Qt's window
    // procedure.

    extern "C" LRESULT QT_WIN_CALLBACK QWKHookedWndProc(HWND hWnd, UINT message, WPARAM wParam,
                                                        LPARAM lParam) {
        Q_ASSERT(hWnd);
        if (!hWnd) {
            return FALSE;
        }

        // Search window context
        auto ctx = g_wndProcHash->value(hWnd);
        if (!ctx) {
            return ::DefWindowProcW(hWnd, message, wParam, lParam);
        }

        // Since Qt does the necessary processing of the WM_NCCALCSIZE message, we need to
        // forward it right away and process it in our native event filter.
        if (message == WM_NCCALCSIZE) {
            WindowsNativeEventFilter::lastMessageContext = ctx;
            return ::CallWindowProcW(g_qtWindowProc, hWnd, message, wParam, lParam);
        }

        // Try hooked procedure and save result
        LRESULT result;
        bool handled = ctx->windowProc(hWnd, message, wParam, lParam, &result);

        // TODO: Determine whether to show system menu
        // ...

        if (handled) {
            return result;
        }

        // Continue dispatching.
        WindowsNativeEventFilter::lastMessageContext = ctx;
        return ::CallWindowProcW(g_qtWindowProc, hWnd, message, wParam, lParam);
    }

    Win32WindowContext::Win32WindowContext() : AbstractWindowContext() {
    }

    Win32WindowContext::~Win32WindowContext() {
        // Remove window handle mapping
        if (auto hWnd = reinterpret_cast<HWND>(windowId); hWnd) {
            g_wndProcHash->remove(hWnd);

            // Remove event filter if the all windows has been destroyed
            if (g_wndProcHash->empty()) {
                WindowsNativeEventFilter::uninstall();
            }
        }
    }

    bool Win32WindowContext::setupHost() {
        // Install window hook
        auto winId = m_windowHandle->winId();
        auto hWnd = reinterpret_cast<HWND>(winId);

        // Inform Qt we want and have set custom margins
        updateInternalWindowFrameMargins(hWnd, m_windowHandle);

        // Store original window proc
        if (!g_qtWindowProc) {
            g_qtWindowProc = reinterpret_cast<WNDPROC>(::GetWindowLongPtrW(hWnd, GWLP_WNDPROC));
        }

        // Hook window proc
        ::SetWindowLongPtrW(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(QWKHookedWndProc));

        // Install global native event filter
        if (!WindowsNativeEventFilter::instance) {
            WindowsNativeEventFilter::install();
        }

        // Cache window ID
        windowId = winId;

        // Save window handle mapping
        g_wndProcHash->insert(hWnd, this);

        return true;
    }

    bool Win32WindowContext::windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam,
                                        LRESULT *result) {
        *result = FALSE;

        // We should skip these messages otherwise we will get crashes.
        // NOTE: WM_QUIT won't be posted to the WindowProc function.
        switch (message) {
            case WM_CLOSE:
            case WM_DESTROY:
            case WM_NCDESTROY:
            // Undocumented messages:
            case WM_UAHDESTROYWINDOW:
            case WM_UNREGISTER_WINDOW_SERVICES:
                return false;
            default:
                break;
        }

        if (!isValidWindow(hWnd, false, true)) {
            return false;
        }

        // Test snap layout
        if (snapLayoutHandler(hWnd, message, wParam, lParam, result)) {
            return true;
        }

        // Main implementation
        if (customWindowHandler(hWnd, message, wParam, lParam, result)) {
            return true;
        }

        return false; // Not handled
    }

    static constexpr const auto kMessageTag = WPARAM(0x97CCEA99);

    static inline constexpr bool isTaggedMessage(WPARAM wParam) {
        return (wParam == kMessageTag);
    }

    static inline quint64 getKeyState() {
        quint64 result = 0;
        const auto &get = [](const int virtualKey) -> bool {
            return (::GetAsyncKeyState(virtualKey) < 0);
        };
        const bool buttonSwapped = ::GetSystemMetrics(SM_SWAPBUTTON);
        if (get(VK_LBUTTON)) {
            result |= (buttonSwapped ? MK_RBUTTON : MK_LBUTTON);
        }
        if (get(VK_RBUTTON)) {
            result |= (buttonSwapped ? MK_LBUTTON : MK_RBUTTON);
        }
        if (get(VK_SHIFT)) {
            result |= MK_SHIFT;
        }
        if (get(VK_CONTROL)) {
            result |= MK_CONTROL;
        }
        if (get(VK_MBUTTON)) {
            result |= MK_MBUTTON;
        }
        if (get(VK_XBUTTON1)) {
            result |= MK_XBUTTON1;
        }
        if (get(VK_XBUTTON2)) {
            result |= MK_XBUTTON2;
        }
        return result;
    }

    static void emulateClientAreaMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam,
                                         const std::optional<int> &overrideMessage = std::nullopt) {
        const int myMsg = overrideMessage.value_or(message);
        const auto wParamNew = [myMsg, wParam]() -> WPARAM {
            if (myMsg == WM_NCMOUSELEAVE) {
                // wParam is always ignored in mouse leave messages, but here we
                // give them a special tag to be able to distinguish which messages
                // are sent by ourselves.
                return kMessageTag;
            }
            const quint64 keyState = getKeyState();
            if ((myMsg >= WM_NCXBUTTONDOWN) && (myMsg <= WM_NCXBUTTONDBLCLK)) {
                const auto xButtonMask = GET_XBUTTON_WPARAM(wParam);
                return MAKEWPARAM(keyState, xButtonMask);
            }
            return keyState;
        }();
        const auto lParamNew = [myMsg, lParam, hWnd]() -> LPARAM {
            if (myMsg == WM_NCMOUSELEAVE) {
                // lParam is always ignored in mouse leave messages.
                return 0;
            }
            const auto screenPos = POINT{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            POINT clientPos = screenPos;
            ::ScreenToClient(hWnd, &clientPos);
            return MAKELPARAM(clientPos.x, clientPos.y);
        }();
#if 0
#  define SEND_MESSAGE ::SendMessageW
#else
#  define SEND_MESSAGE ::PostMessageW
#endif
        switch (myMsg) {
            case WM_NCHITTEST: // Treat hit test messages as mouse move events.
            case WM_NCMOUSEMOVE:
                SEND_MESSAGE(hWnd, WM_MOUSEMOVE, wParamNew, lParamNew);
                break;
            case WM_NCLBUTTONDOWN:
                SEND_MESSAGE(hWnd, WM_LBUTTONDOWN, wParamNew, lParamNew);
                break;
            case WM_NCLBUTTONUP:
                SEND_MESSAGE(hWnd, WM_LBUTTONUP, wParamNew, lParamNew);
                break;
            case WM_NCLBUTTONDBLCLK:
                SEND_MESSAGE(hWnd, WM_LBUTTONDBLCLK, wParamNew, lParamNew);
                break;
            case WM_NCRBUTTONDOWN:
                SEND_MESSAGE(hWnd, WM_RBUTTONDOWN, wParamNew, lParamNew);
                break;
            case WM_NCRBUTTONUP:
                SEND_MESSAGE(hWnd, WM_RBUTTONUP, wParamNew, lParamNew);
                break;
            case WM_NCRBUTTONDBLCLK:
                SEND_MESSAGE(hWnd, WM_RBUTTONDBLCLK, wParamNew, lParamNew);
                break;
            case WM_NCMBUTTONDOWN:
                SEND_MESSAGE(hWnd, WM_MBUTTONDOWN, wParamNew, lParamNew);
                break;
            case WM_NCMBUTTONUP:
                SEND_MESSAGE(hWnd, WM_MBUTTONUP, wParamNew, lParamNew);
                break;
            case WM_NCMBUTTONDBLCLK:
                SEND_MESSAGE(hWnd, WM_MBUTTONDBLCLK, wParamNew, lParamNew);
                break;
            case WM_NCXBUTTONDOWN:
                SEND_MESSAGE(hWnd, WM_XBUTTONDOWN, wParamNew, lParamNew);
                break;
            case WM_NCXBUTTONUP:
                SEND_MESSAGE(hWnd, WM_XBUTTONUP, wParamNew, lParamNew);
                break;
            case WM_NCXBUTTONDBLCLK:
                SEND_MESSAGE(hWnd, WM_XBUTTONDBLCLK, wParamNew, lParamNew);
                break;
#if 0 // ### TODO: How to handle touch events?
        case WM_NCPOINTERUPDATE:
        case WM_NCPOINTERDOWN:
        case WM_NCPOINTERUP:
            break;
#endif
            case WM_NCMOUSEHOVER:
                SEND_MESSAGE(hWnd, WM_MOUSEHOVER, wParamNew, lParamNew);
                break;
            case WM_NCMOUSELEAVE:
                SEND_MESSAGE(hWnd, WM_MOUSELEAVE, wParamNew, lParamNew);
                break;
            default:
                break;
        }

#undef SEND_MESSAGE
    }

    static inline void requestForMouseLeaveMessage(HWND hWnd, bool nonClient) {
        TRACKMOUSEEVENT tme{};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        if (nonClient) {
            tme.dwFlags |= TME_NONCLIENT;
        }
        tme.hwndTrack = hWnd;
        tme.dwHoverTime = HOVER_DEFAULT;
        ::TrackMouseEvent(&tme);
    }

    bool Win32WindowContext::snapLayoutHandler(HWND hWnd, UINT message, WPARAM wParam,
                                               LPARAM lParam, LRESULT *result) {
        switch (message) {
            case WM_MOUSELEAVE: {
                if (!isTaggedMessage(wParam)) {
                    // Qt will call TrackMouseEvent() to get the WM_MOUSELEAVE message when it
                    // receives WM_MOUSEMOVE messages, and since we are converting every
                    // WM_NCMOUSEMOVE message to WM_MOUSEMOVE message and send it back to the window
                    // to be able to hover our controls, we also get lots of WM_MOUSELEAVE messages
                    // at the same time because of the reason above, and these superfluous mouse
                    // leave events cause Qt to think the mouse has left the control, and thus we
                    // actually lost the hover state. So we filter out these superfluous mouse leave
                    // events here to avoid this issue.
                    DWORD dwScreenPos = ::GetMessagePos();
                    POINT screenPoint{GET_X_LPARAM(dwScreenPos), GET_Y_LPARAM(dwScreenPos)};
                    ::ScreenToClient(hWnd, &screenPoint);
                    QPoint qtScenePos =
                        QHighDpi::fromNativeLocalPosition(QPoint{screenPoint.x, screenPoint.y}, m_windowHandle);
                    auto dummy = CoreWindowAgent::Unknown;
                    if (isInSystemButtons(qtScenePos, &dummy)) {
                        // We must record whether the last WM_MOUSELEAVE was filtered, because if
                        // Qt does not receive this message it will not call TrackMouseEvent()
                        // again, resulting in the client area not responding to any mouse event.
                        mouseLeaveBlocked = true;
                        *result = FALSE;
                        return true;
                    }
                }
                mouseLeaveBlocked = false;
                break;
            }

            case WM_MOUSEMOVE: {
                // At appropriate time, we will call TrackMouseEvent() for Qt. Simultaneously,
                // we unset `mouseLeaveBlocked` mark and pretend as if Qt has received
                // WM_MOUSELEAVE.
                if (lastHitTestResult != WindowPart::ChromeButton && mouseLeaveBlocked) {
                    mouseLeaveBlocked = false;
                    requestForMouseLeaveMessage(hWnd, false);
                }
                break;
            }

            case WM_NCMOUSEMOVE:
            case WM_NCLBUTTONDOWN:
            case WM_NCLBUTTONUP:
            case WM_NCLBUTTONDBLCLK:
            case WM_NCRBUTTONDOWN:
            case WM_NCRBUTTONUP:
            case WM_NCRBUTTONDBLCLK:
            case WM_NCMBUTTONDOWN:
            case WM_NCMBUTTONUP:
            case WM_NCMBUTTONDBLCLK:
            case WM_NCXBUTTONDOWN:
            case WM_NCXBUTTONUP:
            case WM_NCXBUTTONDBLCLK:
#if 0 // ### TODO: How to handle touch events?
    case WM_NCPOINTERUPDATE:
    case WM_NCPOINTERDOWN:
    case WM_NCPOINTERUP:
#endif
            case WM_NCMOUSEHOVER: {
                const WindowPart currentWindowPart = lastHitTestResult;
                if (message == WM_NCMOUSEMOVE) {
                    if (currentWindowPart != WindowPart::ChromeButton) {
                        m_delegate->resetQtGrabbedControl();
                        if (mouseLeaveBlocked) {
                            emulateClientAreaMessage(hWnd, message, wParam, lParam,
                                                     WM_NCMOUSELEAVE);
                        }
                    }

                    // We need to make sure we get the right hit-test result when a WM_NCMOUSELEAVE
                    // comes, so we reset it when we receive a WM_NCMOUSEMOVE.

                    // If the mouse is entering the client area, there must be a WM_NCHITTEST
                    // setting it to `Client` before the WM_NCMOUSELEAVE comes; if the mouse is
                    // leaving the window, current window part remains as `Outside`.
                    lastHitTestResult = WindowPart::Outside;
                }

                if (currentWindowPart == WindowPart::ChromeButton) {
                    emulateClientAreaMessage(hWnd, message, wParam, lParam);
                    if (message == WM_NCMOUSEMOVE) {
                        // ### FIXME FIXME FIXME
                        // ### FIXME: Calling DefWindowProc() here is really dangerous, investigate
                        // how to avoid doing this.
                        // ### FIXME FIXME FIXME
                        *result = ::DefWindowProcW(hWnd, WM_NCMOUSEMOVE, wParam, lParam);
                    } else {
                        // According to MSDN, we should return non-zero for X button messages to
                        // indicate we have handled these messages (due to historical reasons), for
                        // all other messages we should return zero instead.
                        *result =
                            (((message >= WM_NCXBUTTONDOWN) && (message <= WM_NCXBUTTONDBLCLK))
                                 ? TRUE
                                 : FALSE);
                    }
                    return true;
                }
                break;
            }

            case WM_NCMOUSELEAVE: {
                const WindowPart currentWindowPart = lastHitTestResult;
                if (currentWindowPart == WindowPart::ChromeButton) {
                    // If we press on the chrome button and move mouse, Windows will take the
                    // pressing area as HTCLIENT which maybe because of our former retransmission of
                    // WM_NCLBUTTONDOWN, as a result, a WM_NCMOUSELEAVE will come immediately and a
                    // lot of WM_MOUSEMOVE will come if we move the mouse, we should track the mouse
                    // in advance.
                    if (mouseLeaveBlocked) {
                        mouseLeaveBlocked = false;
                        requestForMouseLeaveMessage(hWnd, false);
                    }
                } else {
                    if (mouseLeaveBlocked) {
                        // The mouse is moving from the chrome button to other non-client area, we
                        // should emulate a WM_MOUSELEAVE message to reset the button state.
                        emulateClientAreaMessage(hWnd, message, wParam, lParam, WM_NCMOUSELEAVE);
                    }

                    if (currentWindowPart == WindowPart::Outside) {
                        // Notice: we're not going to clear window part cache when the mouse leaves
                        // window from client area, which means we will get previous window part as
                        // HTCLIENT if the mouse leaves window from client area and enters window
                        // from non-client area, but it has no bad effect.
                        m_delegate->resetQtGrabbedControl();
                    }
                }
                break;
            }

            default:
                break;
        }
        return false;
    }

    bool Win32WindowContext::customWindowHandler(HWND hWnd, UINT message, WPARAM wParam,
                                                 LPARAM lParam, LRESULT *result) {
        switch (message) {
            case WM_SHOWWINDOW: {
                if (!centered) {
                    // If wParam is TRUE, the window is being shown.
                    // If lParam is zero, the message was sent because of a call to the ShowWindow
                    // function.
                    if (wParam && lParam == 0) {
                        centered = true;
                        moveToDesktopCenter(hWnd);
                    }
                }
                break;
            }
            case WM_NCHITTEST: {
                // 原生Win32窗口只有顶边是在窗口内部resize的，其余三边都是在窗口
                // 外部进行resize的，其原理是，WS_THICKFRAME这个窗口样式会在窗
                // 口的左、右和底边添加三个透明的resize区域，这三个区域在正常状态
                // 下是完全不可见的，它们由DWM负责绘制和控制。这些区域的宽度等于
                // (SM_CXSIZEFRAME + SM_CXPADDEDBORDER)，高度等于
                // (SM_CYSIZEFRAME + SM_CXPADDEDBORDER)，在100%缩放时，均等
                // 于8像素。它们属于窗口区域的一部分，但不属于客户区，而是属于非客
                // 户区，因此GetWindowRect获取的区域中是包含这三个resize区域的，
                // 而GetClientRect获取的区域是不包含它们的。当把
                // DWMWA_EXTENDED_FRAME_BOUNDS作为参数调用
                // DwmGetWindowAttribute时，也能获取到一个窗口大小，这个大小介
                // 于前面两者之间，暂时不知道这个数据的意义及其作用。我们在
                // WM_NCCALCSIZE消息的处理中，已经把整个窗口都设置为客户区了，也
                // 就是说，我们的窗口已经没有非客户区了，因此那三个透明的resize区
                // 域，此刻也已经成为窗口客户区的一部分了，从而变得不透明了。所以
                // 现在的resize，看起来像是在窗口内部resize，是因为原本透明的地方
                // 现在变得不透明了，实际上，单纯从范围上来看，现在我们resize的地方，
                // 就是普通窗口的边框外部，那三个透明区域的范围。
                // 因此，如果我们把边框完全去掉（就是我们正在做的事情），resize就
                // 会看起来是在内部进行，这个问题通过常规方法非常难以解决。我测试过
                // QQ和钉钉的窗口，它们的窗口就是在外部resize，但实际上它们是通过
                // 把窗口实际的内容，嵌入到一个完全透明的但尺寸要大一圈的窗口中实现
                // 的，虽然看起来效果还不错，但对于此项目而言，代码和窗口结构过于复
                // 杂，因此我没有采用此方案。然而，对于具体的软件项目而言，其做法也
                // 不失为一个优秀的解决方案，毕竟其在大多数条件下的表现都还可以。
                //
                // 和1.x的做法不同，现在的2.x选择了保留窗口三边，去除整个窗口顶部，
                // 好处是保留了系统的原生边框，外观较好，且与系统结合紧密，而且resize
                // 的表现也有很大改善，缺点是需要自行绘制顶部边框线。原本以为只能像
                // Windows Terminal那样在WM_PAINT里搞黑魔法，但后来发现，其实只
                // 要颜色相近，我们自行绘制一根实线也几乎能以假乱真，而且这样也不会
                // 破坏Qt自己的绘制系统，能做到不依赖黑魔法就能实现像Windows Terminal
                // 那样外观和功能都比较完美的自定义边框。

                // A normal Win32 window can be resized outside of it. Here is the
                // reason: the WS_THICKFRAME window style will cause a window has three
                // transparent areas beside the window's left, right and bottom
                // edge. Their width or height is eight pixels if the window is not
                // scaled. In most cases, they are totally invisible. It's DWM's
                // responsibility to draw and control them. They exist to let the
                // user resize the window, visually outside of it. They are in the
                // window area, but not the client area, so they are in the
                // non-client area actually. But we have turned the whole window
                // area into client area in WM_NCCALCSIZE, so the three transparent
                // resize areas also become a part of the client area and thus they
                // become visible. When we resize the window, it looks like we are
                // resizing inside of it, however, that's because the transparent
                // resize areas are visible now, we ARE resizing outside of the
                // window actually. But I don't know how to make them become
                // transparent again without breaking the frame shadow drawn by DWM.
                // If you really want to solve it, you can try to embed your window
                // into a larger transparent window and draw the frame shadow
                // yourself. As what we have said in WM_NCCALCSIZE, you can only
                // remove the top area of the window, this will let us be able to
                // resize outside of the window and don't need much process in this
                // message, it looks like a perfect plan, however, the top border is
                // missing due to the whole top area is removed, and it's very hard
                // to bring it back because we have to use a trick in WM_PAINT
                // (learned from Windows Terminal), but no matter what we do in
                // WM_PAINT, it will always break the backing store mechanism of Qt,
                // so actually we can't do it. And it's very difficult to do such
                // things in NativeEventFilters as well. What's worse, if we really
                // do this, the four window borders will become white and they look
                // horrible in dark mode. This solution only supports Windows 10
                // because the border width on Win10 is only one pixel, however it's
                // eight pixels on Windows 7 so preserving the three window borders
                // looks terrible on old systems.
                //
                // Unlike the 1.x code, we choose to preserve the three edges of the
                // window in 2.x, and get rid of the whole top part of the window.
                // There are quite some advantages such as the appearance looks much
                // better and due to we have the original system window frame, our
                // window can behave just like a normal Win32 window even if we now
                // doesn't have a title bar at all. Most importantly, the flicker and
                // jitter during window resizing is totally gone now. The disadvantage
                // is we have to draw a top frame border ourselves. Previously I thought
                // we have to do the black magic in WM_PAINT just like what Windows
                // Terminal does, however, later I found that if we choose a proper
                // color, our homemade top border can almost have exactly the same
                // appearance with the system's one.

                [[maybe_unused]] const auto &hitTestRecorder = qScopeGuard([this, result]() {
                    lastHitTestResult = getHitWindowPart(int(*result)); //
                });

                POINT nativeGlobalPos{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                POINT nativeLocalPos = nativeGlobalPos;
                ::ScreenToClient(hWnd, &nativeLocalPos);

                RECT clientRect{0, 0, 0, 0};
                ::GetClientRect(hWnd, &clientRect);
                auto clientWidth = RECT_WIDTH(clientRect);
                auto clientHeight = RECT_HEIGHT(clientRect);

                QPoint qtScenePos = QHighDpi::fromNativeLocalPosition(
                    QPoint(nativeLocalPos.x, nativeLocalPos.y), m_windowHandle);

                bool isFixedSize = m_delegate->isHostSizeFixed(m_host);
                bool isTitleBar = isInTitleBarDraggableArea(qtScenePos);
                bool dontOverrideCursor = false; // ### TODO

                CoreWindowAgent::SystemButton sysButtonType = CoreWindowAgent::Unknown;
                if (!isFixedSize && isInSystemButtons(qtScenePos, &sysButtonType)) {
                    // Firstly, we set the hit test result to a default value to be able to detect
                    // whether we have changed it or not afterwards.
                    *result = HTNOWHERE;
                    // Even if the mouse is inside the chrome button area now, we should still allow
                    // the user to be able to resize the window with the top or right window border,
                    // this is also the normal behavior of a native Win32 window (but only when the
                    // window is not maximized/fullscreen/minimized, of course).
                    if (isWindowNoState(hWnd)) {
                        static constexpr const int kBorderSize = 2;
                        bool isTop = (nativeLocalPos.y <= kBorderSize);
                        bool isRight = (nativeLocalPos.x >= (clientWidth - kBorderSize));
                        if (isTop || isRight) {
                            if (dontOverrideCursor) {
                                // The user doesn't want the window to be resized, so we tell
                                // Windows we are in the client area so that the controls beneath
                                // the mouse cursor can still be hovered or clicked.
                                *result = (isTitleBar ? HTCAPTION : HTCLIENT);
                            } else {
                                if (isTop && isRight) {
                                    *result = HTTOPRIGHT;
                                } else if (isTop) {
                                    *result = HTTOP;
                                } else {
                                    *result = HTRIGHT;
                                }
                            }
                        }
                    }
                    if (*result == HTNOWHERE) {
                        // OK, we are now really inside one of the chrome buttons, tell Windows the
                        // exact role of our button. The Snap Layout feature introduced in Windows
                        // 11 won't work without this.
                        switch (sysButtonType) {
                            case CoreWindowAgent::WindowIcon:
                                *result = HTSYSMENU;
                                break;
                            case CoreWindowAgent::Help:
                                *result = HTHELP;
                                break;
                            case CoreWindowAgent::Minimize:
                                *result = HTREDUCE;
                                break;
                            case CoreWindowAgent::Maximize:
                                *result = HTZOOM;
                                break;
                            case CoreWindowAgent::Close:
                                *result = HTCLOSE;
                                break;
                            default:
                                break; // unreachable
                        }
                    }
                    if (*result == HTNOWHERE) {
                        // OK, it seems we are not inside the window resize area, nor inside the
                        // chrome buttons, tell Windows we are in the client area to let Qt handle
                        // this event.
                        *result = HTCLIENT;
                    }
                    return true;
                }
                // OK, we are not inside any chrome buttons, try to find out which part of the
                // window are we hitting.

                bool max = IsMaximized(hWnd);
                bool full = isFullScreen(hWnd);
                int frameSize = getResizeBorderThickness(hWnd);
                bool isTop = (nativeLocalPos.y < frameSize);

                if (isWin10OrGreater()) {
                    // This will handle the left, right and bottom parts of the frame
                    // because we didn't change them.
                    LRESULT originalHitTestResult = ::DefWindowProcW(hWnd, WM_NCHITTEST, 0, lParam);
                    if (originalHitTestResult != HTCLIENT) {
                        // Even if the window is not resizable, we still can't return HTCLIENT here
                        // because when we enter this code path, it means the mouse cursor is
                        // outside the window, that is, the three transparent window resize area.
                        // Returning HTCLIENT will confuse Windows, we can't put our controls there
                        // anyway.
                        *result = ((isFixedSize || dontOverrideCursor) ? HTBORDER
                                                                       : originalHitTestResult);
                        return true;
                    }
                    if (full) {
                        *result = HTCLIENT;
                        return true;
                    }
                    if (max) {
                        *result = (isTitleBar ? HTCAPTION : HTCLIENT);
                        return true;
                    }
                    // At this point, we know that the cursor is inside the client area
                    // so it has to be either the little border at the top of our custom
                    // title bar or the drag bar. Apparently, it must be the drag bar or
                    // the little border at the top which the user can use to move or
                    // resize the window.
                    if (isTop) {
                        // Return HTCLIENT instead of HTBORDER here, because the mouse is
                        // inside our homemade title bar now, return HTCLIENT to let our
                        // title bar can still capture mouse events.
                        *result = ((isFixedSize || dontOverrideCursor)
                                       ? (isTitleBar ? HTCAPTION : HTCLIENT)
                                       : HTTOP);
                        return true;
                    }
                    if (isTitleBar) {
                        *result = HTCAPTION;
                        return true;
                    }
                    *result = HTCLIENT;
                    return true;
                } else {
                    if (full) {
                        *result = HTCLIENT;
                        return true;
                    }
                    if (max) {
                        *result = (isTitleBar ? HTCAPTION : HTCLIENT);
                        return true;
                    }
                    if (!isFixedSize) {
                        const bool isBottom = (nativeLocalPos.y >= (clientHeight - frameSize));
                        // Make the border a little wider to let the user easy to resize on corners.
                        const auto scaleFactor = ((isTop || isBottom) ? qreal(2) : qreal(1));
                        const int scaledFrameSizeX = std::round(qreal(frameSize) * scaleFactor);
                        const bool isLeft = (nativeLocalPos.x < scaledFrameSizeX);
                        const bool isRight = (nativeLocalPos.x >= (clientWidth - scaledFrameSizeX));
                        if (dontOverrideCursor && (isTop || isBottom || isLeft || isRight)) {
                            // Return HTCLIENT instead of HTBORDER here, because the mouse is
                            // inside the window now, return HTCLIENT to let the controls
                            // inside our window can still capture mouse events.
                            *result = (isTitleBar ? HTCAPTION : HTCLIENT);
                            return true;
                        }
                        if (isTop) {
                            if (isLeft) {
                                *result = HTTOPLEFT;
                                return true;
                            }
                            if (isRight) {
                                *result = HTTOPRIGHT;
                                return true;
                            }
                            *result = HTTOP;
                            return true;
                        }
                        if (isBottom) {
                            if (isLeft) {
                                *result = HTBOTTOMLEFT;
                                return true;
                            }
                            if (isRight) {
                                *result = HTBOTTOMRIGHT;
                                return true;
                            }
                            *result = HTBOTTOM;
                            return true;
                        }
                        if (isLeft) {
                            *result = HTLEFT;
                            return true;
                        }
                        if (isRight) {
                            *result = HTRIGHT;
                            return true;
                        }
                    }
                    if (isTitleBar) {
                        *result = HTCAPTION;
                        return true;
                    }
                    *result = HTCLIENT;
                    return true;
                }
            }
            default:
                break;
        }
        if (!isWin10OrGreater()) {
            switch (message) {
                case WM_NCUAHDRAWCAPTION:
                case WM_NCUAHDRAWFRAME: {
                    // These undocumented messages are sent to draw themed window
                    // borders. Block them to prevent drawing borders over the client
                    // area.
                    *result = FALSE;
                    return true;
                }
                case WM_NCPAINT: {
                    // 边框阴影处于非客户区的范围，因此如果直接阻止非客户区的绘制，会导致边框阴影丢失

                    if (!isDwmCompositionEnabled()) {
                        // Only block WM_NCPAINT when DWM composition is disabled. If
                        // it's blocked when DWM composition is enabled, the frame
                        // shadow won't be drawn.
                        *result = FALSE;
                        return true;
                    } else {
                        break;
                    }
                }
                case WM_NCACTIVATE: {
                    if (isDwmCompositionEnabled()) {
                        // DefWindowProc won't repaint the window border if lParam (normally a HRGN)
                        // is -1. See the following link's "lParam" section:
                        // https://docs.microsoft.com/en-us/windows/win32/winmsg/wm-ncactivate
                        // Don't use "*result = 0" here, otherwise the window won't respond to the
                        // window activation state change.
                        *result = ::DefWindowProcW(hWnd, WM_NCACTIVATE, wParam, -1);
                    } else {
                        if (wParam) {
                            *result = FALSE;
                        } else {
                            *result = TRUE;
                        }
                    }
                    return true;
                }
                case WM_SETICON:
                case WM_SETTEXT: {
                    // Disable painting while these messages are handled to prevent them
                    // from drawing a window caption over the client area.
                    const auto oldStyle = static_cast<DWORD>(::GetWindowLongPtrW(hWnd, GWL_STYLE));
                    // Prevent Windows from drawing the default title bar by temporarily
                    // toggling the WS_VISIBLE style.
                    const DWORD newStyle = (oldStyle & ~WS_VISIBLE);
                    ::SetWindowLongPtrW(hWnd, GWL_STYLE, static_cast<LONG_PTR>(newStyle));
                    triggerFrameChange(hWnd);
                    const LRESULT originalResult = ::DefWindowProcW(hWnd, message, wParam, lParam);
                    ::SetWindowLongPtrW(hWnd, GWL_STYLE, static_cast<LONG_PTR>(oldStyle));
                    triggerFrameChange(hWnd);
                    *result = originalResult;
                    return true;
                }
                default:
                    break;
            }
        }
        return false;
    }

    bool Win32WindowContext::nonClientCalcSizeHandler(HWND hWnd, UINT message, WPARAM wParam,
                                                      LPARAM lParam, LRESULT *result) {
        Q_UNUSED(message)

        // Windows是根据这个消息的返回值来设置窗口的客户区（窗口中真正显示的内容）
        // 和非客户区（标题栏、窗口边框、菜单栏和状态栏等Windows系统自行提供的部分
        // ，不过对于Qt来说，除了标题栏和窗口边框，非客户区基本也都是自绘的）的范
        // 围的，lParam里存放的就是新客户区的几何区域，默认是整个窗口的大小，正常
        // 的程序需要修改这个参数，告知系统窗口的客户区和非客户区的范围（一般来说可
        // 以完全交给Windows，让其自行处理，使用默认的客户区和非客户区），因此如果
        // 我们不修改lParam，就可以使客户区充满整个窗口，从而去掉标题栏和窗口边框
        // （因为这些东西都被客户区给盖住了。但边框阴影也会因此而丢失，不过我们会使
        // 用其他方式将其带回，请参考其他消息的处理，此处不过多提及）。但有个情况要
        // 特别注意，那就是窗口最大化后，窗口的实际尺寸会比屏幕的尺寸大一点，从而使
        // 用户看不到窗口的边界，这样用户就不能在窗口最大化后调整窗口的大小了（虽然
        // 这个做法听起来特别奇怪，但Windows确实就是这样做的），因此如果我们要自行
        // 处理窗口的非客户区，就要在窗口最大化后，将窗口边框的宽度和高度（一般是相
        // 等的）从客户区裁剪掉，否则我们窗口所显示的内容就会超出屏幕边界，显示不全。
        // 如果用户开启了任务栏自动隐藏，在窗口最大化后，还要考虑任务栏的位置。因为
        // 如果窗口最大化后，其尺寸和屏幕尺寸相等（因为任务栏隐藏了，所以窗口最大化
        // 后其实是充满了整个屏幕，变相的全屏了），Windows会认为窗口已经进入全屏的
        // 状态，从而导致自动隐藏的任务栏无法弹出。要避免这个状况，就要使窗口的尺寸
        // 小于屏幕尺寸。我下面的做法参考了火狐、Chromium和Windows Terminal
        // 如果没有开启任务栏自动隐藏，是不存在这个问题的，所以要先进行判断。
        // 一般情况下，*result设置为0（相当于DefWindowProc的返回值为0）就可以了，
        // 根据MSDN的说法，返回0意为此消息已经被程序自行处理了，让Windows跳过此消
        // 息，否则Windows会添加对此消息的默认处理，对于当前这个消息而言，就意味着
        // 标题栏和窗口边框又会回来，这当然不是我们想要的结果。根据MSDN，当wParam
        // 为FALSE时，只能返回0，但当其为TRUE时，可以返回0，也可以返回一个WVR_常
        // 量。根据Chromium的注释，当存在非客户区时，如果返回WVR_REDRAW会导致子
        // 窗口/子控件出现奇怪的bug（自绘控件错位），并且Lucas在Windows 10
        // 上成功复现，说明这个bug至今都没有解决。我查阅了大量资料，发现唯一的解决
        // 方案就是返回0。但如果不存在非客户区，且wParam为TRUE，最好返回
        // WVR_REDRAW，否则窗口在调整大小可能会产生严重的闪烁现象。
        // 虽然对大多数消息来说，返回0都代表让Windows忽略此消息，但实际上不同消息
        // 能接受的返回值是不一样的，请注意自行查阅MSDN。

        // Sent when the size and position of a window's client area must be
        // calculated. By processing this message, an application can
        // control the content of the window's client area when the size or
        // position of the window changes. If wParam is TRUE, lParam points
        // to an NCCALCSIZE_PARAMS structure that contains information an
        // application can use to calculate the new size and position of the
        // client rectangle. If wParam is FALSE, lParam points to a RECT
        // structure. On entry, the structure contains the proposed window
        // rectangle for the window. On exit, the structure should contain
        // the screen coordinates of the corresponding window client area.
        // The client area is the window's content area, the non-client area
        // is the area which is provided by the system, such as the title
        // bar, the four window borders, the frame shadow, the menu bar, the
        // status bar, the scroll bar, etc. But for Qt, it draws most of the
        // window area (client + non-client) itself. We now know that the
        // title bar and the window frame is in the non-client area, and we
        // can set the scope of the client area in this message, so we can
        // remove the title bar and the window frame by let the non-client
        // area be covered by the client area (because we can't really get
        // rid of the non-client area, it will always be there, all we can
        // do is to hide it) , which means we should let the client area's
        // size the same with the whole window's size. So there is no room
        // for the non-client area and then the user won't be able to see it
        // again. But how to achieve this? Very easy, just leave lParam (the
        // re-calculated client area) untouched. But of course you can
        // modify lParam, then the non-client area will be seen and the
        // window borders and the window frame will show up. However, things
        // are quite different when you try to modify the top margin of the
        // client area. DWM will always draw the whole title bar no matter
        // what margin value you set for the top, unless you don't modify it
        // and remove the whole top area (the title bar + the one pixel
        // height window border). This can be confirmed in Windows
        // Terminal's source code, you can also try yourself to verify
        // it. So things will become quite complicated if you want to
        // preserve the four window borders.

        // If `wParam` is `FALSE`, `lParam` points to a `RECT` that contains
        // the proposed window rectangle for our window. During our
        // processing of the `WM_NCCALCSIZE` message, we are expected to
        // modify the `RECT` that `lParam` points to, so that its value upon
        // our return is the new client area. We must return 0 if `wParam`
        // is `FALSE`.
        // If `wParam` is `TRUE`, `lParam` points to a `NCCALCSIZE_PARAMS`
        // struct. This struct contains an array of 3 `RECT`s, the first of
        // which has the exact same meaning as the `RECT` that is pointed to
        // by `lParam` when `wParam` is `FALSE`. The remaining `RECT`s, in
        // conjunction with our return value, can
        // be used to specify portions of the source and destination window
        // rectangles that are valid and should be preserved. We opt not to
        // implement an elaborate client-area preservation technique, and
        // simply return 0, which means "preserve the entire old client area
        // and align it with the upper-left corner of our new client area".
        const auto clientRect = wParam ? &(reinterpret_cast<LPNCCALCSIZE_PARAMS>(lParam))->rgrc[0]
                                       : reinterpret_cast<LPRECT>(lParam);
        if (isWin10OrGreater()) {
            // Store the original top margin before the default window procedure applies the
            // default frame.
            const LONG originalTop = clientRect->top;
            // Apply the default frame because we don't want to remove the whole window
            // frame, we still need the standard window frame (the resizable frame border
            // and the frame shadow) for the left, bottom and right edges. If we return 0
            // here directly, the whole window frame will be removed (which means there will
            // be no resizable frame border and the frame shadow will also disappear), and
            // that's also how most applications customize their title bars on Windows. It's
            // totally OK but since we want to preserve as much original frame as possible,
            // we can't use that solution.
            const LRESULT hitTestResult = ::DefWindowProcW(hWnd, WM_NCCALCSIZE, wParam, lParam);
            if ((hitTestResult != HTERROR) && (hitTestResult != HTNOWHERE)) {
                *result = hitTestResult;
                return true;
            }
            // Re-apply the original top from before the size of the default frame was
            // applied, and the whole top frame (the title bar and the top border) is gone
            // now. For the top frame, we only has 2 choices: (1) remove the top frame
            // entirely, or (2) don't touch it at all. We can't preserve the top border by
            // adjusting the top margin here. If we try to modify the top margin, the
            // original title bar will always be painted by DWM regardless what margin we
            // set, so here we can only remove the top frame entirely and use some special
            // technique to bring the top border back.
            clientRect->top = originalTop;
        }
        const bool max = IsMaximized(hWnd);
        const bool full = isFullScreen(hWnd);
        // We don't need this correction when we're fullscreen. We will
        // have the WS_POPUP size, so we don't have to worry about
        // borders, and the default frame will be fine.
        if (max && !full) {
            // When a window is maximized, its size is actually a little bit more
            // than the monitor's work area. The window is positioned and sized in
            // such a way that the resize handles are outside the monitor and
            // then the window is clipped to the monitor so that the resize handle
            // do not appear because you don't need them (because you can't resize
            // a window when it's maximized unless you restore it).
            const quint32 frameSize = getResizeBorderThickness(hWnd);
            clientRect->top += frameSize;
            if (!isWin10OrGreater()) {
                clientRect->bottom -= frameSize;
                clientRect->left += frameSize;
                clientRect->right -= frameSize;
            }
        }
        // Attempt to detect if there's an autohide taskbar, and if
        // there is, reduce our size a bit on the side with the taskbar,
        // so the user can still mouse-over the taskbar to reveal it.
        // Make sure to use MONITOR_DEFAULTTONEAREST, so that this will
        // still find the right monitor even when we're restoring from
        // minimized.
        if (max || full) {
            APPBARDATA abd{};
            abd.cbSize = sizeof(abd);
            const UINT taskbarState = ::SHAppBarMessage(ABM_GETSTATE, &abd);
            // First, check if we have an auto-hide taskbar at all:
            if (taskbarState & ABS_AUTOHIDE) {
                bool top = false, bottom = false, left = false, right = false;
                // Due to ABM_GETAUTOHIDEBAREX was introduced in Windows 8.1,
                // we have to use another way to judge this if we are running
                // on Windows 7 or Windows 8.
                if (isWin8Point1OrGreater()) {
                    const RECT monitorRect = getMonitorForWindow(hWnd).rcMonitor;
                    // This helper can be used to determine if there's an
                    // auto-hide taskbar on the given edge of the monitor
                    // we're currently on.
                    const auto hasAutohideTaskbar = [monitorRect](const UINT edge) -> bool {
                        APPBARDATA abd2{};
                        abd2.cbSize = sizeof(abd2);
                        abd2.uEdge = edge;
                        abd2.rc = monitorRect;
                        const auto hTaskbar =
                            reinterpret_cast<HWND>(::SHAppBarMessage(ABM_GETAUTOHIDEBAREX, &abd2));
                        return (hTaskbar != nullptr);
                    };
                    top = hasAutohideTaskbar(ABE_TOP);
                    bottom = hasAutohideTaskbar(ABE_BOTTOM);
                    left = hasAutohideTaskbar(ABE_LEFT);
                    right = hasAutohideTaskbar(ABE_RIGHT);
                } else {
                    int edge = -1;
                    APPBARDATA abd2{};
                    abd2.cbSize = sizeof(abd2);
                    abd2.hWnd = ::FindWindowW(L"Shell_TrayWnd", nullptr);
                    HMONITOR windowMonitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
                    HMONITOR taskbarMonitor =
                        ::MonitorFromWindow(abd2.hWnd, MONITOR_DEFAULTTOPRIMARY);
                    if (taskbarMonitor == windowMonitor) {
                        ::SHAppBarMessage(ABM_GETTASKBARPOS, &abd2);
                        edge = int(abd2.uEdge);
                    }
                    top = (edge == ABE_TOP);
                    bottom = (edge == ABE_BOTTOM);
                    left = (edge == ABE_LEFT);
                    right = (edge == ABE_RIGHT);
                }
                // If there's a taskbar on any side of the monitor, reduce
                // our size a little bit on that edge.
                // Note to future code archeologists:
                // This doesn't seem to work for fullscreen on the primary
                // display. However, testing a bunch of other apps with
                // fullscreen modes and an auto-hiding taskbar has
                // shown that _none_ of them reveal the taskbar from
                // fullscreen mode. This includes Edge, Firefox, Chrome,
                // Sublime Text, PowerPoint - none seemed to support this.
                // This does however work fine for maximized.
                if (top) {
                    // Peculiarly, when we're fullscreen,
                    clientRect->top += kAutoHideTaskBarThickness;
                } else if (bottom) {
                    clientRect->bottom -= kAutoHideTaskBarThickness;
                } else if (left) {
                    clientRect->left += kAutoHideTaskBarThickness;
                } else if (right) {
                    clientRect->right -= kAutoHideTaskBarThickness;
                }
            }
        }
        // We should call this function only before the function returns.
        syncPaintEventWithDwm();
        // By returning WVR_REDRAW we can make the window resizing look
        // less broken. But we must return 0 if wParam is FALSE, according to Microsoft
        // Docs.
        // **IMPORTANT NOTE**:
        // If you are drawing something manually through D3D in your window, don't
        // try to return WVR_REDRAW here, otherwise Windows exhibits bugs where
        // client pixels and child windows are mispositioned by the width/height
        // of the upper-left non-client area. It's confirmed that this issue exists
        // from Windows 7 to Windows 10. Not tested on Windows 11 yet. Don't know
        // whether it exists on Windows XP to Windows Vista or not.
        *result = wParam ? WVR_REDRAW : FALSE;
        return true;
    }

}
