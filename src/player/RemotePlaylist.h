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

#ifndef REMOTEPLAYLIST_H
#define REMOTEPLAYLIST_H
#include "protobuf/remotecontrolmessages.pb.h"

#include <QtGlobal>
#include <QString>

typedef struct RemotePlaylist
{
    qint32 id;
    QString name;
    qint32 item_count;
    bool active;
    bool closed;
    bool favorite;
    bool playing;

public:
    RemotePlaylist() = default;
    RemotePlaylist(const RemotePlaylist &) = default;
    RemotePlaylist(RemotePlaylist &&) = default;

    RemotePlaylist(const pb::remote::Playlist &p, qint32 activePlaylistID):
        id(p.id()), name(p.name().c_str()), item_count(p.item_count()),
        active(p.active()), closed(p.closed()), favorite(p.favorite()),
        playing(p.id() == activePlaylistID)
    {}

    RemotePlaylist& operator=(const RemotePlaylist &) = default;
    RemotePlaylist& operator=(RemotePlaylist &&) = default;

    ~RemotePlaylist() = default;

    inline QString  str() const;
} RemotePlaylist;

QString RemotePlaylist::str() const
{
    return QString("#%1 %2 (nbSongs: %3, active: %4, closed: %5, fav: %6)").arg(
                id).arg(name).arg(item_count).arg(active).arg(closed).arg(favorite);
}

#endif // REMOTEPLAYLIST_H
