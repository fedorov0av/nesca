#ifndef HEADLESSOPTIONS_H
#define HEADLESSOPTIONS_H

#include <QString>

struct HeadlessOptions
{
    QString mode = QStringLiteral("ip");
    QString target;
    QString targetFile;
    QString ports;

    int threads = 100;
    int timeout = 3;
    int delay = 10;
    int bruteThreads = 10;
    bool shuffle = true;
    bool brute = false;

    bool useDefaultWordlists = false;
    QString userlist;
    QString passlist;
    QString ftpUserlist;
    QString ftpPasslist;
    QString wfUserlist;
    QString wfPasslist;
    QString sshPasslist;

    QString jsonlPath = QStringLiteral("-");
    QString workDir;
    int maxRuntimeSec = 3600;
    bool quiet = false;
};

#endif // HEADLESSOPTIONS_H
