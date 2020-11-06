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

#include "RemoteSongModel.h"
#include "ClementineRemote.h"
#include "player/RemoteSong.h"

const QHash<int, QByteArray> RemoteSongModel::sRoleNames = {
    {SongRole::title,         "title"},
    {SongRole::track,         "track"},
    {SongRole::artist,        "artist"},
    {SongRole::album,         "album"},
    {SongRole::length,        "length"},
    {SongRole::pretty_length, "pretty_length"},
    {SongRole::selected,      "selected"},
    {SongRole::songIndex,     "songIndex"},
    {SongRole::songId,        "songId"},
};

RemoteSongModel::RemoteSongModel(QObject *parent):
    QAbstractListModel(parent),
    _remote(nullptr)
{}


int RemoteSongModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid() || !_remote)
        return 0;

    return _remote->numberOfPlaylistSongs();
}

QVariant RemoteSongModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !_remote)
        return QVariant();

    const RemoteSong &song = _remote->playlistSong(index.row());
    switch (role) {
    case SongRole::title:
        return song.title;
    case SongRole::track:
        return song.track;
    case SongRole::artist:
        return song.artist;
    case SongRole::album:
        return song.album;
    case SongRole::length:
        return song.length;
    case SongRole::pretty_length:
        return song.pretty_length;
    case SongRole::selected:
        return song.selected;
    case SongRole::songIndex:
        return song.index;
    case SongRole::songId:
        return song.id;
    }

    return QVariant();
}

bool RemoteSongModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!_remote)
        return false;

    RemoteSong &song = _remote->playlistSong(index.row());
    switch (role) {
    case SongRole::selected:
        if (song.selected != value.toBool())
        {
            song.selected = value.toBool();
            emit dataChanged(index, index, QVector<int>() << role);
            return true;
        }
        break;
    default:
        return false;
    }

    return false;
}

Qt::ItemFlags RemoteSongModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
}


ClementineRemote *RemoteSongModel::remote() const
{
    return _remote;
}

void RemoteSongModel::setRemote(ClementineRemote *remote)
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


RemoteSongProxyModel::RemoteSongProxyModel(QObject *parent): QSortFilterProxyModel(parent)
{}


bool RemoteSongProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QRegExp regexp = filterRegExp();

    if (regexp.isEmpty())
        return true;

    RemoteSongModel *model = static_cast<RemoteSongModel *>(sourceModel());
    QModelIndex srcRowIndex = model->index(sourceRow, 0, sourceParent);

    QString title = model->data(srcRowIndex, RemoteSongModel::SongRole::title).toString();
    if (title.contains(regexp))
        return true;
    else
    {
        QString artist = model->data(srcRowIndex, RemoteSongModel::SongRole::artist).toString();
        if (artist.contains(regexp))
            return true;
        else{
            QString album = model->data(srcRowIndex, RemoteSongModel::SongRole::album).toString();
            if (album.contains(regexp))
                return true;
        }
    }

    return false;
}


bool RemoteSongProxyModel::allSongsSelected() const
{
    bool allSelected = true;
    for (int i = 0; i < rowCount() ; ++i)
    {
        if (!data(index(i, 0), RemoteSongModel::selected).toBool())
        {
            allSelected = false;
            break;
        }
    }
    return allSelected;
}

void RemoteSongProxyModel::selectAllSongs(bool selectAll)
{
    for (int i = 0; i < rowCount() ; ++i)
        setData(index(i, 0), selectAll, RemoteSongModel::selected);
}

QList<int> RemoteSongProxyModel::selectedSongsIdexes()
{
    QList<int> selectedIndexes;
    for (int i = 0; i < rowCount() ; ++i)
    {
        if (data(index(i, 0), RemoteSongModel::selected).toBool())
            selectedIndexes << data(index(i, 0), RemoteSongModel::songIndex).toInt();
    }
    return selectedIndexes;
}
