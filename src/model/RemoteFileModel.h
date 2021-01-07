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

#ifndef REMOTEFILEMODEL_H
#define REMOTEFILEMODEL_H
#include <QAbstractListModel>

class ClementineRemote;

class RemoteFileModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(ClementineRemote *remote READ remote WRITE setRemote)

    static const QHash<int, QByteArray> sRoleNames;

public:
    explicit RemoteFileModel(QObject *parent = nullptr);

    enum RemoteFileRole {
        filename = Qt::UserRole,
        isDir,
        selected
    };

    Q_INVOKABLE void selectAllFiles(bool select);
    Q_INVOKABLE void select(int row, bool select);


    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    inline virtual QHash<int, QByteArray> roleNames() const override;


    ClementineRemote *remote() const;
    void setRemote(ClementineRemote *remote);

private:
    ClementineRemote *_remote;
};

QHash<int, QByteArray> RemoteFileModel::roleNames() const { return sRoleNames; }


#endif // REMOTEFILEMODEL_H
