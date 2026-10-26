#pragma once

#include <QStyledItemDelegate>

// Entry with icon on the left, beside it the title and below a smaller, dimmed second line
// (taken from MovieMetaM, used for the help topics). The model text is "title\nsecond line".
// Rounded selection in the accent color, some space around.
class TwoLineDelegate : public QStyledItemDelegate
{
public:
    explicit TwoLineDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    static constexpr int kMarginH = 2;   // to the border of the box
    static constexpr int kMarginV = 1;   // half of the gap between two entries
    static constexpr int kPaddingH = 6;  // inside the entry
    static constexpr int kPaddingV = 7;   // the line box of the font has more room on top than below
    static constexpr int kIconGap = 8;   // between icon and text
};
