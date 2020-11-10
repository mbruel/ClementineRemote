//========================================================================
//
// Copyright (C) 2020 Matthieu Bruel <Matthieu.Bruel@gmail.com>
// Code reused from clementine-player project: https://github.com/clementine-player/Clementine
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

#include "ConnectionWorker.h"
#include "ClementineRemote.h"
#include "player/RemotePlaylist.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QCryptographicHash>

ConnectionWorker::ConnectionWorker(ClementineRemote *remote, QObject *parent) :
    QObject(parent),
    _remote(remote),
    _socket(nullptr), _timeout(), _disconnectReason(" "),
    _reading_protobuf(false), _expected_length(0), _buffer(),
    _host(), _port(0), _auth_code(-1),
    _downloader()
{
    setObjectName("ConnectionWorker");

#ifdef __USE_CONNECTION_THREAD__
    Qt::ConnectionType connectionType = Qt::QueuedConnection;
#else
    Qt::ConnectionType connectionType = Qt::DirectConnection;
#endif

    connect(_remote, &ClementineRemote::connectToServer,      this, &ConnectionWorker::onConnectToServer,      connectionType);
    connect(_remote, &ClementineRemote::disconnectFromServer, this, &ConnectionWorker::onDisconnectFromServer, connectionType);
    connect(_remote, &ClementineRemote::changeToSong,         this, &ConnectionWorker::onChangeToSong,         connectionType);
    connect(_remote, &ClementineRemote::setTrackPostion,      this, &ConnectionWorker::onSetTrackPostion,      connectionType);
    connect(_remote, &ClementineRemote::setVolume,            this, &ConnectionWorker::onSetVolume,            connectionType);
    connect(_remote, &ClementineRemote::nextSong,             this, &ConnectionWorker::onNextSong,             connectionType);
    connect(_remote, &ClementineRemote::previousSong,         this, &ConnectionWorker::onPreviousSong,         connectionType);
    connect(_remote, &ClementineRemote::setEngineState,       this, &ConnectionWorker::onSetEngineState,       connectionType);
    connect(_remote, &ClementineRemote::shuffle,              this, &ConnectionWorker::onShuffle,              connectionType);
    connect(_remote, &ClementineRemote::repeat,               this, &ConnectionWorker::onRepeat,               connectionType);
    connect(_remote, &ClementineRemote::changePlaylist,       this, &ConnectionWorker::onChangePlaylist,       connectionType);
    connect(_remote, &ClementineRemote::getServerFiles,       this, &ConnectionWorker::onGetServerFiles,       connectionType);
    connect(_remote, &ClementineRemote::sendFilesToAppend,    this, &ConnectionWorker::onSendFilesToAppend,    connectionType);
    connect(_remote, &ClementineRemote::savePlaylist,         this, &ConnectionWorker::onSavePlaylist,         connectionType);
    connect(_remote, &ClementineRemote::renamePlaylist,       this, &ConnectionWorker::onRenamePlaylist,       connectionType);
    connect(_remote, &ClementineRemote::createPlaylist,       this, &ConnectionWorker::onCreatePlaylist,       connectionType);
    connect(_remote, &ClementineRemote::clearPlaylist,        this, &ConnectionWorker::onClearPlaylist,        connectionType);
    connect(_remote, &ClementineRemote::closePlaylist,        this, &ConnectionWorker::onClosePlaylist,        connectionType);
    connect(_remote, &ClementineRemote::addRadioToPlaylist,   this, &ConnectionWorker::onAddRadioToPlaylist,   connectionType);
    connect(_remote, &ClementineRemote::sendSongsToRemove,    this, &ConnectionWorker::onSendSongsToRemove,    connectionType);
    connect(_remote, &ClementineRemote::getAllPlaylists,      this, &ConnectionWorker::onGetAllPlaylists,      connectionType);
    connect(_remote, &ClementineRemote::openPlaylist,         this, &ConnectionWorker::onOpenPlaylist,         connectionType);
    connect(_remote, &ClementineRemote::downloadCurrentSong,  this, &ConnectionWorker::onDownloadCurrentSong,  connectionType);
    connect(_remote, &ClementineRemote::downloadPlaylist,     this, &ConnectionWorker::onDownloadPlaylist,     connectionType);


    connect(&_timeout, &QTimer::timeout,             this, &ConnectionWorker::onSocketTimeout, Qt::DirectConnection);
    connect(this,     &ConnectionWorker::killSocket, this, &ConnectionWorker::onKillSocket,    connectionType);
}

ConnectionWorker::~ConnectionWorker()
{
    onKillSocket();
}

void ConnectionWorker::onKillSocket(){
    if (_socket)
    {
        qDebug() << "[MB_TRACE][ConnectionWorker::onKillSocket]";
        disconnect(_socket, &QAbstractSocket::disconnected, this, &ConnectionWorker::onDisconnected);
        disconnect(_socket, &QIODevice::readyRead,          this, &ConnectionWorker::onReadyRead);
        _socket->disconnectFromHost();
        if (_socket->state() != QAbstractSocket::UnconnectedState)
            _socket->waitForDisconnected();
        delete _socket;
        _socket = nullptr;
    }
}


void ConnectionWorker::onConnectToServer(const QString &host, ushort port, int auth_code)
{
    _host      = host;
    _port      = port;
    _auth_code = auth_code;
    _socket = new QTcpSocket();

//    _socket->moveToThread(&_thread);

    _socket->setSocketOption(QAbstractSocket::KeepAliveOption, true);
    _socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    //    _socket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, NgPost::articleSize());

    connect(_socket, &QAbstractSocket::connected,    this, &ConnectionWorker::onConnected,    Qt::DirectConnection);
    connect(_socket, &QAbstractSocket::disconnected, this, &ConnectionWorker::onDisconnected, Qt::DirectConnection);
    connect(_socket, &QIODevice::readyRead,          this, &ConnectionWorker::onReadyRead,    Qt::DirectConnection);

    qRegisterMetaType<QAbstractSocket::SocketError>("SocketError" );
    connect(_socket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(onError(QAbstractSocket::SocketError)), Qt::DirectConnection);
    connect(_socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)),
            this, SLOT(onError(QAbstractSocket::SocketError)), Qt::DirectConnection);

    _socket->connectToHost(host, port);
    _timeout.start(_remote->sockTimeoutMs());
}

void ConnectionWorker::onDisconnectFromServer()
{
    _socket->disconnectFromHost();
}

void ConnectionWorker::onNextSong()
{
    qDebug() << "[ConnectionWorker::onNextSong]";
    onChangeToSong(1 + static_cast<int>(_remote->currentSongIndex()));
}

void ConnectionWorker::onPreviousSong()
{
    qDebug() << "[ConnectionWorker::onPreviousSong]";
    int currentIndex = static_cast<int>(_remote->currentSongIndex()) - 1;
    if (currentIndex >= 0 )
        onChangeToSong(currentIndex);
}

void ConnectionWorker::onSetEngineState(qint32 state)
{
    qDebug() << "[ConnectionWorker::onSetEngineState] state: " << state;

    pb::remote::Message msg;
    switch (static_cast<pb::remote::EngineState>(state)) {
      case pb::remote::EngineState::Playing:
        msg.set_type(pb::remote::PLAY);
        break;
      case pb::remote::EngineState::Paused:
        msg.set_type(pb::remote::PAUSE);
        break;
      case pb::remote::EngineState::Empty:
        msg.set_type(pb::remote::STOP);  // Empty is called when player stopped
        break;
      default:
        msg.set_type(pb::remote::STOP);
        break;
    }


    sendDataToServer(msg);

    // Hack to make sure last played song will be resumed
    if (_remote->playerPreviousState() == pb::remote::EngineState::Idle && msg.type() == pb::remote::PLAY )
    {
        qDebug() << "[ConnectionWorker::onSetEngineState] try to play previous song: " << _remote->currentSong().title;
        _doChangeSong(static_cast<int>(_remote->currentSongIndex()), _remote->activePlaylistID());
    }

}

void ConnectionWorker::onShuffle(ushort mode)
{
    pb::remote::Message msg;
    msg.set_type(pb::remote::SHUFFLE);

    switch (mode) {
      case 0:
        msg.mutable_shuffle()->set_shuffle_mode(pb::remote::Shuffle_Off);
        break;
      case 1:
        msg.mutable_shuffle()->set_shuffle_mode(pb::remote::Shuffle_All);
        break;
      case 2:
        msg.mutable_shuffle()->set_shuffle_mode(pb::remote::Shuffle_InsideAlbum);
        break;
      case 3:
        msg.mutable_shuffle()->set_shuffle_mode(pb::remote::Shuffle_Albums);
        break;
    }

    sendDataToServer(msg);
}

void ConnectionWorker::onRepeat(ushort mode)
{
    pb::remote::Message msg;
    msg.set_type(pb::remote::REPEAT);

    switch (mode) {
      case 0:
        msg.mutable_repeat()->set_repeat_mode(pb::remote::Repeat_Off);
        break;
      case 1:
        msg.mutable_repeat()->set_repeat_mode(pb::remote::Repeat_Track);
        break;
      case 2:
        msg.mutable_repeat()->set_repeat_mode(pb::remote::Repeat_Album);
        break;
      case 3:
        msg.mutable_repeat()->set_repeat_mode(pb::remote::Repeat_Playlist);
        break;
//      case PlaylistSequence::Repeat_OneByOne:
//        msg.mutable_repeat()->set_repeat_mode(pb::remote::Repeat_OneByOne);
//        break;
//      case PlaylistSequence::Repeat_Intro:
//        msg.mutable_repeat()->set_repeat_mode(pb::remote::Repeat_Intro);
//        break;
    }

    sendDataToServer(msg);
}

void ConnectionWorker::onChangePlaylist(qint32 pIdx)
{
    qDebug() << "[ConnectionWorker::onChangePlaylist] playlistIdx: " << pIdx;

    pb::remote::Message msg;
    msg.set_type(pb::remote::REQUEST_PLAYLIST_SONGS);
    msg.mutable_request_playlist_songs()->set_id(_remote->playlist(pIdx)->id);

    sendDataToServer(msg);
}

void ConnectionWorker::onGetServerFiles(QString currentPath, QString subFolder)
{
    if (currentPath.isEmpty())
        currentPath = "./";
    if (!subFolder.isEmpty())
    {
        if (!currentPath.endsWith("/"))
            currentPath += "/";
        currentPath += subFolder;
    }
    qDebug() << "[ConnectionWorker::onGetServerFiles] remote path: " << currentPath;

    pb::remote::Message msg;
    msg.set_type(pb::remote::REQUEST_FILES);
    msg.mutable_request_list_files()->set_relative_path(currentPath.toStdString());

    sendDataToServer(msg);
}

void ConnectionWorker::onSendFilesToAppend()
{
    _remote->doSendFilesToAppend();
}

void ConnectionWorker::onCreatePlaylist(const QString &newPlaylistName)
{
    pb::remote::Message msg;
    msg.set_type(pb::remote::UPDATE_PLAYLIST);
    msg.mutable_request_update_playlist()->set_create_new_playlist(true);
    msg.mutable_request_update_playlist()->set_new_playlist_name(newPlaylistName.toStdString());
    sendDataToServer(msg);
}

void ConnectionWorker::onSavePlaylist(qint32 playlistID)
{
    pb::remote::Message msg;
    msg.set_type(pb::remote::UPDATE_PLAYLIST);
    msg.mutable_request_update_playlist()->set_playlist_id(playlistID);
    msg.mutable_request_update_playlist()->set_favorite(true);
    sendDataToServer(msg);
}

void ConnectionWorker::onRenamePlaylist(qint32 playlistID, const QString &newPlaylistName)
{
    pb::remote::Message msg;
    msg.set_type(pb::remote::UPDATE_PLAYLIST);
    msg.mutable_request_update_playlist()->set_playlist_id(playlistID);
    msg.mutable_request_update_playlist()->set_new_playlist_name(newPlaylistName.toStdString());
    sendDataToServer(msg);
}

void ConnectionWorker::onClearPlaylist(qint32 playlistID)
{
    pb::remote::Message msg;
    msg.set_type(pb::remote::UPDATE_PLAYLIST);
    msg.mutable_request_update_playlist()->set_playlist_id(playlistID);
    msg.mutable_request_update_playlist()->set_clear_playlist(true);
    sendDataToServer(msg);
}

void ConnectionWorker::onClosePlaylist(qint32 playlistID)
{
    pb::remote::Message msg;
    msg.set_type(pb::remote::CLOSE_PLAYLIST);
    msg.mutable_request_close_playlist()->set_playlist_id(playlistID);
    sendDataToServer(msg);

    _remote->closingPlaylist(playlistID);
    onChangePlaylist(_remote->playlistIndex());
}

void ConnectionWorker::onGetAllPlaylists()
{
    pb::remote::Message msg;
    msg.set_type(pb::remote::REQUEST_PLAYLISTS);
    msg.mutable_request_playlists()->set_include_closed(true);
    sendDataToServer(msg);
}

void ConnectionWorker::onOpenPlaylist(int playlistID)
{
    pb::remote::Message msg;
    msg.set_type(pb::remote::OPEN_PLAYLIST);
    msg.mutable_request_open_playlist()->set_playlist_id(playlistID);
    sendDataToServer(msg);
}

void ConnectionWorker::onAddRadioToPlaylist(int radioIdx)
{
    pb::remote::Message msg;
    msg.set_type(pb::remote::INSERT_URLS);

    pb::remote::RequestInsertUrls *req = msg.mutable_request_insert_urls();
    req->set_playlist_id(_remote->currentPlaylistID());
    req->set_play_now(true);
    *req->add_urls() = _remote->radioStream(radioIdx).url.toStdString();

    sendDataToServer(msg);
}

void ConnectionWorker::onSendSongsToRemove()
{
    _remote->doSendSongsToRemove();
}

void ConnectionWorker::onDownloadCurrentSong()
{
    _downloader.downloadPath = _remote->downloadPath();
    pb::remote::Message msg;
    msg.set_type(pb::remote::DOWNLOAD_SONGS);

    pb::remote::RequestDownloadSongs *req = msg.mutable_request_download_songs();
    req->set_download_item(pb::remote::DownloadItem::CurrentItem);
    sendDataToServer(msg);
}

void ConnectionWorker::onDownloadPlaylist(qint32 playlistID, QString playlistName)
{
    QString downloadPath = QString("%1/playlists/%2").arg(_remote->downloadPath()).arg(playlistName);
    QFileInfo fi(downloadPath);
    if (fi.exists())
    {
        if (!fi.isDir())
        {
            emit _remote->downloadComplete(0, 0, {
                                           tr("%1 already exist but is not a directory...").arg(
                                           downloadPath)
                                       });
            return;
        }
    }
    else
    {
        QDir dir(_remote->downloadPath());
        if (!dir.mkpath(QString("playlists/%1").arg(playlistName)))
        {
            emit _remote->downloadComplete(0, 0, {
                                               tr("Error creating download folder %1").arg(
                                               downloadPath)
                                           });
            return;
        }
    }

    _downloader.downloadPath = downloadPath;
    pb::remote::Message msg;
    msg.set_type(pb::remote::DOWNLOAD_SONGS);

    pb::remote::RequestDownloadSongs *req = msg.mutable_request_download_songs();
    req->set_download_item(pb::remote::DownloadItem::APlaylist);
    req->set_playlist_id(playlistID);
    sendDataToServer(msg);
}

void ConnectionWorker::_doChangeSong(int songIndex, qint32 playlistID)
{
    const RemoteSong &song = _remote->playlistSong(songIndex);

    pb::remote::Message msg;
    msg.set_type(pb::remote::CHANGE_SONG);
    pb::remote::RequestChangeSong *request = msg.mutable_request_change_song();
    if (playlistID != -1)
        request->set_playlist_id(playlistID);

    qDebug() << "[ConnectionWorker::onChangeToSong] idx: " << songIndex << " : "<< song.str();

    request->set_song_index(song.index);

    sendDataToServer(msg);
}

void ConnectionWorker::onChangeToSong(int proxyRow)
{
    int modelRow = _remote->modelRowFromProxyRow(proxyRow);
    if (modelRow != -1)
    {
        _doChangeSong(modelRow, _remote->currentPlaylistID());

        // this will play music so we need to update the remote
        _remote->setPlay();
        emit _remote->updateEngineState();
    }
    else
        qCritical() << "[ConnectionWorker::onChangeToSong] ERROR getting model row from proxy one: " << proxyRow;
}

void ConnectionWorker::onSetTrackPostion(qint32 newPos)
{
    pb::remote::Message msg;
    msg.set_type(pb::remote::SET_TRACK_POSITION);
    msg.mutable_request_set_track_position()->set_position(newPos);

    qDebug() << "[ConnectionWorker::onSetTrackPostion] new Pos: " << newPos
             << " ("<< _remote->prettyLength(newPos) << " for track: " << _remote->currentSong().title;

    sendDataToServer(msg);
}

void ConnectionWorker::onSetVolume(qint32 vol)
{
    pb::remote::Message msg;
    msg.set_type(pb::remote::SET_VOLUME);
    msg.mutable_request_set_volume()->set_volume(vol);
    qDebug() << "[ConnectionWorker::onSetVolume] new volume: " << vol;
    sendDataToServer(msg);
}

void ConnectionWorker::onConnected()
{
    _timeout.stop();

    _remote->saveSettings(_host, QString::number(_port), QString::number(_auth_code));

    qDebug() << "[ConnectionWorker::onConnected] to server " << _host << ":" << _port;
    // Create the general message and set the message type
    pb::remote::Message msg;
    msg.set_type(pb::remote::CONNECT);

    pb::remote::RequestConnect *reqConnect = msg.mutable_request_connect();
    if (_auth_code != -1)
        reqConnect->set_auth_code(_auth_code);

    sendDataToServer(msg);

//    emit _remote->connected();
}

void ConnectionWorker::onDisconnected()
{
    qDebug() << "[ConnectionWorker::onDisconnected] from server " << _host << ":" << _port;
    if (_timeout.isActive())
        _timeout.stop();

    disconnect(_socket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(onError(QAbstractSocket::SocketError)));
    disconnect(_socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)),
            this, SLOT(onError(QAbstractSocket::SocketError)));
    disconnect(_socket, &QIODevice::readyRead, this, &ConnectionWorker::onReadyRead);


    _socket->deleteLater();
    _socket = nullptr;

    _downloader.init(0, 0);

    emit _remote->disconnected(_disconnectReason);
}

void ConnectionWorker::onReadyRead()
{
    while (_socket->bytesAvailable()) {
        if (!_reading_protobuf) {
            // If we have less than 4 byte, we cannot read the length. Wait for more
            // data
            if (_socket->bytesAvailable() < 4) {
                break;
            }
            // Read the length of the next message
            QDataStream s(_socket);
            s >> _expected_length;

            // Receiving more than 128mb is very unlikely
            // Flush the data and disconnect the client
            if (_expected_length > 134217728) {
                qDebug() << "Received invalid data, disconnect client";
                qDebug() << "_expected_length =" << _expected_length;
                _socket->close();
                return;
            }

            _reading_protobuf = true;
        }

        // Read some of the message
        _buffer.append(_socket->read(_expected_length - _buffer.size()));

        // Did we get everything?
        if (_buffer.size() == _expected_length) {
            // Parse the message
            _remote->parseMessage(_buffer);

            // Clear the buffer
            _buffer.clear();
            _reading_protobuf = false;
        }
    }
}

void ConnectionWorker::onSocketTimeout()
{
    qDebug() << "[ConnectionWorker::onSocketTimeout] to server " << _host << ":" << _port;
    _timeout.stop();
    emit _remote->connectionError(tr("Unable to connect..."));
    if (_socket)
        onDisconnected();
}

void ConnectionWorker::onError(QAbstractSocket::SocketError err)
{
    qDebug() << "[ConnectionWorker::onError] err: " << err << " : " << _socket->errorString();
    emit _remote->connectionError(_socket->errorString());

    if (err == QAbstractSocket::SocketError::ConnectionRefusedError)
    {
        _disconnectReason = _socket->errorString();
        _timeout.stop();
        onDisconnected();
    }
}



void ConnectionWorker::sendDataToServer(pb::remote::Message &msg)
{
    // Set the default version
    msg.set_version(msg.default_instance().version());

    // Check if we are still connected
    if (_socket->state() == QTcpSocket::ConnectedState) {
        // Serialize the message
        std::string data = msg.SerializeAsString();

        // write the length of the data first
        QDataStream s(_socket);
        s << qint32(data.length());
        s.writeRawData(data.data(), static_cast<int>(data.length()));

        // Do NOT flush data here! If the client is already disconnected, it
        // causes a SIGPIPE termination!!!
    } else {
        qDebug() << "Closed";
        _socket->close();
    }
}

void ConnectionWorker::requestSavedRadios()
{
    qDebug() << "Sending requestSavedRadios";
    pb::remote::Message msg;
    msg.set_type(pb::remote::REQUEST_SAVED_RADIOS);
    sendDataToServer(msg);
}

void ConnectionWorker::prepareDownload(const pb::remote::ResponseDownloadTotalSize &downloadSize)
{
    _downloader.init(downloadSize.file_count(), downloadSize.total_size());
    qDebug() << "[ConnectionWorker::prepareDownload] nbFiles: " << _downloader.nbFiles
             << ", total size: " << _downloader.totalSize;
}

void ConnectionWorker::downloadSong(const pb::remote::ResponseSongFileChunk &songChunk)
{
    bool cancelled = _downloader.isCancelled();

    _downloader.chunkNumber = songChunk.chunk_number();
    _downloader.chunkCount  = songChunk.chunk_count();
    _downloader.fileNumber  = songChunk.file_number();
    _downloader.fileSize    = songChunk.size();

//    qDebug() << "rcv ResponseSongFileChunk File: " << _downloader.fileNumber
//             << " / " << _downloader.nbFiles << " size: " << _downloader.fileSize
//             << " - Chunk " << _downloader.chunkNumber << " / " << _downloader.chunkCount;
//if (_downloader.fileNumber == 2 && _downloader.chunkNumber == 2)
//    _downloader.cancelDownload();

    // Song offer is chunk no 0
    if (_downloader.chunkNumber == 0)
    {
        bool fileExist = false;
        if (songChunk.has_song_metadata() && songChunk.size() != 0)
        {
            _downloader.song = songChunk.song_metadata();
            if (cancelled)
            {
                qDebug() << "Cancelling: " << _downloader.song.filename;
                if (!_downloader.hasCancelError)
                {
                    _downloader.addError(tr("Download cancelled from %1").arg(_downloader.song.filename));
                    _downloader.hasCancelError = true;
                }
            }
            else
            {
                _downloader.file = new QFile(QString("%1/%2").arg(_downloader.downloadPath).arg(_downloader.song.filename));
                if (_downloader.file->exists() && !_remote->overwriteDownloadedSongs())
                {
                    _downloader.addError(tr("skipping file %1").arg(_downloader.song.filename));
                    fileExist = true;
                }
                else
                {
                    _downloader.canWrite = _downloader.file->open(QIODevice::ReadWrite);
                    qDebug() << "rcv ResponseSongFileChunk has SongMeta: "
                             << _downloader.song.str()
                             << ", canWrite: " << _downloader.canWrite;
                    if (!_downloader.canWrite)
                        _downloader.addError(tr("can't write file %1").arg(_downloader.song.filename));
                }
            }
        }
        pb::remote::Message msg;
        msg.set_type(pb::remote::SONG_OFFER_RESPONSE);
        msg.mutable_response_song_offer()->set_accepted(!cancelled && !fileExist);
        sendDataToServer(msg);
    }
    else if (_downloader.canWrite)
    {
        if (cancelled){
            if (_downloader.file)
            {
                qDebug() << "Deleting: " << _downloader.song.filename;
                _downloader.addError(tr("Download cancelled from %1").arg(_downloader.song.filename));
                _downloader.file->remove();
                delete _downloader.file;
                _downloader.file = nullptr;
                _downloader.hasCancelError = true;
            }
            if (_downloader.chunkNumber == _downloader.chunkCount)
                emit _remote->downloadComplete(_downloader.downloadedFiles,
                                               _downloader.nbFiles,
                                               _downloader.errorByFileNum.values());
            return;
        }

        const std::string &data = songChunk.data();
        qint64 bytesWritten = 0, size = static_cast<qint64>(data.size());
        int iterMax = 10, iter = 0;
        do {
            qint64 bytes = _downloader.file->write(data.c_str(), size);
            if (bytes == -1)
            {
                _downloader.addError(tr("error writing file %1 (%2)").arg(
                                         _downloader.song.filename).arg(_downloader.file->errorString()));

                qCritical() << tr("error writing file %1 (%2)").arg(
                                  _downloader.song.filename).arg(_downloader.file->errorString());
                break;
            }
            else
                bytesWritten += bytes;

            if (++iter == iterMax)
            {
                _downloader.addError(tr("error writing file %1 (iterMax reached %2)").arg(
                                         _downloader.song.filename).arg(iterMax));

                qCritical() << "Error writting: "  << _downloader.song.filename
                            << " : iterMax reached " << iterMax;
                break;
            }
        } while (bytesWritten != size);

        _downloader.dowloadedSize += bytesWritten;

        if (_downloader.chunkNumber == _downloader.chunkCount) {
            bool sha1Ok = sha1Hex(*_downloader.file) == songChunk.file_hash().c_str();
            qDebug() << "Dowloaded: "  << _downloader.song.str()
                     << " sha1Ok : " << sha1Ok << " (fileNumber: " << _downloader.fileNumber;
            if (!sha1Ok)
            {
                _downloader.addError(tr("error file %1 (wrong sha1)").arg(
                                         _downloader.song.filename));
                _downloader.file->remove();
            }
            else
            {
                ++_downloader.downloadedFiles;
                _downloader.file->close();
            }

            delete _downloader.file;
            _downloader.file = nullptr;
        }
    }

    if (_downloader.fileNumber == _downloader.nbFiles)
        emit _remote->downloadComplete(_downloader.downloadedFiles,
                                       _downloader.nbFiles,
                                       _downloader.errorByFileNum.values());
}

QByteArray ConnectionWorker::sha1Hex(QFile &file)
{
    file.seek(0);
    QCryptographicHash hash(QCryptographicHash::Sha1);
    QByteArray data;

    while (!file.atEnd()) {
      data = file.read(1000000);  // 1 mib
      hash.addData(data.data(), data.length());
      data.clear();
    }

    return hash.result().toHex();
}

Downloader::~Downloader()
{
    if (file)
    {
        file->remove();
        delete file;
    }
}

void Downloader::init(qint32 nbFiles_, qint32 totalSize_)
{
    nbFiles         = nbFiles_;
    totalSize       = totalSize_;
    chunkNumber     = 0;
    chunkCount      = 0;
    fileNumber      = 0;
    fileSize        = 0;
    song            = RemoteSong();
    dowloadedSize   = 0;
    downloadedFiles = 0;
    if (file){
        file->remove();
        delete file;
    }
    canWrite = false;
    errorByFileNum.clear();
    cancel = 0x0;
    hasCancelError = false;
}


