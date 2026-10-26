#pragma once

#include <QStringList>
#include <QWidget>

class QListView;
class QStandardItemModel;
class QTextBrowser;
class ToolbarM;

// Help page (taken from MovieMetaM, in the look of this app): own ToolbarM on top ("Back",
// title), the topics in a list on the left, the text on the right. Works without internet:
// the texts are Markdown files in the resources (help/<language>/<topic>.md), German or English
// like the app. The topics follow the work from scanning to saving, the info about the app last.
class HelpPage : public QWidget {
    Q_OBJECT

public:
    enum Topic { Scanning, Pages, Settings, Saving, About };   // order in the list

    explicit HelpPage(QWidget *parent = nullptr);

    void showTopic(Topic topic);
    ToolbarM *toolbar() const { return m_toolbar; }   // the main window gives it the app's shape

signals:
    void backRequested();

protected:
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;   // line width follows the text area

private:
    static constexpr int kMaxLineWidth = 760;   // lines not too long to read
    static constexpr int kTopicsWidth = 240;
    static constexpr int kImageHeight = 20;     // SVG pictures in the texts
    static constexpr int kScreenshotScale = 2;  // screenshots (PNG) are Retina shots: shown in half the pixels

    void addTopic(const QString &icon, const QString &title, const QString &subtitle, const QString &file);
    void loadText(int row);
    void spaceOut();      // headings, line spacing, paragraphs
    void scaleImages();   // SVGs to kImageHeight, screenshots in their size; sharp on the screen
    void restyle();
    static QString language();   // the language of the app ("de", "en")

    ToolbarM           *m_toolbar = nullptr;
    QListView          *m_topics = nullptr;
    QStandardItemModel *m_model = nullptr;
    QTextBrowser       *m_text = nullptr;
    QStringList         m_icons;   // icon of each topic (tinted again on a theme change)
    QStringList         m_files;   // file name of each topic
};
