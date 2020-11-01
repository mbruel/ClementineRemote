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

#include "PlayListModel.h"
#include "ClementineRemote.h"
#include "player/RemoteSong.h"

const QHash<int, QByteArray> PlayListModel::sRoleNames = {
    {SongRole::title,         "title"},
    {SongRole::track,         "track"},
    {SongRole::artist,        "artist"},
    {SongRole::album,         "album"},
    {SongRole::length,        "length"},
    {SongRole::pretty_length, "pretty_length"},
};

PlayListModel::PlayListModel(QObject *parent):
    QAbstractListModel(parent),
    _remote(nullptr)
{}

int PlayListModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid() || !_remote)
        return 0;

    QMutexLocker lock(_remote->secureSongs());
    return _remote->numberOfPlaylistSongs();
}

QVariant PlayListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !_remote)
        return QVariant();

    QMutexLocker lock(_remote->secureSongs());
    const RemoteSong &song = _remote->playlistSong(index.row());
    switch (role) {
    case SongRole::title:
        return QVariant(song.title);
    case SongRole::track:
        return QVariant(song.track);
    case SongRole::artist:
        return QVariant(song.artist);
    case SongRole::album:
        return QVariant(song.album);
    case SongRole::length:
        return QVariant(song.length);
    case SongRole::pretty_length:
        return QVariant(song.pretty_length);
    }

    return QVariant();
}

bool PlayListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!_remote)
        return false;

    bool update = false;
    RemoteSong &song = _remote->playlistSong(index.row());
    switch (role) {
    case SongRole::title:
        if (song.title != value.toString())
        {
            song.title = value.toString();
            update = true;
        }
        break;
    case SongRole::track:
        if (song.track != value.toInt())
        {
            song.track = value.toInt();
            update = true;
        }
        break;
    case SongRole::artist:
        if (song.artist != value.toString())
        {
            song.artist = value.toString();
            update = true;
        }
        break;
    case SongRole::album:
        if (song.album != value.toString())
        {
            song.album = value.toString();
            update = true;
        }
        break;
    case SongRole::length:
        if (song.length != value.toInt())
        {
            song.length = value.toInt();
            update = true;
        }
        break;
    case SongRole::pretty_length:
        if (song.pretty_length != value.toString())
        {
            song.pretty_length = value.toString();
            update = true;
        }
        break;
    }

    if (update) {
        emit dataChanged(index, index, QVector<int>() << role);
        return true;
    }
    return false;
}

Qt::ItemFlags PlayListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
}


ClementineRemote *PlayListModel::remote() const
{
    return _remote;
}

void PlayListModel::setRemote(ClementineRemote *remote)
{
    beginResetModel();

    if (_remote)
        _remote->disconnect(this);

    _remote = remote;

    if (_remote) {
//        connect(_remote, &ClementineRemote::preSongAppended, this, [=]() {
//            const int index = _remote->numberOfPlaylistSongs();
//            beginInsertRows(QModelIndex(), index, index);
//        });
        connect(_remote, &ClementineRemote::preAddSongs, this, [=](int lastSongIdx) {
            beginInsertRows(QModelIndex(), 0, lastSongIdx);
        });
        connect(_remote, &ClementineRemote::postSongAppended, this, [=]() {
            endInsertRows();
        });

//        connect(_remote, &ClementineRemote::preSongRemoved, this, [=](int index) {
//            beginRemoveRows(QModelIndex(), index, index);
//        });
        connect(_remote, &ClementineRemote::preClearSongs, this, [=](int lastSongIdx) {
            beginRemoveRows(QModelIndex(), 0, lastSongIdx);
        });
        connect(_remote, &ClementineRemote::postSongRemoved, this, [=]() {
            endRemoveRows();
        });
    }

    endResetModel();
}


PlayListProxyModel::PlayListProxyModel(QObject *parent): QSortFilterProxyModel(parent)
{}


bool PlayListProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QRegExp regexp = filterRegExp();

    if (regexp.isEmpty())
        return true;

    PlayListModel *model = static_cast<PlayListModel *>(sourceModel());
    QModelIndex srcRowIndex = model->index(sourceRow, 0, sourceParent);

    QString title = model->data(srcRowIndex, PlayListModel::SongRole::title).toString();
    if (title.contains(regexp))
        return true;
    else
    {
        QString artist = model->data(srcRowIndex, PlayListModel::SongRole::artist).toString();
        if (artist.contains(regexp))
            return true;
        else{
            QString album = model->data(srcRowIndex, PlayListModel::SongRole::album).toString();
            if (album.contains(regexp))
                return true;
        }
    }

    return false;
}

