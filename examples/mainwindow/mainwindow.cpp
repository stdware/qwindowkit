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
        qDebug() << "Frameless handle failed to initialize.";
        return;
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

    static const auto buttonStyleSheet = QLatin1String{ "QPushButton{color:black;};QPushButton:hover{background-color:black;color:white;}" };

    auto iconButton = new QPushButton("I");
    iconButton->setStyleSheet(buttonStyleSheet);
    auto minButton = new QPushButton("—");
    minButton->setStyleSheet(buttonStyleSheet);
    auto maxButton = new QPushButton("🗖");
    maxButton->setStyleSheet(buttonStyleSheet);
    maxButton->setCheckable(true);
    auto closeButton = new QPushButton("✖");
    closeButton->setStyleSheet(buttonStyleSheet);

    auto windowBar = new QWK::WindowBar();
    windowBar->setIconButton(iconButton);
    windowBar->setMinButton(minButton);
    windowBar->setMaxButton(maxButton);
    windowBar->setCloseButton(closeButton);
    windowBar->setMenuBar(menuBar);
    windowBar->setTitleLabel(titleLabel);
    windowBar->setHostWidget(this);

    agent->setTitleBar(windowBar);
    agent->setSystemButton(QWK::CoreWindowAgent::WindowIcon, iconButton);
    agent->setSystemButton(QWK::CoreWindowAgent::Minimize, minButton);
    agent->setSystemButton(QWK::CoreWindowAgent::Maximize, maxButton);
    agent->setSystemButton(QWK::CoreWindowAgent::Close, closeButton);
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

    setMenuWidget(windowBar);
    setCentralWidget(clockWidget);
    setWindowTitle("Example MainWindow");
    resize(1024, 768);
}
