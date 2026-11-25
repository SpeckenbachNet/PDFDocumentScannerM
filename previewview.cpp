#include "previewview.h"
#include "pageprocessing.h"
#include "uicolors.h"

#include <QApplication>
#include <QDebug>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
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

// Small floating bar with flat buttons. Paints its own rounded background like the settings
// overlay.
class FloatingBar : public QWidget
{
public:
    explicit FloatingBar(QWidget *parent) : QWidget(parent) {
        m_layout = new QHBoxLayout(this);
        m_layout->setContentsMargins(4, 4, 4, 4);
        m_layout->setSpacing(2);
    }

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

    QToolButton *addButton(const QString &text) {
        auto *button = new QToolButton(this);
        button->setText(text);
        button->setAutoRaise(true);
        button->setFocusPolicy(Qt::NoFocus);
        m_layout->addWidget(button);
        return button;
    }

private:
    QHBoxLayout *m_layout = nullptr;
};

// Lower right corner, while the mouse is over the view: − / zoom level (click: whole page) / +
class ZoomBar : public FloatingBar
{
public:
    explicit ZoomBar(QWidget *parent) : FloatingBar(parent) {
        outButton = addButton(QStringLiteral("−"));
        levelButton = addButton(QString());
        // Room for "8000 %", so the bar does not change its width while zooming
        levelButton->setMinimumWidth(levelButton->fontMetrics().horizontalAdvance("8000 %") + 12);
        inButton = addButton(QStringLiteral("+"));
    }

    QToolButton *outButton = nullptr;
    QToolButton *levelButton = nullptr;
    QToolButton *inButton = nullptr;
};

// Lower left corner, while cropping: Apply / Cancel
class CropBar : public FloatingBar
{
public:
    explicit CropBar(QWidget *parent) : FloatingBar(parent) {
        applyButton = addButton(PreviewView::tr("Apply"));
        cancelButton = addButton(PreviewView::tr("Cancel"));
    }

    QToolButton *applyButton = nullptr;
    QToolButton *cancelButton = nullptr;
};

namespace {
// Crop frame: which edges a drag moves
enum CropHandle {
    CropLeft = 1, CropRight = 2, CropTop = 4, CropBottom = 8,
    CropMove = 16,   // inside: the whole frame
    CropNew = 32     // outside: a new frame from the press position
};
constexpr int kCropGrip = 8;          // view pixels around an edge or corner that grab it
constexpr int kCropHandleSize = 10;   // view pixels, the squares on corners and edges
constexpr qreal kCropMinSize = 16;    // image pixels, the frame does not get smaller
constexpr int kCropNewStart = 4;      // view pixels the mouse moves before a new frame starts
}

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

    m_cropBar = new CropBar(this);
    m_cropBar->applyButton->setToolTip(tr("Cut the page to the frame (Enter)"));
    m_cropBar->cancelButton->setToolTip(tr("Leave the page as it is (Esc)"));
    connect(m_cropBar->applyButton, &QToolButton::clicked, this, &PreviewView::acceptCrop);
    connect(m_cropBar->cancelButton, &QToolButton::clicked, this, &PreviewView::cancelCrop);
    m_cropBar->hide();
    // Enter applies; only while cropping, so it takes the key from nobody else
    for (const QKeySequence &keys : {QKeySequence(Qt::Key_Return), QKeySequence(Qt::Key_Enter)}) {
        auto *shortcut = new QShortcut(keys, this);
        shortcut->setContext(Qt::WindowShortcut);
        shortcut->setEnabled(false);
        connect(shortcut, &QShortcut::activated, this, &PreviewView::acceptCrop);
        m_cropShortcuts.append(shortcut);
    }
}

bool PreviewView::setImage(const QString &imagePath) {
    QPixmap pixmap(imagePath);
    if (pixmap.isNull()) {
        qDebug() << "Image could not be loaded:" << imagePath;
        return false;
    }

    endCrop();   // a new page (or the cropped one again) ends the crop mode
    m_imagePath = imagePath;
    m_scene->clear();
    m_item = m_scene->addPixmap(pixmap);
    m_item->setTransformationMode(Qt::SmoothTransformation);   // zoomed out it would flicker
    m_scene->setSceneRect(m_item->boundingRect());
    fitPage();   // a new page always starts as a whole page
    if (underMouse()) m_zoomBar->show();
    return true;
}

void PreviewView::clearImage() {
    endCrop();
    m_imagePath.clear();
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
    // Drag moves the zoomed page, while cropping it changes the frame
    setDragMode(m_cropping ? QGraphicsView::NoDrag : QGraphicsView::ScrollHandDrag);
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
    if (m_cropping) placeCropBar();
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

// ====== Crop ======

bool PreviewView::startCrop() {
    if (!m_item || m_cropping) return m_cropping;
    const QImage image(m_imagePath);
    if (image.isNull()) return false;
    m_cropRect = QRectF(PageProcessing::contentRect(image));
    m_cropping = true;
    m_cropHandle = 0;
    setDragMode(QGraphicsView::NoDrag);
    for (QShortcut *shortcut : m_cropShortcuts) shortcut->setEnabled(true);
    placeCropBar();
    m_cropBar->show();
    viewport()->update();
    emit croppingChanged(true);
    return true;
}

void PreviewView::cancelCrop() {
    endCrop();
}

void PreviewView::acceptCrop() {
    if (!m_cropping) return;
    const QString path = m_imagePath;
    const QRect rect = m_cropRect.toRect().intersected(m_item->pixmap().rect());
    const bool whole = rect == m_item->pixmap().rect();
    endCrop();
    if (!whole && !rect.isEmpty()) emit cropAccepted(path, rect);
}

void PreviewView::endCrop() {
    if (!m_cropping) return;
    m_cropping = false;
    m_cropHandle = 0;
    for (QShortcut *shortcut : m_cropShortcuts) shortcut->setEnabled(false);
    m_cropBar->hide();
    viewport()->unsetCursor();
    setDragMode(m_fitMode ? QGraphicsView::NoDrag : QGraphicsView::ScrollHandDrag);
    viewport()->update();
    emit croppingChanged(false);
}

void PreviewView::placeCropBar() {
    m_cropBar->adjustSize();
    const QRect area = viewport()->geometry();
    m_cropBar->move(area.left() + kZoomBarMargin, area.bottom() - m_cropBar->height() - kZoomBarMargin);
    m_cropBar->raise();
}

// Corners before edges, edges before the inside; measured in the view, so the grip stays the
// same size at every zoom level
int PreviewView::cropHandleAt(const QPoint &viewPos) const {
    const QRect frame = mapFromScene(m_cropRect).boundingRect();
    const QRect grab = frame.adjusted(-kCropGrip, -kCropGrip, kCropGrip, kCropGrip);
    if (!grab.contains(viewPos)) return 0;
    int handle = 0;
    if (qAbs(viewPos.x() - frame.left()) <= kCropGrip) handle |= CropLeft;
    else if (qAbs(viewPos.x() - frame.right()) <= kCropGrip) handle |= CropRight;
    if (qAbs(viewPos.y() - frame.top()) <= kCropGrip) handle |= CropTop;
    else if (qAbs(viewPos.y() - frame.bottom()) <= kCropGrip) handle |= CropBottom;
    if (handle) return handle;
    return frame.contains(viewPos) ? CropMove : 0;
}

void PreviewView::updateCropCursor(int handle) {
    Qt::CursorShape shape = Qt::CrossCursor;   // outside: draws a new frame
    if (handle == CropMove) shape = Qt::SizeAllCursor;
    else if (handle == (CropLeft | CropTop) || handle == (CropRight | CropBottom)) shape = Qt::SizeFDiagCursor;
    else if (handle == (CropRight | CropTop) || handle == (CropLeft | CropBottom)) shape = Qt::SizeBDiagCursor;
    else if (handle & (CropLeft | CropRight)) shape = Qt::SizeHorCursor;
    else if (handle & (CropTop | CropBottom)) shape = Qt::SizeVerCursor;
    viewport()->setCursor(shape);
}

void PreviewView::mousePressEvent(QMouseEvent *event) {
    if (!m_cropping || event->button() != Qt::LeftButton) {
        QGraphicsView::mousePressEvent(event);
        return;
    }
    const QPoint pos = event->position().toPoint();
    m_cropHandle = cropHandleAt(pos);
    if (!m_cropHandle) m_cropHandle = CropNew;
    m_cropPressViewPos = pos;
    m_cropPressPos = mapToScene(pos);
    m_cropPressRect = m_cropRect;
    event->accept();
}

void PreviewView::mouseMoveEvent(QMouseEvent *event) {
    if (!m_cropping) {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }
    const QPoint pos = event->position().toPoint();
    if (!m_cropHandle) {   // hover
        updateCropCursor(cropHandleAt(pos));
        return;
    }

    const QRectF bounds = m_item->boundingRect();
    auto clampX = [&](qreal x) { return qBound(bounds.left(), qreal(qRound(x)), bounds.right()); };
    auto clampY = [&](qreal y) { return qBound(bounds.top(), qreal(qRound(y)), bounds.bottom()); };
    const QPointF scenePos = mapToScene(pos);
    const QPointF delta = scenePos - m_cropPressPos;
    QRectF r = m_cropPressRect;

    if (m_cropHandle == CropNew) {
        if ((pos - m_cropPressViewPos).manhattanLength() < kCropNewStart) return;
        const QPointF a(clampX(m_cropPressPos.x()), clampY(m_cropPressPos.y()));
        const QPointF b(clampX(scenePos.x()), clampY(scenePos.y()));
        r = QRectF(a, b).normalized();
        if (r.width() < kCropMinSize || r.height() < kCropMinSize) return;
    } else if (m_cropHandle == CropMove) {
        const qreal dx = qBound(bounds.left() - r.left(), qreal(qRound(delta.x())), bounds.right() - r.right());
        const qreal dy = qBound(bounds.top() - r.top(), qreal(qRound(delta.y())), bounds.bottom() - r.bottom());
        r.translate(dx, dy);
    } else {
        if (m_cropHandle & CropLeft)   r.setLeft(qMin(clampX(r.left() + delta.x()), r.right() - kCropMinSize));
        if (m_cropHandle & CropRight)  r.setRight(qMax(clampX(r.right() + delta.x()), r.left() + kCropMinSize));
        if (m_cropHandle & CropTop)    r.setTop(qMin(clampY(r.top() + delta.y()), r.bottom() - kCropMinSize));
        if (m_cropHandle & CropBottom) r.setBottom(qMax(clampY(r.bottom() + delta.y()), r.top() + kCropMinSize));
    }
    m_cropRect = r;
    viewport()->update();
    event->accept();
}

void PreviewView::mouseReleaseEvent(QMouseEvent *event) {
    if (!m_cropping || event->button() != Qt::LeftButton) {
        QGraphicsView::mouseReleaseEvent(event);
        return;
    }
    m_cropHandle = 0;
    updateCropCursor(cropHandleAt(event->position().toPoint()));
    event->accept();
}

// Outside the frame darkened, the frame and its handles in the accent color; sizes in view
// pixels, so they look the same at every zoom level
void PreviewView::drawForeground(QPainter *painter, const QRectF &rect) {
    QGraphicsView::drawForeground(painter, rect);
    if (!m_cropping || !m_item) return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QPainterPath outside;
    outside.addRect(m_item->boundingRect());
    QPainterPath inside;
    inside.addRect(m_cropRect);
    painter->fillPath(outside.subtracted(inside), QColor(0, 0, 0, 120));

    const QColor accent = QApplication::palette().color(QPalette::Highlight);
    QPen pen(accent, 2);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(m_cropRect);

    const qreal size = kCropHandleSize / currentScale();
    const QRectF r = m_cropRect;
    const QList<QPointF> points = {
        r.topLeft(), r.topRight(), r.bottomLeft(), r.bottomRight(),
        QPointF(r.center().x(), r.top()), QPointF(r.center().x(), r.bottom()),
        QPointF(r.left(), r.center().y()), QPointF(r.right(), r.center().y())
    };
    QPen handlePen(accent, 1.5);
    handlePen.setCosmetic(true);
    painter->setPen(handlePen);
    painter->setBrush(Qt::white);
    for (const QPointF &p : points) {
        painter->drawRect(QRectF(p.x() - size / 2, p.y() - size / 2, size, size));
    }
    painter->restore();
}
