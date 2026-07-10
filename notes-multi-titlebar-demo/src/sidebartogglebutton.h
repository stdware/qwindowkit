#pragma once

#include <QPushButton>

class QEnterEvent;
class QPropertyAnimation;

class SidebarToggleButton final : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)
    Q_PROPERTY(qreal hoverProgress READ hoverProgress WRITE setHoverProgress)

public:
    explicit SidebarToggleButton(QWidget *parent = nullptr);

    void setCollapsed(bool collapsed);

    bool isCollapsed() const noexcept
    {
        return m_collapsed;
    }

    qreal progress() const noexcept
    {
        return m_progress;
    }

    void setProgress(qreal value);

    qreal hoverProgress() const noexcept
    {
        return m_hoverProgress;
    }

    void setHoverProgress(qreal value);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void animateHover(qreal endValue);

private:
    bool m_collapsed = false;

    /*
        0.0 = folders sidebar visible.
        1.0 = folders sidebar collapsed.
    */
    qreal m_progress = 0.0;

    /*
        A tiny hover animation makes the button feel closer to modern macOS
        toolbar controls.
    */
    qreal m_hoverProgress = 0.0;

    QPropertyAnimation *m_progressAnimation = nullptr;
    QPropertyAnimation *m_hoverAnimation = nullptr;
};