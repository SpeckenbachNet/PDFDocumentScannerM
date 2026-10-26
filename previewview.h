#pragma once

#include <QGraphicsView>

class QGraphicsPixmapItem;
class QLabel;
class ZoomBar;

// Large preview of one page. A new page shows the whole page, which follows every resize
// (window, splitter). Zoom: Ctrl + mouse wheel (at the mouse), Ctrl + Plus / Minus / 0,
// double click (whole page <-> 100 %), or the zoom bar that appears while the mouse is over the
// view. Zoomed, the zoom level is kept on resize; the mouse wheel scrolls, dragging moves the page.
class PreviewView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit PreviewView(QWidget *parent = nullptr);

    bool setImage(const QString &imagePath);
    void clearImage();

    void zoomIn();
    void zoomOut();
    void fitPage();   // whole page, follows resizes again

protected:
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    qreal currentScale() const;   // 1.0 = one image pixel per screen pixel
    qreal fitScale() const;       // the whole page fits the view
    void setScale(qreal scale, bool atMouse);
    void zoomBy(qreal factor, bool atMouse);
    void updateZoomBar();
    void placeZoomBar();

    QGraphicsScene *m_scene = nullptr;
    QGraphicsPixmapItem *m_item = nullptr;
    bool m_fitMode = true;        // whole page shown: resizes fit it again
    ZoomBar *m_zoomBar = nullptr;
};
