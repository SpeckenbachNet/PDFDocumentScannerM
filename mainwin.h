#ifndef MAINWIN_H
#define MAINWIN_H

#include <QMainWindow>
#include <QApplication>
#include <QMetaEnum>
#include <QLocale>
#include <QtMath>
#include <QPageSize>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QSettings>
#include <QToolButton>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QMovie>
#include "settingsoverlay.h"
#include "statusoverlay.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWin;
}
QT_END_NAMESPACE

class MainWin : public QMainWindow
{
    Q_OBJECT

protected:
    void resizeEvent(QResizeEvent *event);
    void changeEvent(QEvent *event) override;

public:
    MainWin(QWidget *parent = nullptr);
    ~MainWin();
    QSettings settings;
    void readSettingsFromINI();

private:
    Ui::MainWin *ui;
    void initSettings();
    void initializeForm();
    void initializeConnections();
    void initializeSettings();
    void updateListIconSize();
    void applySurfaceColors();
    void refreshSettings();
    void searchForScanners();
    void refreshScanners();
    void showStatusLabel(QString overlayText, bool isError = false, int durationMs = 0);
    void closeStatusLabel();
    void startScan();
    void getScannerCapabilities(const QString &deviceID);
    void saveScanners(const QString &saneOutput);
    void updatePreview(const QString &imagePath);
    void addPageToList(const QString &filePath);

    QString getPageFormat();
    QStringList m_processedFiles;
    SettingsOverlay *m_settingsOverlay = nullptr;
    StatusOverlay *m_statusOverlay = nullptr;
    QLabel *m_loadingLabel = nullptr;
    QToolButton *m_scanBtn = nullptr;
    QToolButton *m_settingsBtn = nullptr;
    QToolButton *m_refreshBtn = nullptr;
    QToolButton *m_rotateBtn = nullptr;
    QToolButton *m_clearBtn = nullptr;
    QToolButton *m_fakeDplxBtn = nullptr;
    QToolButton *m_saveSelBtn = nullptr;
    QToolButton *m_saveAllBtn = nullptr;

    QString m_cachedScannerOutput;
    QString m_scanErrorOutput;           // Last stderr message of scanimage

    QGraphicsScene* m_previewScene = nullptr;      // Die "Leinwand"
    QGraphicsPixmapItem* m_previewItem = nullptr;  // Das eigentliche Bild-Objekt auf der Leinwand

private slots:
    void showSettingsOverlay();
    void onFilelistTableViewClicked(const QModelIndex &index);
};
#endif // MAINWIN_H
