#pragma once

#include <QHash>
#include <QListWidget>

class PageDelegate;
class QTimer;
class QUndoStack;

// Column of scanned pages. Each page fills the list width (A4 ratio) and the sizes follow
// every resize of the list itself (window, splitter, scroll bar).
// Select with click, Ctrl / Shift + click, Ctrl+A; remove with the × over a page, Delete or
// the context menu; reorder by drag and drop; rotate; crop.
// Undo / redo (Ctrl+Z, Ctrl+Shift+Z, context menu) for removing, clearing, reordering, rotating
// and cropping. Removed pages keep their image files until the app closes (MainWin removes the
// scan folders), rotate and crop keep copies of the images (undo_* next to the page).
class PageListWidget : public QListWidget
{
    Q_OBJECT

public:
    explicit PageListWidget(QWidget *parent = nullptr);
    ~PageListWidget() override;

    struct Page {
        QString filePath;
        int dpi = 0;   // resolution of the scan, 0: unknown
    };

    // Adds the image as a new page; the file path is stored in Qt::UserRole, the scan
    // resolution in kDpiRole, the angle it was straightened by in kStraightenedRole (0: not
    // straightened; the label says so)
    bool addPage(const QString &filePath, int dpi, double straightenedBy = 0);

    // Pages in list order: all or only the selected ones
    QList<Page> pages(bool selectedOnly) const;
    bool hasSelection() const { return !selectedItems().isEmpty(); }

    void rotateSelectedPages();   // 90° clockwise, the image file is rewritten
    // Cuts the page with this image file to rect (image pixels), the file is rewritten.
    // false: page gone or the image could not be written.
    bool cropPage(const QString &filePath, const QRect &rect);
    void removeAllPages();        // undoable, the image files stay until the app closes

    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;
    // MainWin: no undo / redo while scanning, saving or cropping
    void setUndoAllowed(bool allowed) { m_undoAllowed = allowed; }

    static constexpr int kDpiRole = Qt::UserRole + 1;
    static constexpr int kStraightenedRole = Qt::UserRole + 2;

signals:
    void pagesChanged();       // pages added, removed, moved or rotated
    void pageImagesChanged();  // image files rewritten (rotated): a shown page must be reloaded
    void undoMessage(const QString &message);   // "Undone: Crop", for the status overlay

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
    void removeRows(QList<int> rows, const QString &undoText);
    void removeSelectedPages();
    void renumber();
    void setPageLabel(QListWidgetItem *page, int number);   // "Page 3", plus the straightening
    int crossRowAt(const QPoint &pos) const;   // row whose × is at pos, -1: none
    void setCrossHoverRow(int row);
    int dragScrollStep() const;   // pixels per timer tick at m_dragPos, 0: outside the edge zones
    void dragScroll();

    // Undo helpers. Pages are known by their file path.
    QStringList order() const;
    QListWidgetItem *findPage(const QString &filePath) const;   // in the list or taken out
    // Brings the list to the order target: pages of managed that are not in target are taken
    // out (kept for an undo), pages of neither stay behind them (scanned meanwhile).
    // Afterwards the pages of select are selected.
    void applyOrder(const QStringList &target, const QStringList &managed, const QStringList &select);
    // Copies of the image files of the pages (and their gray originals) before and after a
    // change of the images, for undo and redo
    struct ImageCopy {
        QString file;     // page image or gray original
        QString before;   // copy before the change
        QString after;    // copy after the change
    };
    QList<ImageCopy> copyImages(const QStringList &pages);   // the "before" copies
    void copyImagesAfter(QList<ImageCopy> &copies);
    void restoreImages(const QList<ImageCopy> &copies, bool after, const QStringList &pages);
    void pushImageChange(const QString &text, const QList<ImageCopy> &copies, const QStringList &pages);
    void selectPages(const QStringList &pages);

    PageDelegate *m_delegate = nullptr;
    int m_crossHoverRow = -1;     // the mouse is over the × of this row
    int m_pressedCrossRow = -1;   // the click began on the × of this row: removes on release

    QTimer *m_dragScrollTimer = nullptr;   // scrolls while a dragged page is near the top or bottom

    QUndoStack *m_undoStack = nullptr;
    QHash<QString, QListWidgetItem *> m_takenOut;   // removed pages, kept for an undo
    int m_copyCounter = 0;                          // numbers the undo_* copies
    bool m_undoAllowed = true;
    QPoint m_dragPos;                      // last drag position in the viewport
};
