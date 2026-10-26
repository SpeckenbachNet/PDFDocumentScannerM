#include "previewview.h"
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>

PreviewView::PreviewView(QWidget *parent) : QGraphicsView(parent) {
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
}

bool PreviewView::setImage(const QString &imagePath) {
    QPixmap pixmap(imagePath);
    if (pixmap.isNull()) {
        qDebug() << "Fehler: Bild konnte nicht geladen werden!";
        return false;
    }

    m_scene->clear();
    m_item = m_scene->addPixmap(pixmap);
    m_scene->setSceneRect(m_item->boundingRect());
    fitImage();
    return true;
}

void PreviewView::clearImage() {
    m_scene->clear();
    m_item = nullptr;
}

void PreviewView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    fitImage();
}

void PreviewView::fitImage() {
    if (m_item) fitInView(m_item, Qt::KeepAspectRatio);
}
