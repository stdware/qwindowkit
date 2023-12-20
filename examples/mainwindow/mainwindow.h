#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    enum Theme {
        Dark,
        Light,
    };
    Q_ENUM(Theme)

Q_SIGNALS:
    void themeChanged();

protected:
    bool event(QEvent *event) override;

private:
    void installWindowAgent();
    void loadStyleSheet(Theme theme);

    Theme currentTheme{};
};

#endif // MAINWINDOW_H
