#pragma once

#include <QGraphicsView>

class QGraphicsPixmapItem;

// Large preview of one page. The image always fits the view, also after resizing
// (window, splitter).
class PreviewView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit PreviewView(QWidget *parent = nullptr);

    bool setImage(const QString &imagePath);
    void clearImage();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void fitImage();

    QGraphicsScene *m_scene = nullptr;
    QGraphicsPixmapItem *m_item = nullptr;
};
