#include "ResultSink.h"

#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QTextStream>
#include <cstdio>

namespace {
QMutex g_mutex;
QFile g_file;
QTextStream *g_stream = nullptr;
bool g_enabled = false;
qint64 g_startedMs = 0;
quint64 g_ports = 0;
quint64 g_findings = 0;
quint64 g_logs = 0;
}

QString ResultSink::timestamp()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}

void ResultSink::init(const QString &path)
{
    QMutexLocker locker(&g_mutex);
    if (g_enabled) {
        if (g_stream) g_stream->flush();
        delete g_stream;
        g_stream = nullptr;
        if (g_file.isOpen()) g_file.close();
        g_enabled = false;
    }

    if (path == "-") {
        g_stream = new QTextStream(stdout);
    } else {
        g_file.setFileName(path);
        if (!g_file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            std::fprintf(stderr, "nesca: cannot open JSONL output '%s'\n", path.toLocal8Bit().constData());
            return;
        }
        g_stream = new QTextStream(&g_file);
    }

    g_stream->setCodec("UTF-8");
    g_enabled = true;
    g_startedMs = QDateTime::currentMSecsSinceEpoch();
    g_ports = 0;
    g_findings = 0;
    g_logs = 0;
}

void ResultSink::shutdown()
{
    QMutexLocker locker(&g_mutex);
    if (g_stream) {
        g_stream->flush();
        delete g_stream;
        g_stream = nullptr;
    }
    if (g_file.isOpen()) g_file.close();
    g_enabled = false;
}

bool ResultSink::isEnabled()
{
    QMutexLocker locker(&g_mutex);
    return g_enabled;
}

void ResultSink::write(const QJsonObject &obj)
{
    QMutexLocker locker(&g_mutex);
    if (!g_enabled || !g_stream) return;
    const QByteArray line = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    *g_stream << QString::fromUtf8(line) << '\n';
    g_stream->flush();
}

void ResultSink::portOpen(const QString &ip, int port, const QString &banner)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("type"), QStringLiteral("port"));
    obj.insert(QStringLiteral("ts"), timestamp());
    obj.insert(QStringLiteral("ip"), normalizeHost(ip));
    obj.insert(QStringLiteral("port"), port);
    obj.insert(QStringLiteral("status"), QStringLiteral("open"));
    if (!banner.isEmpty()) obj.insert(QStringLiteral("banner"), banner.left(512));
    {
        QMutexLocker locker(&g_mutex);
        ++g_ports;
    }
    write(obj);
}

void ResultSink::finding(const QString &ip, int port, const QString &module,
                         const QString &status, const QString &login,
                         const QString &pass, const QString &banner)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("type"), QStringLiteral("finding"));
    obj.insert(QStringLiteral("ts"), timestamp());
    obj.insert(QStringLiteral("ip"), normalizeHost(ip));
    obj.insert(QStringLiteral("port"), port);
    obj.insert(QStringLiteral("module"), module);
    obj.insert(QStringLiteral("status"), status);
    if (!login.isEmpty()) obj.insert(QStringLiteral("login"), login);
    if (!pass.isEmpty()) obj.insert(QStringLiteral("password"), pass);
    if (!banner.isEmpty()) obj.insert(QStringLiteral("banner"), banner.left(512));
    {
        QMutexLocker locker(&g_mutex);
        ++g_findings;
    }
    write(obj);
}

void ResultSink::log(const QString &level, const QString &line)
{
    if (line.trimmed().isEmpty()) return;
    QJsonObject obj;
    obj.insert(QStringLiteral("type"), QStringLiteral("log"));
    obj.insert(QStringLiteral("ts"), timestamp());
    obj.insert(QStringLiteral("level"), level);
    obj.insert(QStringLiteral("line"), line.left(2048));
    {
        QMutexLocker locker(&g_mutex);
        ++g_logs;
    }
    write(obj);
}

void ResultSink::summary(int exitCode, bool timedOut)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("type"), QStringLiteral("summary"));
    obj.insert(QStringLiteral("ts"), timestamp());
    obj.insert(QStringLiteral("duration_ms"),
               static_cast<double>(QDateTime::currentMSecsSinceEpoch() - g_startedMs));
    {
        QMutexLocker locker(&g_mutex);
        obj.insert(QStringLiteral("ports"), static_cast<double>(g_ports));
        obj.insert(QStringLiteral("findings"), static_cast<double>(g_findings));
        obj.insert(QStringLiteral("logs"), static_cast<double>(g_logs));
    }
    obj.insert(QStringLiteral("exit_code"), exitCode);
    obj.insert(QStringLiteral("timed_out"), timedOut);
    write(obj);
}

QString ResultSink::normalizeHost(const QString &ip)
{
    QString host = ip;
    const int scheme = host.indexOf(QStringLiteral("://"));
    if (scheme >= 0) host = host.mid(scheme + 3);
    const int slash = host.indexOf(QLatin1Char('/'));
    if (slash >= 0) host = host.left(slash);
    if (host.count(QLatin1Char(':')) == 1) host = host.section(QLatin1Char(':'), 0, 0);
    return host;
}

QString ResultSink::stripHtml(const QString &html)
{
    QString out = html;
    out.remove(QRegularExpression(QStringLiteral("<[^>]*>")));
    out.replace(QStringLiteral("&nbsp;"), QStringLiteral(" "));
    out.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
    out.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
    out.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
    out.replace(QStringLiteral("&quot;"), QStringLiteral("\""));
    out.replace(QStringLiteral("&#39;"), QStringLiteral("'"));
    out.replace(QRegularExpression(QStringLiteral("[ \\t]+")), QStringLiteral(" "));
    return out.trimmed();
}
