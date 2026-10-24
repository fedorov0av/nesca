#include "nesca.h"
#include "STh.h"
#include "ResultSink.h"
#include "HeadlessOptions.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QTimer>
#include <qfontdatabase.h>
#include <QMessageBox>
#include <qpixmap.h>

#include "Utils.h"
#include "externData.h"

#include <iostream>
#include <cstring>

namespace {

const char *kDefaultPorts = "80,81,88,8080,8081,60001,60002,8008,8888,554,9000,3536,21";

void loadFonts()
{
    QStringList list;
    list << "small_font.ttf";
    int fontID(-1);
    bool fontWarningShown(false);
    for (QStringList::const_iterator constIterator = list.constBegin(); constIterator != list.constEnd(); ++constIterator) {
        QFile res(":/nesca/" + *constIterator);
        if (res.open(QIODevice::ReadOnly) == false) {
            if (fontWarningShown == false) {
                fontWarningShown = true;
            }
        }
        else {
            fontID = QFontDatabase::addApplicationFontFromData(res.readAll());
            if (fontID == -1 && fontWarningShown == false) {
                fontWarningShown = true;
            }
        }
    }
}

bool copyDir(const QString &src, const QString &dst)
{
    QDir srcDir(src);
    if (!srcDir.exists()) {
        std::cerr << "nesca: missing wordlist directory: " << src.toLocal8Bit().constData() << std::endl;
        return false;
    }
    if (!QDir().mkpath(dst)) {
        std::cerr << "nesca: cannot create " << dst.toLocal8Bit().constData() << std::endl;
        return false;
    }

    const QFileInfoList files = srcDir.entryInfoList(QDir::Files);
    for (const QFileInfo &fi : files) {
        const QString target = dst + "/" + fi.fileName();
        QFile::remove(target);
        if (!QFile::copy(fi.absoluteFilePath(), target)) {
            std::cerr << "nesca: cannot copy " << fi.absoluteFilePath().toLocal8Bit().constData() << std::endl;
            return false;
        }
    }
    return true;
}

void truncateCredentialLists(const QString &dir)
{
    const char *names[] = {
        "login.txt", "pass.txt", "ftplogin.txt", "ftppass.txt",
        "wflogin.txt", "wfpass.txt", "sshpass.txt"
    };
    for (const char *name : names) {
        QFile f(dir + "/" + QString::fromLatin1(name));
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) f.close();
    }
}

bool copyList(const QString &src, const QString &dst, const QString &label)
{
    if (src.isEmpty()) return true;
    QFileInfo fi(src);
    if (!fi.isFile() || fi.size() == 0) {
        std::cerr << "nesca: " << label.toLocal8Bit().constData()
                  << " is missing or empty: " << src.toLocal8Bit().constData() << std::endl;
        return false;
    }
    QFile::remove(dst);
    if (!QFile::copy(src, dst)) {
        std::cerr << "nesca: cannot copy " << label.toLocal8Bit().constData() << std::endl;
        return false;
    }
    return true;
}

QString absolutePath(const QString &path)
{
    return QFileInfo(path).absoluteFilePath();
}

int runHeadless(QApplication &app, HeadlessOptions &opts)
{
    const QString originalCwd = QDir::currentPath();
    const QString appDir = QCoreApplication::applicationDirPath();

    if (opts.targetFile.isEmpty() == false) opts.targetFile = absolutePath(opts.targetFile);
    if (opts.userlist.isEmpty() == false) opts.userlist = absolutePath(opts.userlist);
    if (opts.passlist.isEmpty() == false) opts.passlist = absolutePath(opts.passlist);
    if (opts.ftpUserlist.isEmpty() == false) opts.ftpUserlist = absolutePath(opts.ftpUserlist);
    if (opts.ftpPasslist.isEmpty() == false) opts.ftpPasslist = absolutePath(opts.ftpPasslist);
    if (opts.wfUserlist.isEmpty() == false) opts.wfUserlist = absolutePath(opts.wfUserlist);
    if (opts.wfPasslist.isEmpty() == false) opts.wfPasslist = absolutePath(opts.wfPasslist);
    if (opts.sshPasslist.isEmpty() == false) opts.sshPasslist = absolutePath(opts.sshPasslist);

    if (opts.mode == "import") {
        if (opts.targetFile.isEmpty() || !QFileInfo::exists(opts.targetFile)) {
            std::cerr << "nesca: --target-file is required for import mode and must exist" << std::endl;
            return 2;
        }
    } else if (opts.target.trimmed().isEmpty()) {
        std::cerr << "nesca: --target is required for " << opts.mode.toLocal8Bit().constData() << " mode" << std::endl;
        return 2;
    }

    if (opts.ports.trimmed().isEmpty()) {
        std::cerr << "nesca: --ports must not be empty" << std::endl;
        return 2;
    }

    const bool anyCustomList = !opts.userlist.isEmpty() || !opts.passlist.isEmpty()
        || !opts.ftpUserlist.isEmpty() || !opts.ftpPasslist.isEmpty()
        || !opts.wfUserlist.isEmpty() || !opts.wfPasslist.isEmpty()
        || !opts.sshPasslist.isEmpty();

    if (opts.brute && !opts.useDefaultWordlists && !anyCustomList) {
        std::cerr << "nesca: --brute requires wordlists. Pass --userlist/--passlist "
                     "(or protocol-specific lists) or --use-default-wordlists." << std::endl;
        return 2;
    }

    QString runDir = opts.workDir;
    if (runDir.isEmpty()) {
        runDir = originalCwd + "/nesca-run-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
    } else if (QDir::isRelativePath(runDir)) {
        runDir = originalCwd + "/" + runDir;
    }
    if (!QDir().mkpath(runDir)) {
        std::cerr << "nesca: cannot create workdir: " << runDir.toLocal8Bit().constData() << std::endl;
        return 2;
    }

    const QString pwdDir = runDir + "/pwd_lists";
    if (!copyDir(appDir + "/pwd_lists", pwdDir)) return 2;

    if (opts.brute) {
        if (!opts.useDefaultWordlists) truncateCredentialLists(pwdDir);
        if (!copyList(opts.userlist, pwdDir + "/login.txt", "--userlist")) return 2;
        if (!copyList(opts.passlist, pwdDir + "/pass.txt", "--passlist")) return 2;
        if (!copyList(opts.ftpUserlist, pwdDir + "/ftplogin.txt", "--ftp-userlist")) return 2;
        if (!copyList(opts.ftpPasslist, pwdDir + "/ftppass.txt", "--ftp-passlist")) return 2;
        if (!copyList(opts.wfUserlist, pwdDir + "/wflogin.txt", "--wf-userlist")) return 2;
        if (!copyList(opts.wfPasslist, pwdDir + "/wfpass.txt", "--wf-passlist")) return 2;
        if (!copyList(opts.sshPasslist, pwdDir + "/sshpass.txt", "--ssh-passlist")) return 2;
    } else {
        truncateCredentialLists(pwdDir);
    }

    QString jsonlPath = opts.jsonlPath;
    if (jsonlPath != "-" && QDir::isRelativePath(jsonlPath)) {
        jsonlPath = originalCwd + "/" + jsonlPath;
    }
    if (jsonlPath != "-") {
        QDir().mkpath(QFileInfo(jsonlPath).absolutePath());
    }
    ResultSink::init(jsonlPath);
    if (!ResultSink::isEnabled()) return 2;

    QJsonObject start;
    start.insert(QStringLiteral("type"), QStringLiteral("start"));
    start.insert(QStringLiteral("ts"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    start.insert(QStringLiteral("mode"), opts.mode);
    start.insert(QStringLiteral("target"), opts.target);
    if (!opts.targetFile.isEmpty()) start.insert(QStringLiteral("target_file"), opts.targetFile);
    start.insert(QStringLiteral("ports"), opts.ports);
    start.insert(QStringLiteral("threads"), opts.threads);
    start.insert(QStringLiteral("brute"), opts.brute);
    start.insert(QStringLiteral("workdir"), runDir);
    ResultSink::write(start);

    if (!QDir::setCurrent(runDir)) {
        std::cerr << "nesca: cannot chdir to " << runDir.toLocal8Bit().constData() << std::endl;
        ResultSink::summary(2, false);
        ResultSink::shutdown();
        return 2;
    }

    nesca *gui = new nesca(nullptr);
    (void)gui;

    const bool quiet = opts.quiet;
    QObject::connect(stt, &STh::changeFoundData, stt, [quiet](const QString &s) {
        if (!quiet) ResultSink::log(QStringLiteral("info"), ResultSink::stripHtml(s));
    }, Qt::DirectConnection);
    QObject::connect(stt, &STh::changeGreenFoundData, stt, [quiet](const QString &s) {
        if (!quiet) ResultSink::log(QStringLiteral("info"), ResultSink::stripHtml(s));
    }, Qt::DirectConnection);
    QObject::connect(stt, &STh::changeYellowFoundData, stt, [quiet](const QString &s) {
        if (!quiet) ResultSink::log(QStringLiteral("warn"), ResultSink::stripHtml(s));
    }, Qt::DirectConnection);
    QObject::connect(stt, &STh::changeRedFoundData, stt, [quiet](const QString &s) {
        if (!quiet) ResultSink::log(QStringLiteral("error"), ResultSink::stripHtml(s));
    }, Qt::DirectConnection);

    bool timedOut = false;
    QObject::connect(stt, &STh::killSttThread, &app, [&app]() {
        QTimer::singleShot(500, &app, &QCoreApplication::quit);
    }, Qt::QueuedConnection);

    if (opts.maxRuntimeSec > 0) {
        QTimer::singleShot(opts.maxRuntimeSec * 1000, &app, [&app, &timedOut]() {
            timedOut = true;
            ResultSink::log(QStringLiteral("error"), QStringLiteral("maximum runtime exceeded"));
            app.quit();
        });
    }

    gui->startHeadlessScan(opts);
    if (!stt->isRunning()) {
        ResultSink::log(QStringLiteral("error"), QStringLiteral("scan did not start"));
        QTimer::singleShot(100, &app, &QCoreApplication::quit);
    }

    const int rc = app.exec();

    if (timedOut) {
        globalScanFlag = false;
        Threader::cleanUp();
    }
    if (stt->isRunning()) stt->wait(10000);

    ResultSink::summary(timedOut ? 1 : rc, timedOut);
    ResultSink::shutdown();
    return timedOut ? 1 : rc;
}

} // namespace

int main(int argc, char *argv[])
{
    bool headlessRequested = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--headless") == 0) headlessRequested = true;
    }

    if (headlessRequested && qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }

    QApplication a(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("nesca"));
    QCoreApplication::setApplicationVersion(QStringLiteral("3.2.0-headless"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Nesca network scanner (headless CLI)"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption headlessOpt(QStringLiteral("headless"), QStringLiteral("Run without GUI"));
    QCommandLineOption modeOpt(QStringLiteral("mode"), QStringLiteral("Scan mode: ip, dns or import"), QStringLiteral("mode"), QStringLiteral("ip"));
    QCommandLineOption targetOpt(QStringLiteral("target"), QStringLiteral("Target IP, range or CIDR"), QStringLiteral("target"));
    QCommandLineOption targetFileOpt(QStringLiteral("target-file"), QStringLiteral("File with targets (import mode)"), QStringLiteral("file"));
    QCommandLineOption portsOpt(QStringLiteral("ports"), QStringLiteral("Comma-separated ports"), QStringLiteral("ports"), QString::fromLatin1(kDefaultPorts));
    QCommandLineOption threadsOpt(QStringLiteral("threads"), QStringLiteral("Scanner threads"), QStringLiteral("n"), QStringLiteral("100"));
    QCommandLineOption timeoutOpt(QStringLiteral("timeout"), QStringLiteral("Connection timeout in seconds"), QStringLiteral("sec"), QStringLiteral("3"));
    QCommandLineOption delayOpt(QStringLiteral("delay"), QStringLiteral("Delay between thread launches in ms"), QStringLiteral("ms"), QStringLiteral("10"));
    QCommandLineOption bruteOpt(QStringLiteral("brute"), QStringLiteral("Enable credential checks"));
    QCommandLineOption bruteThreadsOpt(QStringLiteral("brute-threads"), QStringLiteral("Concurrent credential-check threads"), QStringLiteral("n"), QStringLiteral("10"));
    QCommandLineOption userlistOpt(QStringLiteral("userlist"), QStringLiteral("HTTP/Hikvision/RTSP login list"), QStringLiteral("file"));
    QCommandLineOption passlistOpt(QStringLiteral("passlist"), QStringLiteral("HTTP/Hikvision/RTSP password list"), QStringLiteral("file"));
    QCommandLineOption ftpUserlistOpt(QStringLiteral("ftp-userlist"), QStringLiteral("FTP login list"), QStringLiteral("file"));
    QCommandLineOption ftpPasslistOpt(QStringLiteral("ftp-passlist"), QStringLiteral("FTP password list"), QStringLiteral("file"));
    QCommandLineOption wfUserlistOpt(QStringLiteral("wf-userlist"), QStringLiteral("Web-form login list"), QStringLiteral("file"));
    QCommandLineOption wfPasslistOpt(QStringLiteral("wf-passlist"), QStringLiteral("Web-form password list"), QStringLiteral("file"));
    QCommandLineOption sshPasslistOpt(QStringLiteral("ssh-passlist"), QStringLiteral("SSH user:pass list"), QStringLiteral("file"));
    QCommandLineOption useDefaultOpt(QStringLiteral("use-default-wordlists"), QStringLiteral("Use bundled wordlists for --brute"));
    QCommandLineOption jsonlOpt(QStringLiteral("jsonl"), QStringLiteral("JSONL output path, '-' for stdout"), QStringLiteral("path"), QStringLiteral("-"));
    QCommandLineOption workdirOpt(QStringLiteral("workdir"), QStringLiteral("Working directory for results and wordlists"), QStringLiteral("dir"));
    QCommandLineOption maxRuntimeOpt(QStringLiteral("max-runtime"), QStringLiteral("Maximum scan runtime in seconds, 0 = unlimited"), QStringLiteral("sec"), QStringLiteral("3600"));
    QCommandLineOption quietOpt(QStringLiteral("quiet"), QStringLiteral("Do not emit log records"));
    QCommandLineOption noShuffleOpt(QStringLiteral("no-shuffle"), QStringLiteral("Disable IP range shuffling"));

    parser.addOption(headlessOpt);
    parser.addOption(modeOpt);
    parser.addOption(targetOpt);
    parser.addOption(targetFileOpt);
    parser.addOption(portsOpt);
    parser.addOption(threadsOpt);
    parser.addOption(timeoutOpt);
    parser.addOption(delayOpt);
    parser.addOption(bruteOpt);
    parser.addOption(bruteThreadsOpt);
    parser.addOption(userlistOpt);
    parser.addOption(passlistOpt);
    parser.addOption(ftpUserlistOpt);
    parser.addOption(ftpPasslistOpt);
    parser.addOption(wfUserlistOpt);
    parser.addOption(wfPasslistOpt);
    parser.addOption(sshPasslistOpt);
    parser.addOption(useDefaultOpt);
    parser.addOption(jsonlOpt);
    parser.addOption(workdirOpt);
    parser.addOption(maxRuntimeOpt);
    parser.addOption(quietOpt);
    parser.addOption(noShuffleOpt);

    parser.process(a);

    loadFonts();

    if (!parser.isSet(headlessOpt)) {
        nesca *gui = new nesca(nullptr);
        gui->showNormal();
        return a.exec();
    }

    HeadlessOptions opts;
    opts.mode = parser.value(modeOpt).toLower();
    if (opts.mode != "ip" && opts.mode != "dns" && opts.mode != "import") {
        std::cerr << "nesca: --mode must be ip, dns or import" << std::endl;
        return 2;
    }
    opts.target = parser.value(targetOpt);
    opts.targetFile = parser.value(targetFileOpt);
    opts.ports = parser.value(portsOpt);
    opts.threads = parser.value(threadsOpt).toInt();
    opts.timeout = parser.value(timeoutOpt).toInt();
    opts.delay = parser.value(delayOpt).toInt();
    opts.brute = parser.isSet(bruteOpt);
    opts.bruteThreads = parser.value(bruteThreadsOpt).toInt();
    opts.userlist = parser.value(userlistOpt);
    opts.passlist = parser.value(passlistOpt);
    opts.ftpUserlist = parser.value(ftpUserlistOpt);
    opts.ftpPasslist = parser.value(ftpPasslistOpt);
    opts.wfUserlist = parser.value(wfUserlistOpt);
    opts.wfPasslist = parser.value(wfPasslistOpt);
    opts.sshPasslist = parser.value(sshPasslistOpt);
    opts.useDefaultWordlists = parser.isSet(useDefaultOpt);
    opts.jsonlPath = parser.value(jsonlOpt);
    opts.workDir = parser.value(workdirOpt);
    opts.maxRuntimeSec = parser.value(maxRuntimeOpt).toInt();
    opts.quiet = parser.isSet(quietOpt);
    opts.shuffle = !parser.isSet(noShuffleOpt);

    if (opts.threads <= 0 || opts.timeout <= 0 || opts.delay < 0 || opts.bruteThreads <= 0 || opts.maxRuntimeSec < 0) {
        std::cerr << "nesca: threads/timeout/brute-threads must be > 0, delay >= 0, max-runtime >= 0" << std::endl;
        return 2;
    }

    return runHeadless(a, opts);
}
