#include "sidebartogglebutton.h"

#include <QEnterEvent>
#include <QEasingCurve>
#include <QPainter>
#include <QPaintEvent>
#include <QPropertyAnimation>

#include <algorithm>

SidebarToggleButton::SidebarToggleButton(QWidget *parent)
    : QPushButton(parent)
{
    setObjectName(QStringLiteral("sidebarToggleButton"));
    setFocusPolicy(Qt::NoFocus);
    setFlat(true);
    setCursor(Qt::ArrowCursor);
    setFixedSize(30, 30);
    setAttribute(Qt::WA_Hover, true);

    m_progressAnimation = new QPropertyAnimation(this, "progress", this);
    m_progressAnimation->setDuration(190);
    m_progressAnimation->setEasingCurve(QEasingCurve::InOutCubic);

    m_hoverAnimation = new QPropertyAnimation(this, "hoverProgress", this);
    m_hoverAnimation->setDuration(120);
    m_hoverAnimation->setEasingCurve(QEasingCurve::OutCubic);
}

void SidebarToggleButton::setCollapsed(bool collapsed)
{
    if (m_collapsed == collapsed) {
        return;
    }

    m_collapsed = collapsed;

    m_progressAnimation->stop();
    m_progressAnimation->setStartValue(m_progress);
    m_progressAnimation->setEndValue(m_collapsed ? 1.0 : 0.0);
    m_progressAnimation->start();
}

void SidebarToggleButton::setProgress(qreal value)
{
    const qreal clamped = std::clamp(value, 0.0, 1.0);

    if (qFuzzyCompare(m_progress, clamped)) {
        return;
    }

    m_progress = clamped;
    update();
}

void SidebarToggleButton::setHoverProgress(qreal value)
{
    const qreal clamped = std::clamp(value, 0.0, 1.0);

    if (qFuzzyCompare(m_hoverProgress, clamped)) {
        return;
    }

    m_hoverProgress = clamped;
    update();
}

void SidebarToggleButton::enterEvent(QEnterEvent *event)
{
    animateHover(1.0);
    QPushButton::enterEvent(event);
}

void SidebarToggleButton::leaveEvent(QEvent *event)
{
    animateHover(0.0);
    QPushButton::leaveEvent(event);
}

void SidebarToggleButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    /*
        Soft hover background. Kept intentionally subtle to match the Notes-like
        light toolbar feel.
    */
    const QColor hoverFill(0, 0, 0, static_cast<int>(18 + 22 * m_hoverProgress));
    painter.setPen(Qt::NoPen);
    painter.setBrush(hoverFill);
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 8, 8);

    /*
        Draw a simple morphing sidebar/chevron glyph manually. This avoids
        depending on icon files or platform-specific symbol availability.
    */
    painter.setPen(QPen(QColor(48, 48, 52), 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    const QRectF r = rect().adjusted(7, 7, -7, -7);
    const qreal cx = r.center().x();
    const qreal cy = r.center().y();

    auto lerp = [](qreal a, qreal b, qreal t) {
        return a + (b - a) * t;
    };

    /*
        Expanded state: a two-column sidebar-like glyph.
        Collapsed state: a right-facing chevron-like glyph.
    */
    const QPointF a1(lerp(cx - 5, cx - 3, m_progress), lerp(cy - 5, cy - 4, m_progress));
    const QPointF b1(lerp(cx - 1, cx + 1, m_progress), cy);
    const QPointF c1(lerp(cx - 5, cx - 3, m_progress), lerp(cy + 5, cy + 4, m_progress));

    const QPointF a2(lerp(cx + 1, cx - 1, m_progress), lerp(cy - 5, cy - 5, m_progress));
    const QPointF b2(lerp(cx + 5, cx + 3, m_progress), cy);
    const QPointF c2(lerp(cx + 1, cx - 1, m_progress), lerp(cy + 5, cy + 5, m_progress));

    painter.drawLine(a1, b1);
    painter.drawLine(b1, c1);
    painter.drawLine(a2, b2);
    painter.drawLine(b2, c2);
}

void SidebarToggleButton::animateHover(qreal endValue)
{
    m_hoverAnimation->stop();
    m_hoverAnimation->setStartValue(m_hoverProgress);
    m_hoverAnimation->setEndValue(endValue);
    m_hoverAnimation->start();
}