#include "pagelistwidget.h"
#include <QScrollBar>

PageListWidget::PageListWidget(QWidget *parent) : QListWidget(parent) {
    setViewMode(QListView::IconMode);  // Activate gallery mode
    setSpacing(10);   // Distance between pages
    setResizeMode(QListView::Adjust);  // Align the text nicely in the center
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // Pages are scrolled vertically only
    // Minimum width (prevents collapsing to 0) is set in mainwin.ui
}

bool PageListWidget::addPage(const QString &filePath) {
    QImage img(filePath);
    if (img.isNull()) return false;

    // High resolution icon, Qt scales it down when drawing
    QIcon icon(QPixmap::fromImage(img.scaled(1000, 1414, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    QListWidgetItem *item = new QListWidgetItem(icon, tr("PAGE %1").arg(count() + 1));
    item->setData(Qt::UserRole, filePath);

    addItem(item);
    updateItemSizes(); // New item needs the current size hint
    scrollToBottom();
    return true;
}

void PageListWidget::resizeEvent(QResizeEvent *event) {
    QListWidget::resizeEvent(event);
    updateItemSizes();
}

// Items fill the list width minus the spacing on both sides. If the pages do not fit at
// full width, they are shrunk until they fit the height, but at most by the width of the
// vertical scroll bar. Only if that is not enough the scroll bar appears in the space freed.
// The calculation uses the viewport size without scroll bars, so it does not depend on
// whether the bar is currently shown and cannot toggle back and forth.
void PageListWidget::updateItemSizes() {
    const QSize area = maximumViewportSize();
    const int space = spacing();
    const int pages = count();

    auto contentHeight = [&](int width) {
        return space + pages * (static_cast<int>(width * 1.41) + m_textHeight + space);
    };

    const int fullWidth = area.width() - 2 * space;
    const int narrowWidth = fullWidth - verticalScrollBar()->sizeHint().width();
    int w = fullWidth;
    if (pages > 0 && contentHeight(fullWidth) > area.height()) {
        if (contentHeight(narrowWidth) <= area.height()) {
            // Largest width at which all pages still fit the height
            int fitWidth = static_cast<int>((area.height() - space - pages * (m_textHeight + space))
                                            / (pages * 1.41));
            w = qMax(narrowWidth, fitWidth);
        } else {
            w = narrowWidth;
        }
    }
    if (w < 40) return; // List too narrow
    int h = static_cast<int>(w * 1.41); // A4 ratio

    setIconSize(QSize(w, h));
    // The size hint of every item has to follow, otherwise the layout keeps the old size
    for (int i = 0; i < pages; ++i) {
        item(i)->setSizeHint(QSize(w, h + m_textHeight));
    }
    doItemsLayout(); // Force the layout engine to recalculate
}
