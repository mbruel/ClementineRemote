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

#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QAbstractListModel>

class ClementineRemote;

class PlaylistModel : public QAbstractListModel
{
    Q_OBJECT
//    Q_PROPERTY(ClementineRemote *remote READ remote WRITE setRemote)
//    Q_PROPERTY(bool useClosedPl READ useClosedPl WRITE setUsedClosedPl)

    static const QHash<int, QByteArray> sRoleNames;

public:
    explicit PlaylistModel(QObject *parent = nullptr);
    explicit PlaylistModel(ClementineRemote *remote, bool useClosedPl, QObject *parent = nullptr);

    enum PlaylistRole {
        name = Qt::UserRole,
        iconSrc,
        id,
        active,
        closed,
        favorite,
        item_count,
    };


    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    inline virtual QHash<int, QByteArray> roleNames() const override;

    inline ClementineRemote *remote() const;
//    void setRemote(ClementineRemote *remote);

    inline bool useClosedPl() const;
//    inline void setUsedClosedPl(bool useClosed);

signals:
    // signals for PlaylistModel
    void preAddPlaylist(int index);
    void postAddPlaylist(int index);
    void preClearPlaylists(int lastIdx);
    void postClearPlaylists();

private:
    ClementineRemote *const _remote;
    const bool              _useClosedPlaylists;
};

QHash<int, QByteArray> PlaylistModel::roleNames() const { return sRoleNames; }

ClementineRemote *PlaylistModel::remote() const { return _remote; }

bool PlaylistModel::useClosedPl() const { return _useClosedPlaylists; }
//void PlaylistModel::setUsedClosedPl(bool useClosed) { _useClosedPlaylists = useClosed; }

#endif // PLAYLISTMODEL_H
