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

#include "PlaylistModel.h"
#include "ClementineRemote.h"
#include "player/RemotePlaylist.h"

const QHash<int, QByteArray> PlaylistModel::sRoleNames = {
    {PlaylistModel::name,       "name"},
    {PlaylistModel::iconSrc,    "iconSrc"},
    {PlaylistModel::id,         "id"},
    {PlaylistModel::active,     "active"},
    {PlaylistModel::closed,     "closed"},
    {PlaylistModel::favorite,   "favorite"},
    {PlaylistModel::item_count, "item_count"},
};


PlaylistModel::PlaylistModel(QObject *parent):
    QAbstractListModel(parent),
    _remote(nullptr)
{}


int PlaylistModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid() || !_remote)
        return 0;

    return _remote->numberOfPlaylists();
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !_remote)
        return QVariant();

    RemotePlaylist *p = _remote->playlist(index.row());
    switch (role) {
    case PlaylistModel::name:
        return p->name;
    case PlaylistModel::iconSrc:
        if (p->id == _remote->activePlaylistID())
            return "icons/media-playback-start.png";
        else if (p->favorite)
            return "icons/star.png";
        else
            return "icons/tmpFile.png";
    case PlaylistModel::id:
        return p->id;
    case PlaylistModel::active:
        return p->active;
    case PlaylistModel::closed:
        return p->closed;
    case PlaylistModel::favorite:
        return p->favorite;
    case PlaylistModel::item_count:
        return p->item_count;
    }

    return QVariant();
}

Qt::ItemFlags PlaylistModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
}


ClementineRemote *PlaylistModel::remote() const
{
    return _remote;
}

void PlaylistModel::setRemote(ClementineRemote *remote)
{
    beginResetModel();

    if (_remote)
        _remote->disconnect(this);

    _remote = remote;

    if (_remote) {
        connect(_remote, &ClementineRemote::preAddPlaylists, this, [=](int lastIdx) {
            beginInsertRows(QModelIndex(), 0, lastIdx);
        });
        connect(_remote, &ClementineRemote::postAddPlaylists, this, [=]() {
            endInsertRows();
        });
        connect(_remote, &ClementineRemote::preClearPlaylists, this, [=](int lastIdx) {
            beginRemoveRows(QModelIndex(), 0, lastIdx);
        });
        connect(_remote, &ClementineRemote::postClearPlaylists, this, [=]() {
            endRemoveRows();
        });
    }

    endResetModel();
}

