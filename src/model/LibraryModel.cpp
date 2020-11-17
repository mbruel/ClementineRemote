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

#include "LibraryModel.h"

const QHash<int, QByteArray> LibraryModel::sRoleNames = {
    {ItemRole::name,         "name"},
    {ItemRole::type,         "type"},
    {ItemRole::url,          "url"}
};

LibraryModel::LibraryModel(QObject *parent): QStandardItemModel(parent)
{
    connect(this, &LibraryModel::beginReset, this, [=](){beginResetModel();});
    connect(this, &LibraryModel::endReset,   this, [=](){endResetModel();});
}


LibraryProxyModel::LibraryProxyModel(QObject *parent) : QSortFilterProxyModel(parent) {}

bool LibraryProxyModel::isTrack(const QModelIndex &index) const
{
    if (index.isValid())
        // Track and Playlists ;)
        return data(index, LibraryModel::type).toInt() >= LibraryModel::Track;
    else
        return false;
}

QVariantList LibraryProxyModel::getExpandableIndexes(const QModelIndex &currentIndex) const
{
    if (!currentIndex.isValid() || !rowCount(currentIndex))
        return QVariantList();

    QVariantList expandableIndexes = {currentIndex};
    if (data(currentIndex, LibraryModel::type).toInt() == LibraryModel::Artist)
    { // Albums have only Tracks under them which are not expendable ;)
        int childCount = rowCount(currentIndex);
        for (int i = 0; i < childCount ; ++i)
            expandableIndexes << index(i, 0, currentIndex);
    }
    return expandableIndexes;
}

bool LibraryProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (filterRegExp().isEmpty())
        return true;

    if (_matchesTheSearch(sourceRow, sourceParent))
        return true;

    // accept if any of the parents is accepted
    QModelIndex parent = sourceParent;
    while (parent.isValid()) {
        if (_matchesTheSearch(parent.row(), parent.parent()))
            return true;
        parent = parent.parent();
    }

    // accept if any of the children is accepted
    if (_hasAcceptedChildren(sourceRow, sourceParent))
        return true;

    return false;
}


bool LibraryProxyModel::_hasAcceptedChildren(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex modelIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!modelIndex.isValid()) {
        return false;
    }

    //check if there are children
    int childCount = sourceModel()->rowCount(modelIndex);
    if (childCount == 0)
        return false;

    for (int i = 0; i < childCount; ++i) {
        if (_matchesTheSearch(i, modelIndex))
            return true;
        //recursive call -> NOTICE that this is depth-first searching, you're probably better off with breadth first search...
        if (_hasAcceptedChildren(i, modelIndex))
            return true;
    }

    return false;
}
