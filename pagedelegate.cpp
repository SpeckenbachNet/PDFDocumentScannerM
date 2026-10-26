#include "pagedelegate.h"
#include "uicolors.h"

#include <QPainter>

PageDelegate::PageDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

// The image as drawn: the room of the page, the picture inside keeps its own ratio
// (a landscape scan is smaller), centered at the top of the item
QRect PageDelegate::imageRect(const QModelIndex &index, const QRect &itemRect) const
{
    const QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    QSize size = icon.isNull() ? m_pageSize : icon.actualSize(m_pageSize);
    if (size.isEmpty()) size = m_pageSize;
    const QRect room(itemRect.left() + (itemRect.width() - m_pageSize.width()) / 2,
                     itemRect.top() + kPadding, m_pageSize.width(), m_pageSize.height());
    return QRect(room.left() + (room.width() - size.width()) / 2,
                 room.top() + (room.height() - size.height()) / 2, size.width(), size.height());
}

QRect PageDelegate::crossRect(const QModelIndex &index, const QRect &itemRect) const
{
    const QRect image = imageRect(index, itemRect);
    return QRect(image.right() + 1 - kCrossMargin - kCrossSize, image.top() + kCrossMargin,
                 kCrossSize, kCrossSize);
}

void PageDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    const bool selected = opt.state & QStyle::State_Selected;
    const bool hover = opt.state & QStyle::State_MouseOver;

    painter->save();
    painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    // Entry: image and label with padding, centered in the row
    const QRect entry(opt.rect.left() + (opt.rect.width() - m_pageSize.width()) / 2 - kPadding,
                      opt.rect.top(), m_pageSize.width() + 2 * kPadding,
                      m_pageSize.height() + kTextHeight + 2 * kPadding);

    QColor textColor = opt.palette.color(QPalette::Text);
    if (selected) {
        // Background from the group of the view (inactive when the list lost the focus), the
        // text always from the active group; too little contrast: black or white
        const QPalette::ColorGroup group = (opt.state & QStyle::State_Active) ? QPalette::Active
                                                                                : QPalette::Inactive;
        const QColor background = opt.palette.color(group, QPalette::Highlight);
        textColor = opt.palette.color(QPalette::Active, QPalette::HighlightedText);
        if (UiColors::contrastRatio(background, textColor) < 3.0) {
            textColor = UiColors::contrastRatio(background, Qt::black) >= UiColors::contrastRatio(background, Qt::white)
                            ? QColor(Qt::black) : QColor(Qt::white);
        }
        painter->setPen(Qt::NoPen);
        painter->setBrush(background);
        painter->drawRoundedRect(entry, UiColors::entryRadius, UiColors::entryRadius);
    }

    // The scanned page
    const QRect image = imageRect(index, opt.rect);
    opt.icon.paint(painter, image, Qt::AlignCenter);

    // Label below the image
    const QRect label(entry.left(), entry.top() + kPadding + m_pageSize.height(),
                      entry.width(), kTextHeight);
    painter->setPen(textColor);
    painter->drawText(label, Qt::AlignCenter, opt.text);

    // Round × over the image (any colors underneath): dark disc, white ×; accent color when the
    // mouse is over it
    if (hover) {
        const bool overCross = index.row() == m_crossHoverRow;
        const QRectF disc = crossRect(index, opt.rect);
        painter->setPen(Qt::NoPen);
        painter->setBrush(overCross ? opt.palette.color(QPalette::Highlight) : QColor(0, 0, 0, 150));
        painter->drawEllipse(disc);
        painter->setPen(QPen(overCross ? opt.palette.color(QPalette::HighlightedText) : QColor(Qt::white),
                             1.5, Qt::SolidLine, Qt::RoundCap));
        const QRectF cross = QRectF(0, 0, 7, 7).translated(disc.center() - QPointF(3.5, 3.5));
        painter->drawLine(cross.topLeft(), cross.bottomRight());
        painter->drawLine(cross.topRight(), cross.bottomLeft());
    }

    painter->restore();
}

QSize PageDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
    return QSize(m_pageSize.width() + 2 * kPadding, m_pageSize.height() + kTextHeight + 2 * kPadding);
}
