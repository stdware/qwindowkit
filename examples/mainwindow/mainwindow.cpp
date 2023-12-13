#include "mainwindow.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyle>
#include <QtWidgets/QPushButton>

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
        setText(QTime::currentTime().toString(QStringLiteral("hh:mm:ss")));
    }
};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    installWindowAgent();

    auto clockWidget = new ClockWidget();
    clockWidget->setObjectName("clock-widget");
    clockWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCentralWidget(clockWidget);

    if (QFile qss(":/dark-style.qss"); qss.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStyleSheet(QString::fromUtf8(qss.readAll()));
    }

    setWindowTitle("Example MainWindow");
    resize(640, 480);
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

MainWindow::~MainWindow() {
}

bool MainWindow::event(QEvent *event) {
    switch (event->type()) {
        case QEvent::WindowActivate: {
            auto menu = menuWidget();
            menu->setProperty("bar-active", true);
            style()->polish(menu);
            break;
        }

        case QEvent::WindowDeactivate: {
            auto menu = menuWidget();
            menu->setProperty("bar-active", false);
            style()->polish(menu);
            break;
        }

        default:
            break;
    }
    return QMainWindow::event(event);
}

void MainWindow::installWindowAgent() {
    auto agent = new QWK::WidgetWindowAgent(this);
    if (!agent->setup(this)) {
        qFatal("Frameless handle failed to initialize.");
    }

    auto menuBar = []() {
        auto menuBar = new QMenuBar();
        auto file = new QMenu("File(&F)", menuBar);
        file->addAction(new QAction("New(&N)", menuBar));
        file->addAction(new QAction("Open(&O)", menuBar));

        auto edit = new QMenu("Edit(&E)", menuBar);
        edit->addAction(new QAction("Undo(&U)", menuBar));
        edit->addAction(new QAction("Redo(&R)", menuBar));

        menuBar->addMenu(file);
        menuBar->addMenu(edit);
        return menuBar;
    }();
    menuBar->setObjectName("win-menu-bar");

    auto titleLabel = new QLabel();
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setObjectName("win-title-label");

    auto iconButton = new QWK::WindowButton();
    iconButton->setObjectName("icon-button");
    iconButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    auto minButton = new QWK::WindowButton();
    minButton->setObjectName("min-button");
    minButton->setProperty("system-button", true);
    minButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    auto maxButton = new QWK::WindowButton();
    maxButton->setCheckable(true);
    maxButton->setObjectName("max-button");
    maxButton->setProperty("system-button", true);
    maxButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    auto closeButton = new QWK::WindowButton();
    closeButton->setObjectName("close-button");
    closeButton->setProperty("system-button", true);
    closeButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    auto windowBar = new QWK::WindowBar();
    windowBar->setIconButton(iconButton);
    windowBar->setMinButton(minButton);
    windowBar->setMaxButton(maxButton);
    windowBar->setCloseButton(closeButton);
    windowBar->setMenuBar(menuBar);
    windowBar->setTitleLabel(titleLabel);
    windowBar->setHostWidget(this);

    agent->setTitleBar(windowBar);
    agent->setSystemButton(QWK::WindowAgentBase::WindowIcon, iconButton);
    agent->setSystemButton(QWK::WindowAgentBase::Minimize, minButton);
    agent->setSystemButton(QWK::WindowAgentBase::Maximize, maxButton);
    agent->setSystemButton(QWK::WindowAgentBase::Close, closeButton);
    agent->setHitTestVisible(menuBar, true);

#ifdef Q_OS_WINDOWS
    // Emulate Window system menu button behaviors
    connect(iconButton, &QAbstractButton::clicked, agent, [this, iconButton, agent] {
        setProperty("double-click-close", false);

        // Pick a suitable time threshold
        QTimer::singleShot(75, this, [this, iconButton, agent]() {
            if (property("double-click-close").toBool())
                return;
            agent->showSystemMenu(iconButton->mapToGlobal({0, iconButton->height()}));
        });
    });
    connect(iconButton, &QWK::WindowButton::doubleClicked, this, [this]() {
        setProperty("double-click-close", true);
        close();
    });
#endif

    connect(windowBar, &QWK::WindowBar::minimizeRequested, this, &QWidget::showMinimized);
    connect(windowBar, &QWK::WindowBar::maximizeRequested, this, [this, maxButton](bool max) {
        if (max) {
            showMaximized();
        } else {
            showNormal();
        }

        // It's a Qt issue that if a QAbstractButton::clicked triggers a window's maximization,
        // the button remains to be hovered until the mouse move. As a result, we need to
        // manully send leave events to the button.
        emulateLeaveEvent(maxButton);
    });
    connect(windowBar, &QWK::WindowBar::closeRequested, this, &QWidget::close);
    setMenuWidget(windowBar);
}
