#include "previewview.h"
#include "uicolors.h"

#include <QApplication>
#include <QDebug>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QHBoxLayout>
#include <QPainter>
#include <QShortcut>
#include <QToolButton>
#include <QWheelEvent>
#include <QtMath>

namespace {
constexpr qreal kZoomStep = 1.25;      // per wheel notch, button click or key press
constexpr qreal kMaxFitFactor = 8.0;   // zoom in up to 8 × the whole page ...
constexpr int kZoomBarMargin = 10;     // distance of the zoom bar from the corner

// ... but at least to 100 % (small scans)
qreal maxScaleFor(qreal fitScale) {
    return qMax(fitScale * kMaxFitFactor, 1.0);
}
}

// Small bar in the lower right corner: − / zoom level (click: whole page) / +.
// Paints its own rounded background like the settings overlay.
class ZoomBar : public QWidget
{
public:
    explicit ZoomBar(QWidget *parent) : QWidget(parent) {
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(2);
        outButton = addButton(layout, QStringLiteral("−"));
        levelButton = addButton(layout, QString());
        // Room for "8000 %", so the bar does not change its width while zooming
        levelButton->setMinimumWidth(levelButton->fontMetrics().horizontalAdvance("8000 %") + 12);
        inButton = addButton(layout, QStringLiteral("+"));
    }

    QToolButton *outButton = nullptr;
    QToolButton *levelButton = nullptr;
    QToolButton *inButton = nullptr;

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QPalette pal = QApplication::palette();
        QColor fill = pal.color(QPalette::Window);
        fill.setAlpha(230);
        const QColor frame = UiColors::mix(pal.color(QPalette::Window), pal.color(QPalette::WindowText),
                                           UiColors::overlayFrameShade);
        painter.setPen(QPen(frame, 1.0));
        painter.setBrush(fill);
        painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 8, 8);
    }

private:
    QToolButton *addButton(QHBoxLayout *layout, const QString &text) {
        auto *button = new QToolButton(this);
        button->setText(text);
        button->setAutoRaise(true);
        button->setFocusPolicy(Qt::NoFocus);
        layout->addWidget(button);
        return button;
    }
};

PreviewView::PreviewView(QWidget *parent) : QGraphicsView(parent) {
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);   // Ctrl + wheel zooms at the mouse
    setResizeAnchor(QGraphicsView::AnchorViewCenter);

    m_zoomBar = new ZoomBar(this);
    m_zoomBar->outButton->setToolTip(tr("Zoom out (Ctrl + Minus)"));
    m_zoomBar->levelButton->setToolTip(tr("Whole page (Ctrl + 0)"));
    m_zoomBar->inButton->setToolTip(tr("Zoom in (Ctrl + Plus)"));
    connect(m_zoomBar->outButton, &QToolButton::clicked, this, &PreviewView::zoomOut);
    connect(m_zoomBar->levelButton, &QToolButton::clicked, this, &PreviewView::fitPage);
    connect(m_zoomBar->inButton, &QToolButton::clicked, this, &PreviewView::zoomIn);
    m_zoomBar->hide();

    // Keys work in the whole main page (not only when the preview has the focus); a shortcut of
    // a hidden widget (help page shown) does not fire
    auto addShortcut = [this](const QKeySequence &keys, void (PreviewView::*slot)()) {
        auto *shortcut = new QShortcut(keys, this);
        shortcut->setContext(Qt::WindowShortcut);
        connect(shortcut, &QShortcut::activated, this, slot);
    };
    addShortcut(QKeySequence::ZoomIn, &PreviewView::zoomIn);
    addShortcut(QKeySequence::ZoomOut, &PreviewView::zoomOut);
    addShortcut(QKeySequence(Qt::CTRL | Qt::Key_0), &PreviewView::fitPage);
}

bool PreviewView::setImage(const QString &imagePath) {
    QPixmap pixmap(imagePath);
    if (pixmap.isNull()) {
        qDebug() << "Image could not be loaded:" << imagePath;
        return false;
    }

    m_scene->clear();
    m_item = m_scene->addPixmap(pixmap);
    m_item->setTransformationMode(Qt::SmoothTransformation);   // zoomed out it would flicker
    m_scene->setSceneRect(m_item->boundingRect());
    fitPage();   // a new page always starts as a whole page
    if (underMouse()) m_zoomBar->show();
    return true;
}

void PreviewView::clearImage() {
    m_scene->clear();
    m_scene->setSceneRect(QRectF(0, 0, 1, 1));   // the old page's size would keep the scroll bars
    m_item = nullptr;
    m_fitMode = true;
    resetTransform();
    setDragMode(QGraphicsView::NoDrag);
    m_zoomBar->hide();
}

namespace {
// Notice in the empty preview
constexpr qreal kHintTitleScale = 1.0;   // size of the bold title relative to the normal text
}

void PreviewView::setHint(HintState state, const QString &title, const QString &subtitle,
                          const QString &note) {
    const Hint hint{state, title, subtitle, note};
    if (hint.state == m_hint.state && hint.title == m_hint.title && hint.subtitle == m_hint.subtitle
        && hint.note == m_hint.note) {
        return;
    }
    m_hint = hint;
    viewport()->update();
}

// The notice as one block (dot, then the texts), centered
void PreviewView::paintEvent(QPaintEvent *event) {
    QGraphicsView::paintEvent(event);
    if (m_item || m_hint.state == HintState::None) return;

    const QPalette appPal = QApplication::palette();
    const QColor surface = viewport()->palette().color(QPalette::Window);
    const QColor text = appPal.color(QPalette::WindowText);
    const bool light = UiColors::isLight(appPal);

    QFont titleFont = viewport()->font();
    titleFont.setBold(true);
    titleFont.setPointSizeF(titleFont.pointSizeF() * kHintTitleScale);
    const QFont textFont = viewport()->font();
    const QFontMetrics titleFm(titleFont);
    const QFontMetrics textFm(textFont);

    constexpr int kMargin = 24;   // to the edge of the view
    constexpr int kGap = 10;      // dot to text
    constexpr int kLineGap = 2;
    const int dot = qRound(titleFm.height() * 0.5);
    const int maxText = qMax(80, viewport()->width() - 2 * kMargin - dot - kGap);
    auto sizeOf = [maxText](const QFontMetrics &fm, const QString &s) {
        return s.isEmpty() ? QSize(0, 0)
                           : fm.boundingRect(QRect(0, 0, maxText, 10000), Qt::TextWordWrap, s).size();
    };
    const QSize titleSize = sizeOf(titleFm, m_hint.title);
    const QSize subSize = sizeOf(textFm, m_hint.subtitle);
    const QSize noteSize = sizeOf(textFm, m_hint.note);
    const int textW = qMax(titleSize.width(), qMax(subSize.width(), noteSize.width()));
    int textH = titleSize.height();
    if (subSize.height() > 0) textH += kLineGap + subSize.height();
    if (noteSize.height() > 0) textH += 3 * kLineGap + noteSize.height();

    const QSize blockSize(dot + kGap + textW, textH);
    const QRect block(QPoint((viewport()->width() - blockSize.width()) / 2,
                             (viewport()->height() - blockSize.height()) / 2), blockSize);

    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing);

    // Dot: green ready, orange stand-in scanner, red no scanner (a different tone per mode)
    QColor dotColor;
    switch (m_hint.state) {
    case HintState::Ready:   dotColor = light ? QColor(30, 140, 60)  : QColor(80, 200, 110);  break;
    case HintState::Warning: dotColor = light ? QColor(205, 120, 0)  : QColor(240, 170, 60);  break;
    default:                 dotColor = light ? QColor(200, 40, 40)  : QColor(240, 100, 100); break;
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(dotColor);
    painter.drawEllipse(QRectF(block.left(), block.top() + (titleFm.height() - dot) / 2.0, dot, dot));

    // Texts, left aligned
    const int x = block.left() + dot + kGap;
    int y = block.top();
    auto drawBlock = [&](const QFont &font, const QColor &color, const QString &s, const QSize &size) {
        painter.setFont(font);
        painter.setPen(color);
        painter.drawText(QRect(x, y, textW, size.height()), Qt::AlignLeft | Qt::TextWordWrap, s);
        y += size.height();
    };
    const QColor soft = UiColors::mix(surface, text, 0.65);
    drawBlock(titleFont, text, m_hint.title, titleSize);
    if (subSize.height() > 0) { y += kLineGap; drawBlock(textFont, soft, m_hint.subtitle, subSize); }
    if (noteSize.height() > 0) { y += 3 * kLineGap; drawBlock(textFont, soft, m_hint.note, noteSize); }
}

// ====== Zoom ======

qreal PreviewView::currentScale() const {
    return transform().m11();
}

// Like fitInView() (2 px margin), measured without scroll bars: they vanish when fitted
qreal PreviewView::fitScale() const {
    if (!m_item) return 1.0;
    const QRectF image = m_item->boundingRect();
    const QSize area = maximumViewportSize();
    if (image.isEmpty() || area.isEmpty()) return 1.0;
    return qMin((area.width() - 4) / image.width(), (area.height() - 4) / image.height());
}

// Not fitInView(): it measures the viewport while the scroll bars of the zoomed page are still
// there, the page would stay smaller by their width once they vanish
void PreviewView::fitPage() {
    m_fitMode = true;
    setDragMode(QGraphicsView::NoDrag);
    if (m_item) {
        const qreal fit = fitScale();
        resetTransform();
        QGraphicsView::scale(fit, fit);
        centerOn(m_item);
    }
    updateZoomBar();
}

// Zooming down to the whole page switches back to the fit mode
void PreviewView::setScale(qreal scale, bool atMouse) {
    if (!m_item) return;
    const qreal fit = fitScale();
    scale = qMin(scale, maxScaleFor(fit));
    if (scale <= fit * 1.001) {
        fitPage();
        return;
    }
    m_fitMode = false;
    setTransformationAnchor(atMouse ? QGraphicsView::AnchorUnderMouse : QGraphicsView::AnchorViewCenter);
    const qreal factor = scale / currentScale();
    QGraphicsView::scale(factor, factor);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::ScrollHandDrag);   // drag moves the zoomed page
    updateZoomBar();
}

void PreviewView::zoomBy(qreal factor, bool atMouse) {
    setScale(currentScale() * factor, atMouse);
}

void PreviewView::zoomIn() {
    if (isVisible()) zoomBy(kZoomStep, false);
}

void PreviewView::zoomOut() {
    if (isVisible()) zoomBy(1.0 / kZoomStep, false);
}

void PreviewView::updateZoomBar() {
    const qreal scale = currentScale();
    m_zoomBar->levelButton->setText(QString("%1 %").arg(qRound(scale * 100)));
    m_zoomBar->outButton->setEnabled(m_item && !m_fitMode);
    m_zoomBar->inButton->setEnabled(m_item && scale < maxScaleFor(fitScale()) * 0.999);
    placeZoomBar();
}

void PreviewView::placeZoomBar() {
    m_zoomBar->adjustSize();
    const QRect area = viewport()->geometry();   // without scroll bars
    m_zoomBar->move(area.right() - m_zoomBar->width() - kZoomBarMargin,
                    area.bottom() - m_zoomBar->height() - kZoomBarMargin);
    m_zoomBar->raise();
}

// ====== Events ======

void PreviewView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    if (m_fitMode) {
        fitPage();   // the whole page follows the size; zoomed, the zoom level stays
    } else {
        updateZoomBar();
    }
}

// Ctrl + wheel zooms at the mouse, the wheel alone scrolls (zoomed)
void PreviewView::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        const qreal notches = event->angleDelta().y() / 120.0;
        if (m_item && notches != 0) zoomBy(qPow(kZoomStep, notches), true);
        event->accept();
        return;
    }
    QGraphicsView::wheelEvent(event);
}

// Whole page <-> 100 % at the clicked point (small scans: twice the whole page)
void PreviewView::mouseDoubleClickEvent(QMouseEvent *event) {
    if (!m_item || event->button() != Qt::LeftButton) {
        QGraphicsView::mouseDoubleClickEvent(event);
        return;
    }
    if (m_fitMode) {
        const qreal fit = fitScale();
        setScale(fit < 1.0 ? 1.0 : fit * 2.0, true);
    } else {
        fitPage();
    }
    event->accept();
}

void PreviewView::enterEvent(QEnterEvent *event) {
    QGraphicsView::enterEvent(event);
    if (m_item) {
        placeZoomBar();
        m_zoomBar->show();
    }
}

void PreviewView::leaveEvent(QEvent *event) {
    QGraphicsView::leaveEvent(event);
    m_zoomBar->hide();
}
