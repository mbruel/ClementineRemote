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

#ifndef REMOTESONG_H
#define REMOTESONG_H
#include "protobuf/remotecontrolmessages.pb.h"

#include <QtGlobal>
#include <QString>
#include <QImage>
typedef struct RemoteSong
{
    qint32 id; // unique id of the song
    qint32 index; // Index of the current row of the active playlist
    QString title;
    QString album;
    QString artist ;
    QString albumartist;
    qint32 track;
    qint32 disc;
    QString pretty_year;
    QString genre;
    qint32 playcount;
    QString pretty_length;
    qint32 length;
    bool is_local;
    QString filename;
    qint32 file_size;
    float rating; // 0 (0 stars) to 1 (5 stars)
    QString url;
    QString art_automatic;
    QString art_manual;
    pb::remote::SongMetadata_Type type;
    QImage art;

public:
    RemoteSong() = default;
    RemoteSong(const RemoteSong &) = default;
    RemoteSong(RemoteSong &&) = default;

    RemoteSong(const pb::remote::SongMetadata &m);

    RemoteSong& operator=(const RemoteSong &) = default;
    RemoteSong& operator=(RemoteSong &&) = default;

    ~RemoteSong() = default;

    inline QString  str() const;

//    RemoteSong& operator=(const pb::remote::SongMetadata &m);
} RemoteSong;


QString RemoteSong::str() const
{
    return QString("#%1 %2 (title: %3, length: %4 (%5), size: %6)").arg(
                id).arg(filename).arg(title).arg(pretty_length).arg(length).arg(file_size);
}

#endif // REMOTESONG_H
