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
    _remote(nullptr),
    _useClosedPlaylists(false)
{}

PlaylistModel::PlaylistModel(ClementineRemote *remote, bool useClosedPl, QObject *parent):
    QAbstractListModel(parent),
    _remote(remote),
    _useClosedPlaylists(useClosedPl)
{
    connect(this, &PlaylistModel::preAddPlaylist, this, [=](int index) {
        beginInsertRows(QModelIndex(), index, index);
    }, Qt::DirectConnection);
    connect(this, &PlaylistModel::postAddPlaylist, this, [=]() {
        endInsertRows();
    }, Qt::DirectConnection);
    connect(this, &PlaylistModel::preClearPlaylists, this, [=](int lastIdx) {
        beginRemoveRows(QModelIndex(), 0, lastIdx);
    }, Qt::DirectConnection);
    connect(this, &PlaylistModel::postClearPlaylists, this, [=]() {
        endRemoveRows();
    }, Qt::DirectConnection);
}


int PlaylistModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid() || !_remote)
        return 0;

    return _remote->numberOfPlaylists(_useClosedPlaylists);
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !_remote)
        return QVariant();

    RemotePlaylist *p = _remote->playlist(index.row(), _useClosedPlaylists);
    switch (role) {
    case PlaylistModel::name:
        return p->name;
    case PlaylistModel::iconSrc:
        if (p->playing)
            return "icons/media-playback-start.png";
        else if (p->favorite)
            return "icons/star.png";
        else
            return "icons/playlistFile.png";
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

//void PlaylistModel::setRemote(ClementineRemote *remote)
//{
//    beginResetModel();
//    if (_remote)
//        _remote->disconnect(this);
//    _remote = remote;
//    endResetModel();
//}

