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

#include "RadioStreamModel.h"
#include "ClementineRemote.h"
#include "player/Stream.h"

const QHash<int, QByteArray> RadioStreamModel::sRoleNames = {
    {RemoteFileRole::name,    "name"},
    {RemoteFileRole::url,     "url"},
    {RemoteFileRole::logoUrl, "logoUrl"},
};


RadioStreamModel::RadioStreamModel(QObject *parent):
    QAbstractListModel(parent),
    _remote(nullptr)
{}


int RadioStreamModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid() || !_remote)
        return 0;

    return _remote->numberOfRadioStreams();
}

QVariant RadioStreamModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !_remote)
        return QVariant();

    const Stream &stream = _remote->radioStream(index.row());
    switch (role) {
    case RadioStreamModel::name:
        return stream.name;
    case RadioStreamModel::url:
        return stream.url;
    case RadioStreamModel::logoUrl:
        return stream.logoUrl;
    }

    return QVariant();
}

Qt::ItemFlags RadioStreamModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
}


ClementineRemote *RadioStreamModel::remote() const
{
    return _remote;
}

void RadioStreamModel::setRemote(ClementineRemote *remote)
{
    beginResetModel();

    if (_remote)
        _remote->disconnect(this);

    _remote = remote;

    if (_remote) {
        connect(_remote, &ClementineRemote::preAddRadioStreams, this, [=](int lastIdx) {
            beginInsertRows(QModelIndex(), 0, lastIdx);
        });
        connect(_remote, &ClementineRemote::postAddRadioStreams, this, [=]() {
            endInsertRows();
        });
        connect(_remote, &ClementineRemote::preClearRadioStreams, this, [=](int lastIdx) {
            beginRemoveRows(QModelIndex(), 0, lastIdx);
        });
        connect(_remote, &ClementineRemote::postClearRadioStreams, this, [=]() {
            endRemoveRows();
        });
    }

    endResetModel();
}

