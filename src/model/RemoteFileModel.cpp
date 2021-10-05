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

#include "RemoteFileModel.h"
#include "ClementineRemote.h"
#include "player/RemoteFile.h"

const QHash<int, QByteArray> RemoteFileModel::sRoleNames = {
    {RemoteFileRole::filename,   "filename"},
    {RemoteFileRole::isDir,      "isDir"},
    {RemoteFileRole::selected,   "selected"},
};


RemoteFileModel::RemoteFileModel(QObject *parent):
    QAbstractListModel(parent),
    _remote(nullptr)
{}

void RemoteFileModel::selectAllFiles(bool select)
{
    for (int i = 0; i < rowCount() ; ++i)
    {
        if (!data(index(i), RemoteFileRole::isDir).toBool())
            setData(index(i), select, RemoteFileRole::selected);
    }
}

void RemoteFileModel::select(int row, bool select)
{
    QModelIndex idx = index(row);
    if (idx.isValid())
        setData(index(row), select, RemoteFileRole::selected);
}

int RemoteFileModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid() || !_remote)
        return 0;

    return _remote->numberOfRemoteFiles();
}

QVariant RemoteFileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !_remote)
        return QVariant();

    const RemoteFile &file = _remote->remoteFile(index.row());
    switch (role) {
    case RemoteFileRole::filename:
        return file.filename;
    case RemoteFileRole::isDir:
        return file.isDir;
    case RemoteFileRole::selected:
        return file.selected;
    }

    return QVariant();
}

bool RemoteFileModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!_remote)
        return false;

    bool update = false;
    RemoteFile &file = _remote->remoteFile(index.row());
    switch (role) {
    case RemoteFileRole::filename:
        if (file.filename != value.toString())
        {
            file.filename = value.toString();
            update = true;
        }
        break;
    case RemoteFileRole::isDir:
        if (file.isDir != value.toBool())
        {
            file.isDir = value.toBool();
            update = true;
        }
        break;
    case RemoteFileRole::selected:
        if (file.selected != value.toBool())
        {
            file.selected = value.toBool();
            update = true;
        }
        break;
    }

    if (update) {
        emit dataChanged(index, index, {role});
        return true;
    }
    return false;
}


Qt::ItemFlags RemoteFileModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
}


ClementineRemote *RemoteFileModel::remote() const
{
    return _remote;
}

void RemoteFileModel::setRemote(ClementineRemote *remote)
{
    beginResetModel();

    if (_remote)
        _remote->disconnect(this);

    _remote = remote;

    if (_remote) {
        connect(_remote, &ClementineRemote::preAddRemoteFiles, this, [=](int lastIdx) {
            beginInsertRows(QModelIndex(), 0, lastIdx);
        });
        connect(_remote, &ClementineRemote::postAddRemoteFiles, this, [=]() {
            endInsertRows();
        });
        connect(_remote, &ClementineRemote::preClearRemoteFiles, this, [=](int lastIdx) {
            beginRemoveRows(QModelIndex(), 0, lastIdx);
        });
        connect(_remote, &ClementineRemote::postClearRemoteFiles, this, [=]() {
            endRemoveRows();
        });
    }

    endResetModel();
}

RemoteFileProxyModel::RemoteFileProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent)
  , _model(new RemoteFileModel(this))
{
  setSourceModel(_model);
}

void RemoteFileProxyModel::selectAllFiles(bool select_)
{
    for (int i = 0; i < rowCount() ; ++i)
    {
        if (!data(index(i, 0), RemoteFileModel::isDir).toBool())
            select(i, select_);
    }
}

bool RemoteFileProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (!filterRegularExpression().isValid() || filterRegularExpression().pattern().isEmpty())
        return true;

    QModelIndex modelIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    QString name = sourceModel()->data(modelIndex, RemoteFileModel::filename).toString();
    return name.contains(filterRegularExpression());
}
