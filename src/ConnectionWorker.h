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

#ifndef CONNECTIONWORKER_H
#define CONNECTIONWORKER_H
#include "protobuf/remotecontrolmessages.pb.h"
#include "player/RemoteSong.h"
#include <QTcpSocket>
#include <QByteArray>
#include <QTimer>
class ClementineRemote;
class RemotePlaylist;
class QFile;

using AtomicBool = QAtomicInteger<unsigned short>; // 16 bit only (faster than using 8 bit variable...)

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    #define M_LoadAtomic(atom) atom.load()
#else
    #define M_LoadAtomic(atom) atom.loadRelaxed()
#endif

struct Downloader{

    Downloader():
        nbFiles(0), totalSize(0),
        chunkNumber(0), chunkCount(0),
        fileNumber(0), fileSize(0),
        song(), dowloadedSize(0), downloadedFiles(0),
        file(nullptr), canWrite(false),
        errorByFileNum(),
        cancel(0x0), hasCancelError(false)
    {}

    ~Downloader();
    Downloader(const Downloader&) = delete;
    Downloader(Downloader&&) = delete;
    Downloader &operator=(const Downloader&) = delete;
    Downloader &operator=(Downloader&&) = delete;

    void cancelDownload() { cancel = 0x1; }
    bool isCancelled() { return M_LoadAtomic(cancel); }

    void init(qint32 nbFiles_, qint32 totalSize_);

    void addError(const QString &err)
    {
        errorByFileNum[fileNumber] = QString("[%1 / %2] %3").arg(
                    fileNumber).arg(nbFiles).arg(err);
    }

    qint32 nbFiles;
    qint32 totalSize;
    qint32 chunkNumber;
    qint32 chunkCount;
    qint32 fileNumber;
    qint32 fileSize;

    RemoteSong song;
    int dowloadedSize;
    int downloadedFiles;

    QFile *file;
    bool   canWrite;

    QMap<int, QString> errorByFileNum;
    AtomicBool cancel;
    bool hasCancelError;
};

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
    QString _host;
    ushort  _port;
    int     _auth_code; //!< -1 <=> no pass

    Downloader _downloader;

public:
    ConnectionWorker(ClementineRemote *remote, QObject *parent = nullptr);
    ~ConnectionWorker();

    inline const QString &disconnectReason() const;
    inline void setDisconnectReason(const QString &disconnectReason);

    // Sends data to client without check if authenticated
    void sendDataToServer(pb::remote::Message &msg);

    void requestSavedRadios();

    inline const QString &hostname() const;

    void prepareDownload(const pb::remote::ResponseDownloadTotalSize &downloadSize);
    void downloadSong(const pb::remote::ResponseSongFileChunk &songChunk);

signals:
    void connectToServer(const QString &host, ushort port, int auth_code);
    void disconnectFromServer();
    void killSocket();


private slots:
    void onConnectToServer(const QString &host, ushort port, int auth_code);
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

    void onDownloadCurrentSong();
    void onDownloadPlaylist(qint32 playlistID);





// Socket handlers
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSocketTimeout();
    void onError(QAbstractSocket::SocketError err);


private:
    void _doChangeSong(int songIndex, qint32 playlistID);

    QByteArray sha1Hex(QFile &file);

};

const QString &ConnectionWorker::hostname() const { return _host; }
const QString &ConnectionWorker::disconnectReason() const { return _disconnectReason; }
void ConnectionWorker::setDisconnectReason(const QString &disconnectReason) { _disconnectReason = disconnectReason; }

#endif // CONNECTIONWORKER_H
