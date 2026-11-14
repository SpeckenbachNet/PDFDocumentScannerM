#include "sanedevice.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>

namespace SaneDevice {

namespace {

// The name in our airscan.conf; airscan calls the device airscan:e0:<name>
const char *kAirscanName = "Scanner";

bool writeFile(const QString &path, const QByteArray &content) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::Truncate) && file.write(content) == content.size();
}

// ~/.cache/SpeckenbachNet/PDFDocumentScannerM/sane with dll.conf and airscan.conf; empty: failed
QString writeAirscanConfig(const QString &esclUrl) {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/sane";
    if (!QDir().mkpath(dir)) return QString();
    const QByteArray airscan = "[devices]\n\"" + QByteArray(kAirscanName) + "\" = " + esclUrl.toUtf8()
                               + ", eSCL\n\n[options]\ndiscovery = disable\n";
    if (!writeFile(dir + "/dll.conf", "airscan\n") || !writeFile(dir + "/airscan.conf", airscan)) {
        return QString();
    }
    return dir;
}

}

Target forDevice(const QString &deviceId, const QString &esclUrl) {
    Target target{deviceId, QProcessEnvironment::systemEnvironment()};
    const bool airscanInstalled = !QStandardPaths::findExecutable("airscan-discover").isEmpty();
    if (!deviceId.startsWith("escl:") || esclUrl.isEmpty() || !airscanInstalled) return target;

    const QString configDir = writeAirscanConfig(esclUrl);
    if (configDir.isEmpty()) return target;   // escl as before
    target.device = QString("airscan:e0:%1").arg(kAirscanName);
    target.environment.insert("SANE_CONFIG_DIR", configDir);
    return target;
}

}
