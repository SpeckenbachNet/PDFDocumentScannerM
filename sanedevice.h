#pragma once

#include <QProcessEnvironment>
#include <QString>

// How scanimage opens a scanner.
//
// The escl backend of SANE 1.4 takes only the first page of a feeder job (the scanner pulls the
// whole stack in one job, escl starts a new job for page 2 and gets "out of documents"). The
// airscan backend (sane-airscan) takes every page of the job. So escl devices are opened through
// airscan when sane-airscan is installed – with a configuration of our own (SANE_CONFIG_DIR, only
// for our scanimage calls): only airscan, the scanner entered by its URL, discovery off. Then the
// device is always "airscan:e0:Scanner" (airscan's own IDs change with every search) and opening
// needs no search. The stored device ID stays the escl one (escl:http://IP:port).
namespace SaneDevice {

struct Target {
    QString device;                   // for scanimage -d
    QProcessEnvironment environment;  // for the QProcess
};

// esclUrl: the scanner's eSCL root, e.g. http://192.168.178.31:8080/eSCL/ (only used for escl IDs)
Target forDevice(const QString &deviceId, const QString &esclUrl);

}
