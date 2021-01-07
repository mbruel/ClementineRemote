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

#ifndef LIBRARYMODEL_H
#define LIBRARYMODEL_H
#include <QStandardItemModel>
#include <QSortFilterProxyModel>

class LibraryModel : public QStandardItemModel
{
    Q_OBJECT

    static const QHash<int, QByteArray> sRoleNames;

public:
    explicit LibraryModel(QObject *parent = nullptr);

    enum ItemType {
        Artist = 0,
        Album,
        Track,
        Playlist
    };

    enum ItemRole {
        name = Qt::DisplayRole,
        type = Qt::UserRole,
        url
    };

    inline virtual QHash<int, QByteArray> roleNames() const override;

signals:
    void beginReset();
    void endReset();
};

QHash<int, QByteArray> LibraryModel::roleNames() const { return sRoleNames; }


class LibraryProxyModel : public QSortFilterProxyModel {

public:
    explicit LibraryProxyModel(QObject *parent = nullptr);

    bool isTrack(const QModelIndex &index) const;
    QVariantList getExpandableIndexes(const QModelIndex &currentIndex) const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;


private:
    inline bool _matchesTheSearch(int sourceRow, const QModelIndex &sourceParent) const;
    bool _hasAcceptedChildren(int sourceRow, const QModelIndex &sourceParent) const;
};

bool LibraryProxyModel::_matchesTheSearch(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex modelIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    QString name = sourceModel()->data(modelIndex, LibraryModel::name).toString();
    return name.contains(filterRegExp());
}

#endif // LIBRARYMODEL_H
