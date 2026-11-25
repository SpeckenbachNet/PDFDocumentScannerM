#pragma once

#include <QGraphicsView>

class QGraphicsPixmapItem;
class QLabel;
class QShortcut;
class CropBar;
class ZoomBar;

// Large preview of one page. A new page shows the whole page, which follows every resize
// (window, splitter). Zoom: Ctrl + mouse wheel (at the mouse), Ctrl + Plus / Minus / 0,
// double click (whole page <-> 100 %), or the zoom bar that appears while the mouse is over the
// view. Zoomed, the zoom level is kept on resize; the mouse wheel scrolls, dragging moves the page.
// Crop mode (startCrop): a frame on the page, outside darkened; corners and edges resize it,
// dragging inside moves it, dragging outside draws a new one. Enter or "Apply" emits
// cropAccepted, Esc (MainWin) or "Cancel" ends it; a new page or an empty preview ends it too.
// Zooming works as before, dragging then changes the frame instead of moving the page.
class PreviewView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit PreviewView(QWidget *parent = nullptr);

    bool setImage(const QString &imagePath);
    void clearImage();
    // Notice in the middle while no page is shown: a colored dot (state), a bold title, a
    // subtitle and an optional note below. None: nothing is shown.
    enum class HintState { None, Ready, Warning, Problem };
    void setHint(HintState state, const QString &title = QString(),
                 const QString &subtitle = QString(), const QString &note = QString());

    void zoomIn();
    void zoomOut();
    void fitPage();   // whole page, follows resizes again

    // Crop mode for the shown page; the frame starts around the content (PageProcessing::
    // contentRect). false: no page shown.
    bool startCrop();
    void cancelCrop();
    void acceptCrop();   // emits cropAccepted, unless the frame is the whole page
    bool isCropping() const { return m_cropping; }

signals:
    // rect in image pixels of the file imagePath; the caller cuts the file
    void cropAccepted(const QString &imagePath, const QRect &rect);
    void croppingChanged(bool cropping);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void drawForeground(QPainter *painter, const QRectF &rect) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    qreal currentScale() const;   // 1.0 = one image pixel per screen pixel
    qreal fitScale() const;       // the whole page fits the view
    void setScale(qreal scale, bool atMouse);
    void zoomBy(qreal factor, bool atMouse);
    void updateZoomBar();
    void placeZoomBar();
    void placeCropBar();
    void endCrop();
    int cropHandleAt(const QPoint &viewPos) const;   // CropHandle flags, 0: outside the frame
    void updateCropCursor(int handle);

    QGraphicsScene *m_scene = nullptr;
    QGraphicsPixmapItem *m_item = nullptr;
    bool m_fitMode = true;        // whole page shown: resizes fit it again
    ZoomBar *m_zoomBar = nullptr;
    QString m_imagePath;          // file of the shown page

    // Crop mode
    bool m_cropping = false;
    QRectF m_cropRect;            // in image pixels (= scene coordinates)
    int m_cropHandle = 0;         // what the mouse drags, CropHandle flags
    QPointF m_cropPressPos;       // scene position of the mouse press
    QPoint m_cropPressViewPos;    // the same in the view (a new frame starts after a few pixels)
    QRectF m_cropPressRect;       // the frame at the mouse press
    CropBar *m_cropBar = nullptr;
    QList<QShortcut *> m_cropShortcuts;   // Enter: only enabled while cropping
    struct Hint {
        HintState state = HintState::None;
        QString title, subtitle, note;
    } m_hint;
};
