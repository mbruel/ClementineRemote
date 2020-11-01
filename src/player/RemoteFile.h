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

#ifndef REMOTEFILE_H
#define REMOTEFILE_H

#include <QString>

typedef struct RemoteFile
{
    QString filename;
    bool isDir;
    bool closed;


public:
    RemoteFile() = default;
    RemoteFile(const RemoteFile &) = default;
    RemoteFile(RemoteFile &&) = default;

    RemoteFile(const std::string &name_, bool isDir_);

    RemoteFile& operator=(const RemoteFile &) = default;
    RemoteFile& operator=(RemoteFile &&) = default;

    ~RemoteFile() = default;

    inline QString  str() const;
} RemoteFile;

QString RemoteFile::str() const
{
    return QString("%1 %2").arg(isDir?"Dir.:":"File:").arg(filename);
}


#endif // REMOTEFILE_H
