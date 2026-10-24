#ifndef RESULTSINK_H
#define RESULTSINK_H

#include <QJsonObject>
#include <QString>

class ResultSink
{
public:
    static void init(const QString &path);
    static void shutdown();
    static bool isEnabled();

    static void write(const QJsonObject &obj);
    static void portOpen(const QString &ip, int port, const QString &banner = QString());
    static void finding(const QString &ip, int port, const QString &module,
                        const QString &status, const QString &login = QString(),
                        const QString &pass = QString(), const QString &banner = QString());
    static void log(const QString &level, const QString &line);
    static void summary(int exitCode, bool timedOut);

    static QString stripHtml(const QString &html);
    static QString normalizeHost(const QString &ip);

private:
    static QString timestamp();
};

#endif // RESULTSINK_H
