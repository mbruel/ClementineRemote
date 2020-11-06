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

#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QAbstractListModel>

class ClementineRemote;

class PlaylistModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(ClementineRemote *remote READ remote WRITE setRemote)

    static const QHash<int, QByteArray> sRoleNames;

public:
    explicit PlaylistModel(QObject *parent = nullptr);

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

    ClementineRemote *remote() const;
    void setRemote(ClementineRemote *remote);

private:
    ClementineRemote *_remote;
};

QHash<int, QByteArray> PlaylistModel::roleNames() const { return sRoleNames; }

#endif // PLAYLISTMODEL_H
