#pragma once

#include <QListWidget>

class PageDelegate;
class QTimer;

// Column of scanned pages. Each page fills the list width (A4 ratio) and the sizes follow
// every resize of the list itself (window, splitter, scroll bar).
// Select with click, Ctrl / Shift + click, Ctrl+A; remove with the × over a page, Delete or
// the context menu (the image file is deleted too); reorder by drag and drop; rotate.
class PageListWidget : public QListWidget
{
    Q_OBJECT

public:
    explicit PageListWidget(QWidget *parent = nullptr);

    struct Page {
        QString filePath;
        int dpi = 0;   // resolution of the scan, 0: unknown
    };

    // Adds the image as a new page; the file path is stored in Qt::UserRole, the scan
    // resolution in kDpiRole
    bool addPage(const QString &filePath, int dpi);

    // Pages in list order: all or only the selected ones
    QList<Page> pages(bool selectedOnly) const;
    bool hasSelection() const { return !selectedItems().isEmpty(); }

    void rotateSelectedPages();   // 90° clockwise, the image file is rewritten
    void removeAllPages();        // the image files are deleted too

    static constexpr int kDpiRole = Qt::UserRole + 1;

signals:
    void pagesChanged();       // pages added, removed, moved or rotated
    void pageImagesChanged();  // image files rewritten (rotated): a shown page must be reloaded

protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void updateItemSizes();
    void removeRows(QList<int> rows);
    void removeSelectedPages();
    void renumber();
    int crossRowAt(const QPoint &pos) const;   // row whose × is at pos, -1: none
    void setCrossHoverRow(int row);
    int dragScrollStep() const;   // pixels per timer tick at m_dragPos, 0: outside the edge zones
    void dragScroll();

    PageDelegate *m_delegate = nullptr;
    int m_crossHoverRow = -1;     // the mouse is over the × of this row
    int m_pressedCrossRow = -1;   // the click began on the × of this row: removes on release

    QTimer *m_dragScrollTimer = nullptr;   // scrolls while a dragged page is near the top or bottom
    QPoint m_dragPos;                      // last drag position in the viewport
};
