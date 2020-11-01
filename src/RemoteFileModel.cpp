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

#include "RemoteFileModel.h"
#include "ClementineRemote.h"
#include "player/RemoteFile.h"

const QHash<int, QByteArray> RemoteFileModel::sRoleNames = {
    {RemoteFileRole::filename,   "filename"},
    {RemoteFileRole::isDir,      "isDir"},
};


RemoteFileModel::RemoteFileModel(QObject *parent):
    QAbstractListModel(parent),
    _remote(nullptr)
{}

int RemoteFileModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid() || !_remote)
        return 0;

    QMutexLocker lock(_remote->secureRemoteFiles());
    return _remote->numberOfRemoteFiles();
}

QVariant RemoteFileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !_remote)
        return QVariant();

    QMutexLocker lock(_remote->secureRemoteFiles());

    // https://stackoverflow.com/questions/9485339/design-pattern-qt-model-view-and-multiple-threads
    if(_remote->numberOfRemoteFiles() <= index.row())
        return QVariant();

    const RemoteFile &file = _remote->remoteFile(index.row());
    switch (role) {
    case RemoteFileRole::filename:
        return file.filename;
    case RemoteFileRole::isDir:
        return file.isDir;
    }

    return QVariant();
}

//bool RemoteFileModel::setData(const QModelIndex &index, const QVariant &value, int role)
//{
//    if (!_remote)
//        return false;

//    bool update = false;
//    RemoteFile &file = _remote->remoteFile(index.row());
//    switch (role) {
//    case RemoteFileRole::filename:
//        if (file.filename != value.toString())
//        {
//            file.filename = value.toString();
//            update = true;
//        }
//        break;
//    case RemoteFileRole::isDir:
//        if (file.isDir != value.toInt())
//        {
//            file.isDir = value.toBool();
//            update = true;
//        }
//        break;
//    }

//    if (update) {
//        emit dataChanged(index, index, QVector<int>() << role);
//        return true;
//    }
//    return false;
//}


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
