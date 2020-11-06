#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "ClementineRemote.h"
#include "RemoteSongModel.h"
#include "PlaylistModel.h"
#include "RemoteFileModel.h"
#include "RadioStreamModel.h"
int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);
    app.setApplicationName(ClementineRemote::appName());
    app.setApplicationVersion(ClementineRemote::appVersion());
    app.setApplicationDisplayName(ClementineRemote::appTitle());

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);


    qmlRegisterType<RemoteSongModel>("PlayList", 1, 0, "RemoteSongModel");
    qmlRegisterType<PlaylistModel>(  "PlayList", 1, 0, "PlaylistModel");
    qmlRegisterInterface<QAbstractItemModel>("PlayList", 1);

    qmlRegisterType<RemoteFileModel>( "RemoteFile", 1, 0, "RemoteFileModel");
    qmlRegisterType<RadioStreamModel>("RadioStream", 1, 0, "RadioStreamModel");

    qmlRegisterUncreatableType<ClementineRemote>("PlayList",    1, 0, "ClementineRemote",
        QStringLiteral("ClementineRemote should not be created in QML"));
    qmlRegisterUncreatableType<ClementineRemote>("RemoteFile",  1, 0, "ClementineRemote",
        QStringLiteral("ClementineRemote should not be created in QML"));
    qmlRegisterUncreatableType<ClementineRemote>("RadioStream", 1, 0, "ClementineRemote",
        QStringLiteral("ClementineRemote should not be created in QML"));

    QPointer<ClementineRemote> remote = QPointer<ClementineRemote>(ClementineRemote::getInstance());
    engine.rootContext()->setContextProperty("cppRemote", remote.data());

    // really important, otherwise QML takes the ownership
    // which means the model get deleted when any View is...
    // This wouldn't happen if the Model has already a parent
    // (cf http://doc.qt.io/qt-5/qtqml-cppintegration-data.html#data-ownership )
    engine.setObjectOwnership(remote->playListModel(), QQmlEngine::CppOwnership);

    engine.load(url);

    return app.exec();
}
