#include "pagelistwidget.h"
#include "pagedelegate.h"
#include "pageprocessing.h"

#include <QContextMenuEvent>
#include <QDragMoveEvent>
#include <QFile>
#include <QImage>
#include <QMenu>
#include <QScrollBar>
#include <QTimer>
#include <QTransform>
#include <algorithm>

namespace {
constexpr int kDragScrollZone = 40;      // height of the edge zones that scroll while dragging
constexpr int kDragScrollMaxStep = 20;   // pixels per tick right at the edge
constexpr int kDragScrollInterval = 20;  // ms between two ticks

// High resolution icon, Qt scales it down when drawing
QIcon pageIcon(const QImage &img) {
    return QIcon(QPixmap::fromImage(img.scaled(1000, 1414, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
}

// 90° clockwise in the same format: Qt turns a 1 bit image into 32 bit while rotating, the
// black/white page would lose its small size
QImage rotatedImage(const QImage &image) {
    if (image.isNull()) return image;
    QImage rotated = image.transformed(QTransform().rotate(90));
    if (rotated.format() != image.format()) {
        rotated = rotated.convertToFormat(image.format(), Qt::ThresholdDither);
    }
    rotated.setDotsPerMeterX(image.dotsPerMeterY());
    rotated.setDotsPerMeterY(image.dotsPerMeterX());
    return rotated;
}
}

PageListWidget::PageListWidget(QWidget *parent) : QListWidget(parent) {
    // One column of pages. ListMode instead of IconMode: only there drag and drop keeps the
    // pages in order (IconMode places dropped items freely)
    setViewMode(QListView::ListMode);
    setFlow(QListView::TopToBottom);
    setWrapping(false);
    setResizeMode(QListView::Adjust);
    setSpacing(10);   // Distance between pages
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // Pages are scrolled vertically only
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);  // ListMode scrolls per item otherwise
    // Minimum width (prevents collapsing to 0) is set in mainwin.ui

    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDefaultDropAction(Qt::MoveAction);
    setDropIndicatorShown(true);
    // Qt's own auto scroll reads QCursor::pos(), which does not move during a drag under
    // Wayland; the own timer uses the position of the drag events instead
    setAutoScroll(false);
    m_dragScrollTimer = new QTimer(this);
    m_dragScrollTimer->setInterval(kDragScrollInterval);
    connect(m_dragScrollTimer, &QTimer::timeout, this, &PageListWidget::dragScroll);

    // The × follows the mouse
    setMouseTracking(true);
    viewport()->setAttribute(Qt::WA_Hover);

    m_delegate = new PageDelegate(this);
    setItemDelegate(m_delegate);
}

bool PageListWidget::addPage(const QString &filePath, int dpi) {
    QImage img(filePath);
    if (img.isNull()) return false;

    QListWidgetItem *item = new QListWidgetItem(pageIcon(img), tr("PAGE %1").arg(count() + 1));
    item->setData(Qt::UserRole, filePath);
    item->setData(kDpiRole, dpi);

    addItem(item);
    updateItemSizes(); // More pages may need the narrower width
    // The new page is the current and only selected one (the preview follows)
    setCurrentItem(item, QItemSelectionModel::ClearAndSelect);
    scrollToBottom();
    emit pagesChanged();
    return true;
}

QList<PageListWidget::Page> PageListWidget::pages(bool selectedOnly) const {
    QList<Page> result;
    for (int row = 0; row < count(); ++row) {
        const QListWidgetItem *page = item(row);
        if (selectedOnly && !page->isSelected()) continue;
        result.append({page->data(Qt::UserRole).toString(), page->data(kDpiRole).toInt()});
    }
    return result;
}

// Rotating the file itself keeps list, preview and PDF the same without extra state
void PageListWidget::rotateSelectedPages() {
    const QList<QListWidgetItem *> selected = selectedItems();
    if (selected.isEmpty()) return;
    for (QListWidgetItem *page : selected) {
        const QString filePath = page->data(Qt::UserRole).toString();
        const QImage rotated = rotatedImage(QImage(filePath));
        if (rotated.isNull() || !rotated.save(filePath, "PNG")) continue;
        page->setIcon(pageIcon(rotated));
        // The gray original of a black/white page turns along (for a later new threshold)
        const QString original = PageProcessing::originalPath(filePath);
        if (QFile::exists(original)) rotatedImage(QImage(original)).save(original, "PNG");
    }
    viewport()->update();
    emit pagesChanged();
    emit pageImagesChanged();
}

void PageListWidget::removeAllPages() {
    QList<int> rows;
    for (int row = 0; row < count(); ++row) rows.append(row);
    removeRows(rows);
}

void PageListWidget::resizeEvent(QResizeEvent *event) {
    QListWidget::resizeEvent(event);
    updateItemSizes();
}

// Pages fill the list width minus the spacing and the padding of the entry. If they do not fit
// at full width, they are shrunk until they fit the height, but at most by the width of the
// vertical scroll bar. Only if that is not enough the scroll bar appears in the space freed.
// The calculation uses the viewport size without scroll bars, so it does not depend on
// whether the bar is currently shown and cannot toggle back and forth.
// ListMode layout (measured): every item has the spacing above and below it.
void PageListWidget::updateItemSizes() {
    const QSize area = maximumViewportSize();
    const int space = spacing();
    const int pages = count();
    const int frame = 2 * PageDelegate::kPadding;        // padding around image and label
    const int extra = PageDelegate::kTextHeight + frame;  // item height besides the image

    auto contentHeight = [&](int width) {
        return pages * (static_cast<int>(width * 1.41) + extra + 2 * space);
    };

    const int fullWidth = area.width() - 2 * space - frame;
    const int narrowWidth = fullWidth - verticalScrollBar()->sizeHint().width();
    int w = fullWidth;
    if (pages > 0 && contentHeight(fullWidth) > area.height()) {
        if (contentHeight(narrowWidth) <= area.height()) {
            // Largest width at which all pages still fit the height
            int fitWidth = static_cast<int>((area.height() - pages * (extra + 2 * space))
                                            / (pages * 1.41));
            w = qMax(narrowWidth, fitWidth);
        } else {
            w = narrowWidth;
        }
    }
    if (w < 40) return; // List too narrow
    int h = static_cast<int>(w * 1.41); // A4 ratio

    m_delegate->setPageSize(QSize(w, h));
    doItemsLayout(); // The delegate reports the new size
}

// ====== Remove ======

int PageListWidget::crossRowAt(const QPoint &pos) const {
    const QModelIndex index = indexAt(pos);
    if (!index.isValid()) return -1;
    return m_delegate->crossRect(index, visualRect(index)).contains(pos) ? index.row() : -1;
}

void PageListWidget::setCrossHoverRow(int row) {
    if (row == m_crossHoverRow) return;
    m_crossHoverRow = row;
    m_delegate->setCrossHoverRow(row);
    viewport()->update();
}

// Removes the pages and deletes their image files (a page cannot be brought back anyway)
void PageListWidget::removeRows(QList<int> rows) {
    if (rows.isEmpty()) return;
    std::sort(rows.begin(), rows.end(), std::greater<int>());   // from the end: rows stay valid
    for (int row : std::as_const(rows)) {
        QListWidgetItem *page = takeItem(row);
        if (!page) continue;
        const QString filePath = page->data(Qt::UserRole).toString();
        QFile::remove(filePath);
        QFile::remove(PageProcessing::originalPath(filePath));   // gray original, if any
        delete page;
    }
    setCrossHoverRow(-1);
    renumber();
    updateItemSizes();   // fewer pages may fit at full width again
    emit pagesChanged();
}

void PageListWidget::removeSelectedPages() {
    QList<int> rows;
    const QList<QListWidgetItem *> pages = selectedItems();
    for (QListWidgetItem *page : pages) rows.append(row(page));
    removeRows(rows);
}

void PageListWidget::renumber() {
    for (int i = 0; i < count(); ++i) {
        item(i)->setText(tr("PAGE %1").arg(i + 1));
    }
}

// ====== Scrolling while dragging ======

// The closer to the edge, the faster; negative: up
int PageListWidget::dragScrollStep() const {
    const int y = m_dragPos.y();
    const int height = viewport()->height();
    if (y < kDragScrollZone)
        return -(1 + (kDragScrollZone - qMax(y, 0)) * kDragScrollMaxStep / kDragScrollZone);
    if (y > height - kDragScrollZone)
        return 1 + (qMin(y, height) - (height - kDragScrollZone)) * kDragScrollMaxStep / kDragScrollZone;
    return 0;
}

// Runs on while the mouse rests in an edge zone (no drag events come then)
void PageListWidget::dragScroll() {
    const int step = dragScrollStep();
    QScrollBar *bar = verticalScrollBar();
    const int before = bar->value();
    bar->setValue(before + step);
    if (step == 0 || bar->value() == before) m_dragScrollTimer->stop();   // nothing left to scroll
}

// ====== Events ======

// A click on the × removes the page on release; it neither selects nor starts a drag
void PageListWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_pressedCrossRow = crossRowAt(event->position().toPoint());
        if (m_pressedCrossRow >= 0) {
            event->accept();
            return;
        }
    }
    QListWidget::mousePressEvent(event);
}

void PageListWidget::mouseMoveEvent(QMouseEvent *event) {
    setCrossHoverRow(crossRowAt(event->position().toPoint()));
    if (m_pressedCrossRow >= 0) {
        event->accept();
        return;
    }
    QListWidget::mouseMoveEvent(event);
}

void PageListWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (m_pressedCrossRow >= 0) {
        const int row = m_pressedCrossRow;
        m_pressedCrossRow = -1;
        if (crossRowAt(event->position().toPoint()) == row) removeRows({row});
        event->accept();
        return;
    }
    QListWidget::mouseReleaseEvent(event);
}

void PageListWidget::leaveEvent(QEvent *event) {
    setCrossHoverRow(-1);
    QListWidget::leaveEvent(event);
}

void PageListWidget::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        removeSelectedPages();
        event->accept();
        return;
    }
    QListWidget::keyPressEvent(event);
}

void PageListWidget::contextMenuEvent(QContextMenuEvent *event) {
    QListWidgetItem *page = itemAt(event->pos());
    if (!page) return;
    // Right click on a page that is not selected: it alone becomes the selection
    if (!page->isSelected()) {
        clearSelection();
        page->setSelected(true);
        setCurrentItem(page);
    }
    QMenu menu(this);
    QAction *remove = menu.addAction(tr("REMOVE_PAGES %n", nullptr, int(selectedItems().size())));
    if (menu.exec(event->globalPos()) == remove) removeSelectedPages();
}

void PageListWidget::dragMoveEvent(QDragMoveEvent *event) {
    QListWidget::dragMoveEvent(event);
    m_dragPos = event->position().toPoint();
    if (dragScrollStep() != 0) {
        if (!m_dragScrollTimer->isActive()) m_dragScrollTimer->start();
    } else {
        m_dragScrollTimer->stop();
    }
}

void PageListWidget::dragLeaveEvent(QDragLeaveEvent *event) {
    m_dragScrollTimer->stop();
    QListWidget::dragLeaveEvent(event);
}

void PageListWidget::dropEvent(QDropEvent *event) {
    m_dragScrollTimer->stop();
    QListWidget::dropEvent(event);
    renumber();   // the labels follow the new order
    emit pagesChanged();
}
