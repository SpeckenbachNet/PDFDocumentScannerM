#pragma once

#include <QStyledItemDelegate>

// One page of the page list: the scanned image, below it the label ("Page 1"). Selected pages
// get a rounded background in the accent color. Over the page under the mouse a round × in the
// top right corner of the image removes it (like the cover in MovieMetaM).
class PageDelegate : public QStyledItemDelegate
{
public:
    explicit PageDelegate(QObject *parent = nullptr);

    // Room for the image, set by the list from its width (A4 ratio)
    void setPageSize(const QSize &size) { m_pageSize = size; }
    QSize pageSize() const { return m_pageSize; }

    // Row whose × is under the mouse (-1: none): drawn in the accent color
    void setCrossHoverRow(int row) { m_crossHoverRow = row; }

    QRect crossRect(const QModelIndex &index, const QRect &itemRect) const;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    static constexpr int kPadding = 6;      // around image and label (the selection)
    static constexpr int kTextHeight = 24;  // label below the image

private:
    QRect imageRect(const QModelIndex &index, const QRect &itemRect) const;

    static constexpr int kCrossSize = 22;
    static constexpr int kCrossMargin = 8;  // to the corner of the image

    QSize m_pageSize = QSize(100, 141);
    int m_crossHoverRow = -1;
};
