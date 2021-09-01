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

#ifndef REMOTESONGMODEL_H
#define REMOTESONGMODEL_H
#include <QSortFilterProxyModel>
#include <QAbstractListModel>

class ClementineRemote;
#ifndef OPAQUE_ClementineRemote
#define OPAQUE_ClementineRemote
// To avoid Qt6 error: Type argument of Q_PROPERTY must be fully defined
// cf https://www.qt.io/blog/whats-new-in-qmetatype-qvariant
//Q_MOC_INCLUDE("ClementineRemote.h")
Q_DECLARE_OPAQUE_POINTER(ClementineRemote*)
#endif // OPAQUE_ClementineRemote

class RemoteSongModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(ClementineRemote *remote READ remote WRITE setRemote)

    static const QHash<int, QByteArray> sRoleNames;

public:
    explicit RemoteSongModel(QObject *parent = nullptr);

    enum SongRole {
        title = Qt::UserRole,
        track,
        artist,
        album,
        length,
        pretty_length,
        selected,
        songIndex,
        songId,
        url
    };

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Editable (only for selected role...)
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    inline virtual QHash<int, QByteArray> roleNames() const override;


    ClementineRemote *remote() const;
    void setRemote(ClementineRemote *remote);

private:
    ClementineRemote *_remote;
};

QHash<int, QByteArray> RemoteSongModel::roleNames() const { return sRoleNames; }



class RemoteSongProxyModel : public QSortFilterProxyModel {

public:
    RemoteSongProxyModel(QObject *parent = nullptr);
    ~RemoteSongProxyModel() override = default;

    bool allSongsSelected() const;
    void selectAllSongs(bool selectAll);
    QList<int>  selectedSongsIdexes();
    QList<int>  selectedSongsIDs();
    QStringList selectedSongsURLs();


protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

};


#endif // REMOTESONGMODEL_H
