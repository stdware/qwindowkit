#include "mainwindow.h"

#include <QtCore/QDebug>
#include <QtCore/QTime>
#include <QtWidgets/QPushButton>

#include <QWKWidgets/widgetwindowagent.h>

#include <widgetframe/windowbar.h>

class ClockWidget : public QPushButton {
public:
    explicit ClockWidget(QWidget *parent = nullptr) : QPushButton(parent) {
        startTimer(100);
    }

    ~ClockWidget() override = default;

protected:
    void timerEvent(QTimerEvent *event) override {
        setText(QTime::currentTime().toString(QStringLiteral("hh:mm:ss")));
    }
};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    installWindowAgent();
}

MainWindow::~MainWindow() {
}

void MainWindow::installWindowAgent() {
    auto agent = new QWK::WidgetWindowAgent(this);
    if (!agent->setup(this)) {
        qFatal("Frameless handle failed to initialize.");
    }

    auto titleLabel = new QLabel();
    titleLabel->setAlignment(Qt::AlignCenter);

    auto menuBar = []() {
        auto menuBar = new QMenuBar();
        auto file = new QMenu("File(&F)");
        file->addAction(new QAction("New(&N)"));
        file->addAction(new QAction("Open(&O)"));

        auto edit = new QMenu("Edit(&E)");
        edit->addAction(new QAction("Undo(&U)"));
        edit->addAction(new QAction("Redo(&R)"));

        menuBar->addMenu(file);
        menuBar->addMenu(edit);
        return menuBar;
    }();

    auto iconButton = new QPushButton("I");
    iconButton->setAttribute(Qt::WA_Hover);
    iconButton->setMouseTracking(true);
    auto minButton = new QPushButton("—");
    minButton->setAttribute(Qt::WA_Hover);
    minButton->setMouseTracking(true);
    auto maxButton = new QPushButton("🗖");
    maxButton->setCheckable(true);
    maxButton->setAttribute(Qt::WA_Hover);
    maxButton->setMouseTracking(true);
    auto closeButton = new QPushButton("✖");
    closeButton->setAttribute(Qt::WA_Hover);
    closeButton->setMouseTracking(true);

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

    connect(windowBar, &QWK::WindowBar::minimizeRequested, this, &QWidget::showMinimized);
    connect(windowBar, &QWK::WindowBar::maximizeRequested, this, [this](bool max) {
        if (max) {
            showMaximized();
        } else {
            showNormal();
        }
    });
    connect(windowBar, &QWK::WindowBar::closeRequested, this, &QWidget::close);

    auto clockWidget = new ClockWidget();
    clockWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(clockWidget, &QAbstractButton::clicked, this, [this]() {
        if (!isMaximized()) {
            showMaximized();
        } else {
            showNormal();
        }
    });

    setMenuWidget(windowBar);
    setCentralWidget(clockWidget);
    setWindowTitle("Example MainWindow");
    // setContentsMargins({0, 1, 0, 0});
    resize(640, 480);
}
