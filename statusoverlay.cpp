#include "statusoverlay.h"

StatusOverlay::StatusOverlay(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    hide();

    m_label = new QLabel(this);
    m_label->setAlignment(Qt::AlignCenter);

    // Effect for pulsing
    auto *eff = new QGraphicsOpacityEffect(m_label);
    m_label->setGraphicsEffect(eff);

    m_anim = new QPropertyAnimation(eff, "opacity", this);
    m_anim->setDuration(3000); // A total of 3 seconds for a full “inhalation and exhalation”
    m_anim->setLoopCount(-1);

    // define the turning point at 50% of the time
    m_anim->setStartValue(1.0);
    m_anim->setKeyValueAt(0.5, 0.55);   // It is darkest halfway through the time
    m_anim->setEndValue(1.0);           // Fully bright again at the end
    m_anim->setEasingCurve(QEasingCurve::InOutSine);

    // Timer for automatic fading out
    m_autoHideTimer = new QTimer(this);
    m_autoHideTimer->setSingleShot(true);
    connect(m_autoHideTimer, &QTimer::timeout, this, &StatusOverlay::hideOverlay);
}

void StatusOverlay::showMessage(const QString &text, bool isError, int timeoutMs) {
    m_autoHideTimer->stop(); // Stop timer
    m_anim->stop();
    // Manually reset opacity to 1.0 (fully visible)
    // This prevents the label from getting stuck in a “semi-transparent” state.
    if (m_label->graphicsEffect()) {
        static_cast<QGraphicsOpacityEffect*>(m_label->graphicsEffect())->setOpacity(1.0);
    }

    m_label->setText(text);
    // Customize stylesheet (red for errors, black for status)
    m_label->setStyleSheet(isError
       ? "background: rgba(0, 0, 0, 160); color: red; border-radius: 15px; padding: 25px; font-weight: bold; font-size: 18px;"
       : "background: rgba(0, 0, 0, 160); color: white; border-radius: 15px; padding: 25px; font-weight: bold; font-size: 18px;");

    // Positioning
    this->resize(parentWidget()->size());
    m_label->adjustSize();
    centerLabel();

    show();

    if (!isError) {
        QTimer::singleShot(50, m_anim, [this]() {
            m_anim->start();
        });
    }

    // If a timeout has been set (e.g., 3000 ms), start the timer.
    if (timeoutMs > 0) {
        m_autoHideTimer->start(timeoutMs);
    }
}

void StatusOverlay::centerLabel() {
    m_label->move((width() - m_label->width()) / 2, (height() - m_label->height()) / 2);
}

// The first message is shown before the main window has its final size,
// so the label has to follow every resize of the overlay
void StatusOverlay::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    centerLabel();
}

void StatusOverlay::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0, 50)); // Very slight darkening of the rest
}

void StatusOverlay::hideOverlay() {
    m_anim->stop();
    hide();
}
