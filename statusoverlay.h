#pragma once

#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QPainter>
#include <QPropertyAnimation>
#include <QTimer>
#include <QWidget>

class StatusOverlay : public QWidget {
    Q_OBJECT
public:
    explicit StatusOverlay(QWidget *parent = nullptr);

    // timeoutMs = 0 bedeutet: Manuell schließen (unendlich zeigen)
    void showMessage(const QString &text, bool isError = false, int timeoutMs = 0);
    void hideOverlay();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void layoutLabel();
    void centerLabel();

    QLabel *m_label;
    QPropertyAnimation *m_anim;
    QTimer *m_autoHideTimer;
};
