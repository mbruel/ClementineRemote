//========================================================================
//
// Copyright (C) 2020 Matthieu Bruel <Matthieu.Bruel@gmail.com>
//
// This file is a part of ClementineRemote : https://github.com/mbruel/ClementineRemote
//
// ClementineRemote is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; version 3.0 of the License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301,
// USA.
//
//========================================================================

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
