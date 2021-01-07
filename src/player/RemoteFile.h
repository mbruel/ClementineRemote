//========================================================================
//
// Copyright (C) 2020 Matthieu Bruel <Matthieu.Bruel@gmail.com>
// This file is a part of ClementineRemote : https://github.com/mbruel/ClementineRemote
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 3..
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>
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
    bool selected; // for its selection in ListView


public:
    RemoteFile(const std::string &name_, bool isDir_):
        filename(name_.c_str()), isDir(isDir_), selected(false){}

    RemoteFile():
        filename(), isDir(false), selected(false){}

    RemoteFile(const RemoteFile &) = default;
    RemoteFile(RemoteFile &&) = default;

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
