#ifndef CLEMENTINESESSION_H
#define CLEMENTINESESSION_H

#include <QObject>

class ClementineSession : public QObject {

    Q_OBJECT

    Q_PROPERTY(QString host READ host WRITE setHost)
    Q_PROPERTY(ushort  port READ port WRITE setPort)
    Q_PROPERTY(int     pass READ pass WRITE setPass)
    Q_PROPERTY(QString remotePath READ remotePath)

public:
    // Providing the c++ parent is really important otherwise QML would take it and destroy the object when changing page...
    ClementineSession(const QString &name, QObject *parent, const QString &host = "192.168.", ushort port = 5500, int pass = -1, const QString &remotePath = "./")
        :QObject(parent), _name(name), _host(host), _port(port), _pass(pass), _remotePath(remotePath){}

    QString name() const {return _name;}
    QString host() const {return _host;}
    ushort  port() const {return _port;}
    int     pass() const {return _pass;}
    QString remotePath() const {return _remotePath;}

    QString str() const {return QString("Session %1: %2@%3:%4/%5").arg(
                    _name).arg(_pass).arg(_host).arg(_port).arg(_remotePath);}

    void setHost(const QString &val) {if (val != _host) _host = val;}
    void setPort(ushort val) {if (val != _port) _port = val;}
    void setPass(int val) {if (val != _pass) _pass = val;}
    void setRemotePath(const QString &val) {if (val != _remotePath) _remotePath = val;}

private:
    QString _name;
    QString _host;
    ushort  _port;
    int     _pass;
    QString _remotePath;
};

#endif // CLEMENTINESESSION_H
