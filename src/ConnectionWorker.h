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

#ifndef CONNECTIONWORKER_H
#define CONNECTIONWORKER_H
#include "protobuf/remotecontrolmessages.pb.h"
#include "utils/Downloader.h"
#include <QTcpSocket>
#include <QByteArray>
#include <QTimer>
class ClementineRemote;
class ClementineSession;
class RemotePlaylist;

/*!
 * \brief manages all the network communications
 * so they can be moved to another thread ;)
 */
class ConnectionWorker : public QObject
{
    Q_OBJECT

private:

    ClementineRemote *_remote;

    QTcpSocket *_socket;
    QTimer      _timeout;
    QString     _disconnectReason;
    bool        _reading_protobuf;
    qint32      _expected_length;
    QByteArray  _buffer;

    // server details
    ClementineSession *_session;

    Downloader      _libraryDL;
    SongsDownloader _songsDL;

    AtomicBool _killingSocket;

public:
    ConnectionWorker(ClementineRemote *remote, QObject *parent = nullptr);
    ~ConnectionWorker();

    inline bool isConnected() const;
    inline const QString &disconnectReason() const;
    inline void setDisconnectReason(const QString &disconnectReason);

    // Sends data to client without check if authenticated
    void sendDataToServer(pb::remote::Message &msg);

    void sendChangeSong(int songIndex, qint32 playlistID);

    void requestSavedRadios();

    void prepareDownload(const pb::remote::ResponseDownloadTotalSize &downloadSize);
    void downloadSong(const pb::remote::ResponseSongFileChunk &songChunk);
    void downloadFinished();

    void downloadLibrary(const pb::remote::ResponseLibraryChunk &libChunk);


    inline void cancelDownload();

signals:
    void connectToServer(ClementineSession *session);
    void getLibrary();
    void disconnectFromServer();
    void killSocket();


private slots:
    void onConnectToServer(ClementineSession *session);
    void onDisconnectFromServer();
    void onKillSocket();


    void onChangeToSong(int proxyRow);
    void onSetTrackPostion(qint32 newPos);
    void onSetVolume(qint32 vol);

    void onNextSong();
    void onPreviousSong();
    void onSetEngineState(qint32 state);
    void onShuffle(ushort mode);
    void onRepeat(ushort mode);

    void onChangePlaylist(qint32 pIdx);

    void onGetServerFiles(QString currentPath, QString subFolder);
    void onSendFilesToAppend();

    void onCreatePlaylist(const QString &newPlaylistName);
    void onSavePlaylist(qint32 playlistID);
    void onRenamePlaylist(qint32 playlistID, const QString &newPlaylistName);

    void onClearPlaylist(qint32 playlistID);
    void onClosePlaylist(qint32 playlistID);
    void onGetAllPlaylists();
    void onOpenPlaylist(int playlistID);


    void onAddRadioToPlaylist(int radioIdx);

    void onSendSongsToRemove();
    void onSendSongsToDownload(const QString &dstFolder);

    void onDownloadCurrentSong();
    void onDownloadPlaylist(qint32 playlistID, QString playlistName);


    void onGetLibrary();
    void onInsertUrls(qint32 playlistID, const QString &newPlaylistName);




// Socket handlers
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSocketTimeout();
    void onError(QAbstractSocket::SocketError err);


private:
    bool createDownloadDestinationFolder(const QString &dstFolder);

    QByteArray sha1Hex(QFile &file);

};

void ConnectionWorker::cancelDownload(){ _songsDL.cancelDownload(); }

bool ConnectionWorker::isConnected() const { return _socket != nullptr; }

const QString &ConnectionWorker::disconnectReason() const { return _disconnectReason; }
void ConnectionWorker::setDisconnectReason(const QString &disconnectReason) { _disconnectReason = disconnectReason; }

#endif // CONNECTIONWORKER_H
