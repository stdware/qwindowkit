// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "mainwindow.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QVariant>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QWindow>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyle>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QColorDialog>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  include <QtGui/QActionGroup>
#else
#  include <QtWidgets/QActionGroup>
#endif

// #include <QtWebEngineWidgets/QWebEngineView>

#include <QWKWidgets/widgetwindowagent.h>

#include <widgetframe/windowbar.h>
#include <widgetframe/windowbutton.h>

class ClockWidget : public QLabel {
public:
    explicit ClockWidget(QWidget *parent = nullptr) : QLabel(parent) {
        startTimer(100);
        setAlignment(Qt::AlignCenter);
    }

    ~ClockWidget() override = default;

protected:
    void timerEvent(QTimerEvent *event) override {
        QLabel::timerEvent(event);
        setText(QTime::currentTime().toString(QStringLiteral("hh:mm:ss")));
    }
};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setAttribute(Qt::WA_DontCreateNativeAncestors);

    installWindowAgent();

#if 1
    auto clockWidget = new ClockWidget();
    clockWidget->setObjectName(QStringLiteral("clock-widget"));
    clockWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCentralWidget(clockWidget);
#else
    auto webView = new QWebEngineView();
    webView->load(QUrl("https://www.baidu.com"));
    setCentralWidget(webView);
#endif

    loadStyleSheet(Dark);

    setWindowTitle(tr("Example MainWindow"));
    resize(800, 600);

    // setFixedHeight(600);
    // windowAgent->centralize();
}

static inline void emulateLeaveEvent(QWidget *widget) {
    Q_ASSERT(widget);
    if (!widget) {
        return;
    }
    QTimer::singleShot(0, widget, [widget]() {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        const QScreen *screen = widget->screen();
#else
        const QScreen *screen = widget->windowHandle()->screen();
#endif
        const QPoint globalPos = QCursor::pos(screen);
        if (!QRect(widget->mapToGlobal(QPoint{0, 0}), widget->size()).contains(globalPos)) {
            QCoreApplication::postEvent(widget, new QEvent(QEvent::Leave));
            if (widget->testAttribute(Qt::WA_Hover)) {
                const QPoint localPos = widget->mapFromGlobal(globalPos);
                const QPoint scenePos = widget->window()->mapFromGlobal(globalPos);
                static constexpr const auto oldPos = QPoint{};
                const Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
                const auto event =
                    new QHoverEvent(QEvent::HoverLeave, scenePos, globalPos, oldPos, modifiers);
                Q_UNUSED(localPos);
#elif (QT_VERSION >= QT_VERSION_CHECK(6, 3, 0))
                const auto event =  new QHoverEvent(QEvent::HoverLeave, localPos, globalPos, oldPos, modifiers);
                Q_UNUSED(scenePos);
#else
                const auto event =  new QHoverEvent(QEvent::HoverLeave, localPos, oldPos, modifiers);
                Q_UNUSED(scenePos);
#endif
                QCoreApplication::postEvent(widget, event);
            }
        }
    });
}

MainWindow::~MainWindow() = default;

bool MainWindow::event(QEvent *event) {
    switch (event->type()) {
        case QEvent::WindowActivate: {
            auto menu = menuWidget();
            if (menu) {
                menu->setProperty("bar-active", true);
                style()->polish(menu);
            }
            break;
        }

        case QEvent::WindowDeactivate: {
            auto menu = menuWidget();
            if (menu) {
                menu->setProperty("bar-active", false);
                style()->polish(menu);
            }
            break;
        }

        default:
            break;
    }
    return QMainWindow::event(event);
}

void MainWindow::installWindowAgent() {
    // 1. Setup window agent
    windowAgent = new QWK::WidgetWindowAgent(this);
    windowAgent->setup(this);

    // 2. Construct your title bar
    auto menuBar = [this]() {
        auto menuBar = new QMenuBar(this);

        // Virtual menu
        auto file = new QMenu(tr("File(&F)"), menuBar);
        file->addAction(new QAction(tr("New(&N)"), menuBar));
        file->addAction(new QAction(tr("Open(&O)"), menuBar));
        file->addSeparator();

        auto edit = new QMenu(tr("Edit(&E)"), menuBar);
        edit->addAction(new QAction(tr("Undo(&U)"), menuBar));
        edit->addAction(new QAction(tr("Redo(&R)"), menuBar));

        // Theme action
        auto darkAction = new QAction(tr("Enable dark theme"), menuBar);
        darkAction->setCheckable(true);
        connect(darkAction, &QAction::triggered, this, [this](bool checked) {
            loadStyleSheet(checked ? Dark : Light); //
        });
        connect(this, &MainWindow::themeChanged, darkAction, [this, darkAction]() {
            darkAction->setChecked(currentTheme == Dark); //
        });

#ifdef Q_OS_WIN
        auto noneAction = new QAction(tr("None"), menuBar);
        noneAction->setData(QStringLiteral("none"));
        noneAction->setCheckable(true);
        noneAction->setChecked(true);

        auto dwmBlurAction = new QAction(tr("Enable DWM blur"), menuBar);
        dwmBlurAction->setData(QStringLiteral("dwm-blur"));
        dwmBlurAction->setCheckable(true);

        auto acrylicAction = new QAction(tr("Enable acrylic material"), menuBar);
        acrylicAction->setData(QStringLiteral("acrylic-material"));
        acrylicAction->setCheckable(true);

        auto micaAction = new QAction(tr("Enable mica"), menuBar);
        micaAction->setData(QStringLiteral("mica"));
        micaAction->setCheckable(true);

        auto micaAltAction = new QAction(tr("Enable mica alt"), menuBar);
        micaAltAction->setData(QStringLiteral("mica-alt"));
        micaAltAction->setCheckable(true);

        auto winStyleGroup = new QActionGroup(menuBar);
        winStyleGroup->addAction(noneAction);
        winStyleGroup->addAction(dwmBlurAction);
        winStyleGroup->addAction(acrylicAction);
        winStyleGroup->addAction(micaAction);
        winStyleGroup->addAction(micaAltAction);
        connect(winStyleGroup, &QActionGroup::triggered, this,
                [this, winStyleGroup](QAction *action) {
                    // Unset all custom style attributes first, otherwise the style will not display
                    // correctly
                    for (const QAction *_act : winStyleGroup->actions()) {
                        const QString data = _act->data().toString();
                        if (data.isEmpty() || data == QStringLiteral("none")) {
                            continue;
                        }
                        windowAgent->setWindowAttribute(data, false);
                    }
                    const QString data = action->data().toString();
                    if (data == QStringLiteral("none")) {
                        setProperty("custom-style", false);
                    } else if (!data.isEmpty()) {
                        windowAgent->setWindowAttribute(data, true);
                        setProperty("custom-style", true);
                    }
                    style()->polish(this);
                });

        auto borderColorAction = new QAction(tr("Border color"), menuBar);
        borderColorAction->setData(QStringLiteral("dwm-border-color"));
        borderColorAction->setCheckable(false);
        connect(borderColorAction, &QAction::triggered, this, [this, borderColorAction]() {
            QColor color = QColorDialog::getColor(Qt::black, this, tr("Select window border color"));
            if (color.isValid()) {
                const QString data = borderColorAction->data().toString();
                windowAgent->setWindowAttribute(data, color);
                style()->polish(this);
            }
        });
#elif defined(Q_OS_MAC)
        // Set whether to use system buttons (close/minimize/zoom)
        // - true:  Hide system buttons (use custom UI controls)
        // - false: Show native system buttons (default behavior)
        windowAgent->setWindowAttribute(QStringLiteral("no-system-buttons"), false);

        auto polishMacStyle = [this](bool customStyle) {
            setProperty("custom-style", customStyle);
            style()->polish(this);
        };

        auto applyMacBlurEffect = [this, polishMacStyle](const QString &effect) {
            windowAgent->setWindowAttribute(QStringLiteral("glass-effect"), QStringLiteral("none"));
            if (!windowAgent->setWindowAttribute(QStringLiteral("blur-effect"), effect)) {
                return;
            }
            polishMacStyle(effect != QStringLiteral("none"));
        };

        auto applyMacGlassEffect = [this, polishMacStyle](const QString &effect,
                                                          qreal radius = 0,
                                                          const QVariant &tintColor = {}) {
            windowAgent->setWindowAttribute(QStringLiteral("blur-effect"), QStringLiteral("none"));
            windowAgent->setWindowAttribute(QStringLiteral("glass-corner-radius"), radius);
            windowAgent->setWindowAttribute(QStringLiteral("glass-tint-color"),
                                            tintColor.isValid() ? tintColor : QVariant(QStringLiteral("none")));
            if (!windowAgent->setWindowAttribute(QStringLiteral("glass-effect"), effect)) {
                return;
            }
            polishMacStyle(effect != QStringLiteral("none"));
        };

        auto glassRegularAction = new QAction(tr("Glass: regular"), menuBar);
        glassRegularAction->setCheckable(true);
        connect(glassRegularAction, &QAction::toggled, this, [applyMacGlassEffect](bool checked) {
            if (checked) {
                applyMacGlassEffect(QStringLiteral("regular"));
            }
        });

        auto glassClearAction = new QAction(tr("Glass: clear"), menuBar);
        glassClearAction->setCheckable(true);
        connect(glassClearAction, &QAction::toggled, this, [applyMacGlassEffect](bool checked) {
            if (checked) {
                applyMacGlassEffect(QStringLiteral("clear"));
            }
        });

        auto glassRegularRoundedAction = new QAction(tr("Glass: regular, rounded"), menuBar);
        glassRegularRoundedAction->setCheckable(true);
        connect(glassRegularRoundedAction, &QAction::toggled, this,
                [applyMacGlassEffect](bool checked) {
                    if (checked) {
                        applyMacGlassEffect(QStringLiteral("regular"), 24);
                    }
                });

        auto glassRegularDarkTintAction = new QAction(tr("Glass: regular, dark tint"), menuBar);
        glassRegularDarkTintAction->setCheckable(true);
        connect(glassRegularDarkTintAction, &QAction::toggled, this,
                [applyMacGlassEffect](bool checked) {
                    if (checked) {
                        applyMacGlassEffect(QStringLiteral("regular"), 0, QColor(0, 0, 0, 46));
                    }
                });

        auto glassRegularLightTintAction = new QAction(tr("Glass: regular, light tint"), menuBar);
        glassRegularLightTintAction->setCheckable(true);
        connect(glassRegularLightTintAction, &QAction::toggled, this,
                [applyMacGlassEffect](bool checked) {
                    if (checked) {
                        applyMacGlassEffect(QStringLiteral("regular"), 0, QColor(255, 255, 255, 46));
                    }
                });

        auto darkBlurAction = new QAction(tr("Dark blur"), menuBar);
        darkBlurAction->setCheckable(true);
        connect(darkBlurAction, &QAction::toggled, this, [applyMacBlurEffect](bool checked) {
            if (checked) {
                applyMacBlurEffect(QStringLiteral("dark"));
            }
        });

        auto lightBlurAction = new QAction(tr("Light blur"), menuBar);
        lightBlurAction->setCheckable(true);
        connect(lightBlurAction, &QAction::toggled, this, [applyMacBlurEffect](bool checked) {
            if (checked) {
                applyMacBlurEffect(QStringLiteral("light"));
            }
        });

        auto noEffectAction = new QAction(tr("No effect"), menuBar);
        noEffectAction->setCheckable(true);
        connect(noEffectAction, &QAction::toggled, this, [applyMacGlassEffect](bool checked) {
            if (checked) {
                applyMacGlassEffect(QStringLiteral("none"));
            }
        });

        auto macStyleGroup = new QActionGroup(menuBar);
        macStyleGroup->addAction(glassRegularAction);
        macStyleGroup->addAction(glassClearAction);
        macStyleGroup->addAction(glassRegularRoundedAction);
        macStyleGroup->addAction(glassRegularDarkTintAction);
        macStyleGroup->addAction(glassRegularLightTintAction);
        macStyleGroup->addAction(darkBlurAction);
        macStyleGroup->addAction(lightBlurAction);
        macStyleGroup->addAction(noEffectAction);
#endif

        // Real menu
        auto settings = new QMenu(tr("Settings(&S)"), menuBar);
        settings->addAction(darkAction);

#ifdef Q_OS_WIN
        settings->addSeparator();
        settings->addAction(noneAction);
        settings->addAction(dwmBlurAction);
        settings->addAction(acrylicAction);
        settings->addAction(micaAction);
        settings->addAction(micaAltAction);
        settings->addSeparator();
        settings->addAction(borderColorAction);
#elif defined(Q_OS_MAC)
        settings->addAction(glassRegularAction);
        settings->addAction(glassClearAction);
        settings->addAction(glassRegularRoundedAction);
        settings->addAction(glassRegularDarkTintAction);
        settings->addAction(glassRegularLightTintAction);
        settings->addSeparator();
        settings->addAction(darkBlurAction);
        settings->addAction(lightBlurAction);
        settings->addAction(noEffectAction);
#endif

        menuBar->addMenu(file);
        menuBar->addMenu(edit);
        menuBar->addMenu(settings);
        return menuBar;
    }();
    menuBar->setObjectName(QStringLiteral("win-menu-bar"));

    auto titleLabel = new QLabel();
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setObjectName(QStringLiteral("win-title-label"));

#ifndef Q_OS_MAC
    auto iconButton = new QWK::WindowButton();
    iconButton->setObjectName(QStringLiteral("icon-button"));
    iconButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    auto pinButton = new QWK::WindowButton();
    pinButton->setCheckable(true);
    pinButton->setObjectName(QStringLiteral("pin-button"));
    pinButton->setProperty("system-button", true);
    pinButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    auto minButton = new QWK::WindowButton();
    minButton->setObjectName(QStringLiteral("min-button"));
    minButton->setProperty("system-button", true);
    minButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    auto maxButton = new QWK::WindowButton();
    maxButton->setCheckable(true);
    maxButton->setObjectName(QStringLiteral("max-button"));
    maxButton->setProperty("system-button", true);
    maxButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    auto closeButton = new QWK::WindowButton();
    closeButton->setObjectName(QStringLiteral("close-button"));
    closeButton->setProperty("system-button", true);
    closeButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
#endif

    auto windowBar = new QWK::WindowBar();
#ifndef Q_OS_MAC
    windowBar->setIconButton(iconButton);
    windowBar->setPinButton(pinButton);
    windowBar->setMinButton(minButton);
    windowBar->setMaxButton(maxButton);
    windowBar->setCloseButton(closeButton);
#endif
    windowBar->setMenuBar(menuBar);
    windowBar->setTitleLabel(titleLabel);
    windowBar->setHostWidget(this);

    windowAgent->setTitleBar(windowBar);
#ifndef Q_OS_MAC
    windowAgent->setHitTestVisible(pinButton, true);
    windowAgent->setSystemButton(QWK::WindowAgentBase::WindowIcon, iconButton);
    windowAgent->setSystemButton(QWK::WindowAgentBase::Minimize, minButton);
    windowAgent->setSystemButton(QWK::WindowAgentBase::Maximize, maxButton);
    windowAgent->setSystemButton(QWK::WindowAgentBase::Close, closeButton);
#endif
    windowAgent->setHitTestVisible(menuBar, true);

#if defined(Q_OS_MAC) && 0
    windowAgent->setSystemButtonAreaCallback([](const QSize &size) {
        static constexpr const int width = 75;
        return QRect(QPoint(size.width() - width, 0), QSize(width, size.height())); //
    });
#endif

    setMenuWidget(windowBar);


#ifndef Q_OS_MAC
    connect(windowBar, &QWK::WindowBar::pinRequested, this, [this, pinButton](bool pin) {
        if (isHidden() || isMinimized() || isMaximized() || isFullScreen()) {
            return;
        }
        setWindowFlag(Qt::WindowStaysOnTopHint, pin);
        show();
        pinButton->setChecked(pin);
    });
    connect(windowBar, &QWK::WindowBar::minimizeRequested, this, &QWidget::showMinimized);
    connect(windowBar, &QWK::WindowBar::maximizeRequested, this, [this, maxButton](bool max) {
        if (max) {
            showMaximized();
        } else {
            showNormal();
        }

        // It's a Qt issue that if a QAbstractButton::clicked triggers a window's maximization,
        // the button remains to be hovered until the mouse move. As a result, we need to
        // manually send leave events to the button.
        emulateLeaveEvent(maxButton);
    });
    connect(windowBar, &QWK::WindowBar::closeRequested, this, &QWidget::close);
#endif
}

void MainWindow::loadStyleSheet(Theme theme) {
    if (!styleSheet().isEmpty() && theme == currentTheme)
        return;
    currentTheme = theme;

    if (QFile qss(theme == Dark ? QStringLiteral(":/dark-style.qss")
                                : QStringLiteral(":/light-style.qss"));
        qss.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStyleSheet(QString::fromUtf8(qss.readAll()));
        Q_EMIT themeChanged();
    }
}
