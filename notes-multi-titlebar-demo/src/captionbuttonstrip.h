#pragma once

#include <QWidget>

class QPushButton;

class CaptionButtonStrip final : public QWidget
{
    Q_OBJECT

public:
    explicit CaptionButtonStrip(QWidget *parent = nullptr);

    int preferredWidth() const noexcept;
    int preferredHeight() const noexcept;

    QPushButton *minimizeButton() const noexcept
    {
        return m_minimizeButton;
    }

    QPushButton *maximizeButton() const noexcept
    {
        return m_maximizeButton;
    }

    QPushButton *closeButton() const noexcept
    {
        return m_closeButton;
    }

    void syncWindowState(bool maximized);

private:
    QPushButton *m_minimizeButton = nullptr;
    QPushButton *m_maximizeButton = nullptr;
    QPushButton *m_closeButton = nullptr;
};