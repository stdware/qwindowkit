#include "captionbuttonstrip.h"

#include <QHBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>

namespace {

    constexpr int kButtonWidth = 46;
    constexpr int kButtonHeight = 32;

    enum class CaptionRole {
        Minimize,
        Maximize,
        Close
    };

    class CaptionButton final : public QPushButton
    {
    public:
        explicit CaptionButton(CaptionRole role, QWidget *parent = nullptr)
            : QPushButton(parent), m_role(role)
        {
            setFocusPolicy(Qt::NoFocus);
            setFlat(true);
            setCursor(Qt::ArrowCursor);
            setMouseTracking(true);
            setFixedSize(kButtonWidth, kButtonHeight);
        }

        void setMaximizedState(bool maximized)
        {
            if (m_maximized == maximized) {
                return;
            }

            m_maximized = maximized;
            update();
        }

    protected:
        void paintEvent(QPaintEvent *event) override
        {
            Q_UNUSED(event);

            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing, true);

            const bool hovered = underMouse();
            const bool pressed = isDown();
            const bool close = m_role == CaptionRole::Close;

            if (hovered || pressed) {
                QColor fill(0, 0, 0, pressed ? 32 : 20);

                if (close) {
                    fill = pressed ? QColor(153, 27, 18) : QColor(232, 17, 35);
                }

                painter.fillRect(rect(), fill);
            }

            const QColor glyphColor =
                close && (hovered || pressed) ? QColor(Qt::white) : QColor(44, 44, 48);

            /*
                Segoe Fluent Icons / Segoe MDL2 Assets exist on modern Windows.
                This keeps the demo resource-free.
            */
            QFont font;
            font.setFamilies({
                QStringLiteral("Segoe Fluent Icons"),
                QStringLiteral("Segoe MDL2 Assets")
            });
            font.setPixelSize(10);
            font.setStyleStrategy(QFont::PreferNoShaping);

            QString glyph;

            switch (m_role) {
                case CaptionRole::Minimize:
                    glyph = QString(QChar(0xE921));
                    break;
                case CaptionRole::Maximize:
                    glyph = QString(QChar(m_maximized ? 0xE923 : 0xE922));
                    break;
                case CaptionRole::Close:
                    glyph = QString(QChar(0xE8BB));
                    break;
            }

            painter.setFont(font);
            painter.setPen(glyphColor);
            painter.drawText(rect(), Qt::AlignCenter, glyph);
        }

    private:
        CaptionRole m_role;
        bool m_maximized = false;
    };

} // namespace

CaptionButtonStrip::CaptionButtonStrip(QWidget *parent)
    : QWidget(parent)
{
    /*
        This is an overlay widget. It is not part of any pane titlebar.

         Reason:
         Windows has only one native top-level window. Minimize/maximize/close
         are window-level semantics, not pane-level semantics.
     */
    setObjectName(QStringLiteral("captionButtonStrip"));
    setFixedSize(preferredWidth(), preferredHeight());

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_minimizeButton = new CaptionButton(CaptionRole::Minimize, this);
    m_maximizeButton = new CaptionButton(CaptionRole::Maximize, this);
    m_closeButton = new CaptionButton(CaptionRole::Close, this);

    layout->addWidget(m_minimizeButton);
    layout->addWidget(m_maximizeButton);
    layout->addWidget(m_closeButton);
}

int CaptionButtonStrip::preferredWidth() const noexcept
{
    return kButtonWidth * 3;
}

int CaptionButtonStrip::preferredHeight() const noexcept
{
    return kButtonHeight;
}

void CaptionButtonStrip::syncWindowState(bool maximized)
{
    /*
        m_maximizeButton is created by us as CaptionButton, so static_cast is
        correct. Do not use qobject_cast here because this private helper class
        intentionally has no Q_OBJECT macro.
    */
    auto *button = static_cast<CaptionButton *>(m_maximizeButton);

    if (button) {
        button->setMaximizedState(maximized);
    }
}