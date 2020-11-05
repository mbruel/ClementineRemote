#ifndef STREAM_H
#define STREAM_H

#include <QString>

typedef struct Stream
{
    QString name;
    QString url;
    QString logoUrl;

public:
    Stream(const std::string &name_, const std::string &url_, const std::string &logoUrl_):
        name(name_.c_str()), url(url_.c_str()), logoUrl(logoUrl_.c_str()){}

    Stream():
        name(), url(), logoUrl(){}

    Stream(const Stream &) = default;
    Stream(Stream &&) = default;

    Stream& operator=(const Stream &) = default;
    Stream& operator=(Stream &&) = default;

    ~Stream() = default;

    inline QString  str() const;
} Stream;

QString Stream::str() const
{
    return QString("%1 %2 (logo: %3)").arg(name).arg(url).arg(
                logoUrl.isEmpty() ? "none" : logoUrl);
}

#endif // STREAM_H
