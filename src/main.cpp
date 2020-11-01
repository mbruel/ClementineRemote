#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "ClementineRemote.h"
#include "PlayListModel.h"
#include "RemoteFileModel.h"

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


    qmlRegisterType<PlayListModel>("PlayList", 1, 0, "PlayListModel");
    qmlRegisterInterface<QAbstractItemModel>("PlayList", 1);
    qmlRegisterUncreatableType<ClementineRemote>("PlayList", 1, 0, "ClementineRemote",
        QStringLiteral("ClementineRemote should not be created in QML"));

    qmlRegisterType<RemoteFileModel>("RemoteFile", 1, 0, "RemoteFileModel");


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
