#include "updatechecker.h"

#include <QDate>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QVersionNumber>

namespace {

const char *kLatestReleaseApi = "https://api.github.com/repos/SpeckenbachNet/PDFDocumentScannerM/releases/latest";
const char *kReleasePage = "https://github.com/SpeckenbachNet/PDFDocumentScannerM/releases/tag/v";

} // namespace

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
{
}

bool UpdateChecker::isEnabled()
{
    return QSettings().value("General/CheckUpdates", true).toBool();
}

void UpdateChecker::setEnabled(bool enabled)
{
    QSettings().setValue("General/CheckUpdates", enabled);
}

QUrl UpdateChecker::releaseUrl(const QString &version)
{
    return QUrl(QString::fromLatin1(kReleasePage) + version);
}

void UpdateChecker::check()
{
    if (!isEnabled()) {
        return;
    }
    QSettings settings;
    announceIfNewer(settings.value("General/LatestVersion").toString());
    if (settings.value("General/LastUpdateCheck").toDate() != QDate::currentDate()) {
        request();
    }
}

void UpdateChecker::request()
{
    if (!m_network) {
        m_network = new QNetworkAccessManager(this);
    }
    QNetworkRequest request{QUrl(QString::fromLatin1(kLatestReleaseApi))};
    // GitHub wants a User-Agent; JSON of the REST API
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral(PROJECT_NAME "/" PROJECT_VERSION));
    request.setRawHeader("Accept", "application/vnd.github+json");
    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            return;   // no network, no release yet (404), rate limit: try again tomorrow or next start
        }
        const QString tag = QJsonDocument::fromJson(reply->readAll()).object().value("tag_name").toString();
        const QString version = tag.startsWith(QLatin1Char('v')) ? tag.mid(1) : tag;
        if (QVersionNumber::fromString(version).isNull()) {
            return;
        }
        QSettings settings;
        settings.setValue("General/LastUpdateCheck", QDate::currentDate());
        settings.setValue("General/LatestVersion", version);
        announceIfNewer(version);
    });
}

void UpdateChecker::announceIfNewer(const QString &version)
{
    const QVersionNumber latest = QVersionNumber::fromString(version);
    if (!latest.isNull() && latest > QVersionNumber::fromString(QStringLiteral(PROJECT_VERSION))) {
        emit updateAvailable(version);
    }
}
