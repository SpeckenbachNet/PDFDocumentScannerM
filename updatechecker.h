#pragma once

#include <QObject>
#include <QUrl>

class QNetworkAccessManager;

// Is there a newer version? Asks GitHub for the latest release (at most once a day, setting
// "General/LastUpdateCheck") and compares its tag ("v0.8.0") with PROJECT_VERSION. A newer
// version found is remembered ("General/LatestVersion"), so it is shown again on the next start
// of the day without a new request. No answer, no network: nothing happens.
// Setting "General/CheckUpdates" (on by default): off = no request, nothing shown.
// Taken from MovieMetaM; here the button opens the release page (the AppImage carries the
// version in its name, so there is no fixed download link).
class UpdateChecker : public QObject
{
    Q_OBJECT

public:
    explicit UpdateChecker(QObject *parent = nullptr);

    void check();                                  // at the start: the remembered version, then GitHub if due
    static bool isEnabled();                       // setting "General/CheckUpdates"
    static void setEnabled(bool enabled);
    static QUrl releaseUrl(const QString &version);   // GitHub page of that release

signals:
    void updateAvailable(const QString &version);   // e.g. "0.8.0"

private:
    void request();
    void announceIfNewer(const QString &version);

    QNetworkAccessManager *m_network = nullptr;
};
