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

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "ClementineRemote.h"
#include "ClementineSession.h"
#include "model/RemoteSongModel.h"
#include "model/PlaylistModel.h"
#include "model/RemoteFileModel.h"
#include "model/RadioStreamModel.h"
#include "model/LibraryModel.h"
int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);
    app.setApplicationName(ClementineRemote::appName());
    app.setApplicationVersion(ClementineRemote::appVersion());
    app.setApplicationDisplayName(ClementineRemote::appTitle());
    app.setOrganizationName(ClementineRemote::appName());
    app.setWindowIcon(QIcon(":/icon.png"));
//    QCoreApplication::setOrganizationDomain("clementine-player.org");

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);


    qmlRegisterType<RemoteSongModel>("PlayList", 1, 0, "RemoteSongModel");
    qmlRegisterType<RemoteSongProxyModel>("PlayList", 1, 0, "RemoteSongProxyModel");
    qmlRegisterType<PlaylistModel>(  "PlayList", 1, 0, "PlaylistModel");
    qmlRegisterInterface<QAbstractItemModel>("PlayList", 1);

    qmlRegisterType<RemoteFileModel>( "RemoteFile", 1, 0, "RemoteFileModel");
    qmlRegisterType<RemoteFileProxyModel>( "RemoteFile", 1, 0, "RemoteFileProxyModel");
    qmlRegisterType<RadioStreamModel>("RadioStream", 1, 0, "RadioStreamModel");

    qmlRegisterType<LibraryModel>("Library", 1, 0, "LibraryModel");
    qmlRegisterType<LibraryProxyModel>("Library", 1, 0, "LibraryProxyModel");
    qmlRegisterInterface<QAbstractItemModel>("Library", 1);


    qmlRegisterUncreatableType<ClementineSession>("Session", 1, 0, "ClementineSession",
                                                  QStringLiteral("ClementineSession should not be created in QML"));
    qmlRegisterUncreatableType<ClementineRemote>("PlayList",    1, 0, "ClementineRemote",
                                                 QStringLiteral("ClementineRemote should not be created in QML"));
    qmlRegisterUncreatableType<ClementineRemote>("RemoteFile",  1, 0, "ClementineRemote",
                                                 QStringLiteral("ClementineRemote should not be created in QML"));
    qmlRegisterUncreatableType<ClementineRemote>("RadioStream", 1, 0, "ClementineRemote",
                                                 QStringLiteral("ClementineRemote should not be created in QML"));
    qmlRegisterUncreatableType<ClementineRemote>("Session", 1, 0, "ClementineRemote",
                                                 QStringLiteral("ClementineRemote should not be created in QML"));


    QPointer<ClementineRemote> remote = QPointer<ClementineRemote>(ClementineRemote::getInstance());
    engine.rootContext()->setContextProperty("cppRemote", remote.data());

    // really important, otherwise QML takes the ownership
    // which means the model get deleted when any View is...
    // This wouldn't happen if the Model has already a parent
    // (cf http://doc.qt.io/qt-5/qtqml-cppintegration-data.html#data-ownership )
    engine.setObjectOwnership(remote->modelOpenedPlaylists(), QQmlEngine::CppOwnership);
    engine.setObjectOwnership(remote->modelClosedPlaylists(), QQmlEngine::CppOwnership);
    engine.setObjectOwnership(remote->modelRemoteSongs(), QQmlEngine::CppOwnership);
    engine.setObjectOwnership(remote->libraryModel(), QQmlEngine::CppOwnership);

    engine.load(url);

    return app.exec();
}
