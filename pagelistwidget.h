#pragma once

#include <QListWidget>

// Column of scanned pages. Each page fills the list width (A4 ratio) and the sizes follow
// every resize of the list itself (window, splitter, scroll bar).
class PageListWidget : public QListWidget
{
    Q_OBJECT

public:
    explicit PageListWidget(QWidget *parent = nullptr);

    // Adds the image as a new page; the file path is stored in Qt::UserRole
    bool addPage(const QString &filePath);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateItemSizes();

    static constexpr int m_textHeight = 30;   // Space for the page label below the image
};
