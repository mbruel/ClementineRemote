//========================================================================
//
// Copyright (C) 2020 Matthieu Bruel <Matthieu.Bruel@gmail.com>
// This file is a part of ClementineRemote : https://github.com/mbruel/ClementineRemote
// Code reused from clementine-player project: https://github.com/clementine-player/Clementine
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

#include "ConnectionWorker.h"
#include "ClementineRemote.h"
#include "ClementineSession.h"
#include "player/RemotePlaylist.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QCryptographicHash>

ConnectionWorker::ConnectionWorker(ClementineRemote *remote, QObject *parent) :
    QObject(parent),
    _remote(remote),
    _socket(nullptr), _timeout(this), _disconnectReason(" "),
    _reading_protobuf(false), _expected_length(0), _buffer(),
    _session(nullptr),
    _libraryDL(), _songsDL(),
    _killingSocket(0x0)
{
    setObjectName("ConnectionWorker");

#ifdef __USE_CONNECTION_THREAD__
    Qt::ConnectionType connectionType = Qt::QueuedConnection;
#else
    Qt::ConnectionType connectionType = Qt::DirectConnection;
#endif

    connect(&_timeout, &QTimer::timeout, this, &ConnectionWorker::onSocketTimeout, Qt::DirectConnection);

    connect(this,    &ConnectionWorker::connectToServer,      this, &ConnectionWorker::onConnectToServer,      connectionType);
    connect(this,    &ConnectionWorker::killSocket,           this, &ConnectionWorker::onKillSocket,           connectionType);
    connect(this,    &ConnectionWorker::getLibrary,           this, &ConnectionWorker::onGetLibrary,           connectionType);

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
    connect(_remote, &ClementineRemote::sendSongsToDownload,  this, &ConnectionWorker::onSendSongsToDownload,  connectionType);
    connect(_remote, &ClementineRemote::insertUrls,           this, &ConnectionWorker::onInsertUrls,           connectionType);
}

ConnectionWorker::~ConnectionWorker()
{
    onKillSocket();
}

void ConnectionWorker::onKillSocket(){
    _killingSocket = 0x1;
    if (_socket)
    {
        qDebug() << "[MB_TRACE][ConnectionWorker::onKillSocket]";
        disconnect(_socket, &QAbstractSocket::disconnected, this, &ConnectionWorker::onDisconnected);
        disconnect(_socket, &QIODevice::readyRead,          this, &ConnectionWorker::onReadyRead);
        disconnect(_socket, SIGNAL(error(QAbstractSocket::SocketError)),
                   this, SLOT(onError(QAbstractSocket::SocketError)));
        disconnect(_socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)),
                   this, SLOT(onError(QAbstractSocket::SocketError)));
        _socket->disconnectFromHost();
        if (_socket->state() != QAbstractSocket::UnconnectedState)
            _socket->waitForDisconnected();
        _socket->deleteLater();
        _socket = nullptr;
    }
    if (_timeout.isActive())
        _timeout.stop();
    _killingSocket = 0x0;
}

void ConnectionWorker::onConnectToServer(ClementineSession *session)
{
    _session = session;
    _socket = new QTcpSocket();

    _socket->setSocketOption(QAbstractSocket::KeepAliveOption, true);
    _socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    connect(_socket, &QAbstractSocket::connected,    this, &ConnectionWorker::onConnected,    Qt::DirectConnection);
    connect(_socket, &QAbstractSocket::disconnected, this, &ConnectionWorker::onDisconnected, Qt::DirectConnection);
    connect(_socket, &QIODevice::readyRead,          this, &ConnectionWorker::onReadyRead,    Qt::DirectConnection);

    qRegisterMetaType<QAbstractSocket::SocketError>("SocketError" );
    connect(_socket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(onError(QAbstractSocket::SocketError)), Qt::QueuedConnection);
    connect(_socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)),
            this, SLOT(onError(QAbstractSocket::SocketError)), Qt::QueuedConnection);

    _socket->connectToHost(session->host(), session->port());
    _timeout.start(_remote->sockTimeoutMs());
}

void ConnectionWorker::onDisconnectFromServer()
{
    _socket->disconnectFromHost();
}

void ConnectionWorker::onNextSong()
{
    qDebug() << "[ConnectionWorker::onNextSong]";
    int nextIndex = 1 + static_cast<int>(_remote->activeSongIndex());
    if (_remote->isActivePlaylistDisplayed())
        onChangeToSong(nextIndex);
    else
        _remote->changeAndPlaySong(nextIndex, _remote->activePlaylistID());
}

void ConnectionWorker::onPreviousSong()
{
    qDebug() << "[ConnectionWorker::onPreviousSong]";
    int previousIndex = static_cast<int>(_remote->activeSongIndex()) - 1;
    if (previousIndex >= 0 )
    {
        if (_remote->isActivePlaylistDisplayed())
            onChangeToSong(previousIndex);
        else
            _remote->changeAndPlaySong(previousIndex, _remote->activePlaylistID());
    }
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

    if (msg.type() == pb::remote::PLAY)
        _remote->shallForceRePlayActiveSong();

    sendDataToServer(msg);
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

    RemotePlaylist *playlist = _remote->playlist(pIdx);
    if (!playlist)
    {
        qCritical() << "[ConnectionWorker::onChangePlaylist] Can't find playlist with index: " << pIdx;
        return ;
    }

    pb::remote::Message msg;
    msg.set_type(pb::remote::REQUEST_PLAYLIST_SONGS);
    msg.mutable_request_playlist_songs()->set_id(playlist->id);
    _remote->setRequestSongsForPlaylistID(playlist->id);

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
    req->set_playlist_id(_remote->displayedPlaylistID());
    req->set_play_now(true);
    *req->add_urls() = _remote->radioStream(radioIdx).url.toStdString();

    sendDataToServer(msg);
}

void ConnectionWorker::onSendSongsToRemove()
{
    _remote->doSendSongsToRemove();
}

void ConnectionWorker::onSendSongsToDownload(const QString &dstFolder)
{
    if (dstFolder.isEmpty())
        _songsDL.downloadPath = _remote->downloadPath();
    else if (!createDownloadDestinationFolder(dstFolder)){
        _remote->releaseUserMutex();
        return;
    }

    emit _remote->downloadProgress(0); // let's make the progress bar visible ;)
    _remote->doSendSongsToDownload();
}

void ConnectionWorker::onDownloadCurrentSong()
{
    _songsDL.downloadPath = _remote->downloadPath();
    pb::remote::Message msg;
    msg.set_type(pb::remote::DOWNLOAD_SONGS);

    pb::remote::RequestDownloadSongs *req = msg.mutable_request_download_songs();
    req->set_download_item(pb::remote::DownloadItem::CurrentItem);
    sendDataToServer(msg);
}

bool ConnectionWorker::createDownloadDestinationFolder(const QString &dstFolder)
{
    QString downloadPath = QString("%1/%2").arg(_remote->downloadPath()).arg(dstFolder);
    QFileInfo fi(downloadPath);
    if (fi.exists())
    {
        if (!fi.isDir())
        {
            emit _remote->downloadComplete(0, 0, {
                                           tr("%1 already exist but is not a directory...").arg(
                                           downloadPath)
                                       });
            return false;
        }
    }
    else
    {
        QDir dir(_remote->downloadPath());
        if (!dir.mkpath(dstFolder))
        {
            emit _remote->downloadComplete(0, 0, {
                                               tr("Error creating download folder %1").arg(
                                               downloadPath)
                                           });
            return false;
        }
    }

    _songsDL.downloadPath = downloadPath;
    return true;
}

void ConnectionWorker::onDownloadPlaylist(qint32 playlistID, QString playlistName)
{
    QString dstFolder = QString("playlists/%1").arg(playlistName);
    if (!createDownloadDestinationFolder(dstFolder))
        return;

    emit _remote->downloadProgress(0); // let's make the progress bar visible ;)

    pb::remote::Message msg;
    msg.set_type(pb::remote::DOWNLOAD_SONGS);

    pb::remote::RequestDownloadSongs *req = msg.mutable_request_download_songs();
    req->set_download_item(pb::remote::DownloadItem::APlaylist);
    req->set_playlist_id(playlistID);
    sendDataToServer(msg);
}

void ConnectionWorker::onGetLibrary()
{
    pb::remote::Message msg;
    msg.set_type(pb::remote::GET_LIBRARY);
    sendDataToServer(msg);
}

void ConnectionWorker::onInsertUrls(qint32 playlistID, const QString &newPlaylistName)
{
    _remote->doSendInsertUrls(playlistID, newPlaylistName);
}

void ConnectionWorker::sendChangeSong(int songIndex, qint32 playlistID)
{
    pb::remote::Message msg;
    msg.set_type(pb::remote::CHANGE_SONG);
    pb::remote::RequestChangeSong *request = msg.mutable_request_change_song();
    if (playlistID != -1)
        request->set_playlist_id(playlistID);

    qDebug() << "[ConnectionWorker::onChangeToSong] playlistID: " << playlistID << ", songIndex: " << songIndex;

    request->set_song_index(songIndex);

    sendDataToServer(msg);
}

void ConnectionWorker::onChangeToSong(int proxyRow)
{
    int modelRow = _remote->modelRowFromProxyRow(proxyRow);
    if (modelRow != -1)
        _remote->changeAndPlaySong(modelRow, _remote->displayedPlaylistID());
    else
        qCritical() << "[ConnectionWorker::onChangeToSong] ERROR getting model row from proxy one: " << proxyRow;
}

void ConnectionWorker::onSetTrackPostion(qint32 newPos)
{
    pb::remote::Message msg;
    msg.set_type(pb::remote::SET_TRACK_POSITION);
    msg.mutable_request_set_track_position()->set_position(newPos);

    qDebug() << "[ConnectionWorker::onSetTrackPostion] new Pos: " << newPos
             << " ("<< _remote->prettyLength(newPos) << " for track: " << _remote->activeSong().title;

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

    qDebug() << "[ConnectionWorker::onConnected] to " << _session->str();
    // Create the general message and set the message type
    pb::remote::Message msg;
    msg.set_type(pb::remote::CONNECT);

    pb::remote::RequestConnect *reqConnect = msg.mutable_request_connect();
    if (_session->pass() != -1)
        reqConnect->set_auth_code(_session->pass());

    sendDataToServer(msg);

//    emit _remote->connected();
}

void ConnectionWorker::onDisconnected()
{
    qDebug() << "[ConnectionWorker::onDisconnected] from " << _session->str();
    if (_timeout.isActive())
        _timeout.stop();

    if (!_killingSocket.loadRelaxed() && _socket)
    {
        disconnect(_socket, SIGNAL(error(QAbstractSocket::SocketError)),
                   this, SLOT(onError(QAbstractSocket::SocketError)));
        disconnect(_socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)),
                   this, SLOT(onError(QAbstractSocket::SocketError)));
        disconnect(_socket, &QIODevice::readyRead, this, &ConnectionWorker::onReadyRead);
        _socket->deleteLater();
        _socket = nullptr;
    }

    _songsDL.init(0, 0);
    _libraryDL.init();

    _session = nullptr;
    _remote->clearData(_disconnectReason);
}

void ConnectionWorker::onReadyRead()
{
    if (_killingSocket.loadRelaxed() || !_socket)
    {
        qDebug() << "[ConnectionWorker::onReadyRead] ignoring read...";
        return;
    }

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
    qDebug() << "[ConnectionWorker::onSocketTimeout] on " << _session->str();
    _timeout.stop();
    emit _remote->connectionError(tr("Unable to connect..."));
    if (_socket)
        onDisconnected();
}

void ConnectionWorker::onError(QAbstractSocket::SocketError err)
{
    if (!_killingSocket.loadRelaxed() && _socket)
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
    _songsDL.init(downloadSize.file_count(), downloadSize.total_size());
    qDebug() << "[ConnectionWorker::prepareDownload] nbFiles: " << _songsDL.nbFiles
             << ", total size: " << _songsDL.totalSize;

    if (_songsDL.nbFiles) // let's make the progress bar visible ;)
        emit _remote->downloadProgress(0);
}

void ConnectionWorker::downloadSong(const pb::remote::ResponseSongFileChunk &songChunk)
{
    bool cancelled = _songsDL.isCancelled();

    _songsDL.chunkNumber = songChunk.chunk_number();
    _songsDL.chunkCount  = songChunk.chunk_count();
    _songsDL.fileNumber  = songChunk.file_number();
    _songsDL.fileSize    = songChunk.size();

//    qDebug() << "rcv ResponseSongFileChunk File: " << _songsDL.fileNumber
//             << " / " << _songsDL.nbFiles << " size: " << _songsDL.fileSize
//             << " - Chunk " << _songsDL.chunkNumber << " / " << _songsDL.chunkCount;
//if (_songsDL.fileNumber == 2 && _songsDL.chunkNumber == 2)
//    _songsDL.cancelDownload();

    // Song offer is chunk no 0
    if (_songsDL.chunkNumber == 0)
    {
        bool acceptFile = true;
        if (songChunk.has_song_metadata() && songChunk.size() != 0)
        {
            _songsDL.song = songChunk.song_metadata();
            if (cancelled)
            {
                qDebug() << "Cancelling: " << _songsDL.song.filename;
                if (!_songsDL.hasCancelError)
                {
                    _songsDL.addError(tr("Download cancelled from %1").arg(_songsDL.song.filename));
                    _songsDL.hasCancelError = true;
                }
            }
            else
            {
                _songsDL.file = new QFile(QString("%1/%2").arg(_songsDL.downloadPath).arg(_songsDL.song.filename));
                if (_songsDL.file->exists() && !_remote->overwriteDownloadedSongs())
                {
                    _songsDL.addError(tr("skipping file %1").arg(_songsDL.song.filename));
                    acceptFile = false;
                }
                else
                {
                    _songsDL.canWrite = _songsDL.file->open(QIODevice::ReadWrite);
                    qDebug() << "rcv ResponseSongFileChunk has SongMeta: "
                             << _songsDL.song.str()
                             << ", canWrite: " << _songsDL.canWrite;
                    if (!_songsDL.canWrite)
                    {
                        _songsDL.addError(tr("can't write file %1").arg(_songsDL.song.filename));
                        acceptFile = false;
                    }
                }
            }
        }

        if (!acceptFile) // To update progress bar
            _songsDL.dowloadedSize += _songsDL.fileSize;

        pb::remote::Message msg;
        msg.set_type(pb::remote::SONG_OFFER_RESPONSE);
        msg.mutable_response_song_offer()->set_accepted(acceptFile && !cancelled);
        sendDataToServer(msg);
    }
    else if (_songsDL.canWrite)
    {
        if (cancelled){
            if (_songsDL.file)
            {
                qDebug() << "Deleting: " << _songsDL.song.filename;
                _songsDL.addError(tr("Download cancelled from %1").arg(_songsDL.song.filename));
                _songsDL.file->remove();
                delete _songsDL.file;
                _songsDL.file = nullptr;
                _songsDL.hasCancelError = true;
            }
//            if (_songsDL.chunkNumber == _songsDL.chunkCount)
//                emit _remote->downloadComplete(_songsDL.downloadedFiles,
//                                               _songsDL.nbFiles,
//                                               _songsDL.errorByFileNum.values());
            return;
        }

        const std::string &data = songChunk.data();
        qint64 bytesWritten = 0, size = static_cast<qint64>(data.size());
        int iterMax = 10, iter = 0;
        do {
            qint64 bytes = _songsDL.file->write(data.c_str(), size);
            if (bytes == -1)
            {
                _songsDL.addError(tr("error writing file %1 (%2)").arg(
                                         _songsDL.song.filename).arg(_songsDL.file->errorString()));

                qCritical() << tr("error writing file %1 (%2)").arg(
                                  _songsDL.song.filename).arg(_songsDL.file->errorString());
                break;
            }
            else
                bytesWritten += bytes;

            if (++iter == iterMax)
            {
                _songsDL.addError(tr("error writing file %1 (iterMax reached %2)").arg(
                                         _songsDL.song.filename).arg(iterMax));

                qCritical() << "Error writing: "  << _songsDL.song.filename
                            << " : iterMax reached " << iterMax;
                break;
            }
        } while (bytesWritten != size);

        _songsDL.dowloadedSize += bytesWritten;

        if (_songsDL.chunkNumber == _songsDL.chunkCount) {
            bool sha1Ok = sha1Hex(*_songsDL.file) == songChunk.file_hash().c_str();
            qDebug() << "Dowloaded: "  << _songsDL.song.str()
                     << " sha1Ok : " << sha1Ok << " (fileNumber: " << _songsDL.fileNumber;
            if (!sha1Ok)
            {
                _songsDL.addError(tr("error file %1 (wrong sha1)").arg(
                                         _songsDL.song.filename));
                _songsDL.file->remove();
            }
            else
            {
                ++_songsDL.downloadedFiles;
                _songsDL.file->close();
            }

            delete _songsDL.file;
            _songsDL.file = nullptr;
        }
    }

    emit _remote->downloadProgress(
                static_cast<double>(_songsDL.dowloadedSize) / _songsDL.totalSize);
//    if (_songsDL.fileNumber == _songsDL.nbFiles)
//        emit _remote->downloadComplete(_songsDL.downloadedFiles,
//                                       _songsDL.nbFiles,
//                                       _songsDL.errorByFileNum.values());
}

void ConnectionWorker::downloadFinished()
{
    emit _remote->downloadComplete(_songsDL.downloadedFiles,
                                   _songsDL.nbFiles,
                                   _songsDL.errorByFileNum.values());
}

void ConnectionWorker::downloadLibrary(const pb::remote::ResponseLibraryChunk &libChunk)
{

    if (libChunk.chunk_number() == 1)
    {
        _libraryDL.init();
        _libraryDL.downloadPath = _remote->libraryPath();
        _libraryDL.file = new QFile(QString("%1/%2.db").arg(_libraryDL.downloadPath).arg(_session->name()));
        if (_libraryDL.file->exists())
        {
            qDebug() << "overwriting existing Library " << _libraryDL.file->fileName();
            _libraryDL.file->remove();
        }
        _libraryDL.canWrite = _libraryDL.file->open(QIODevice::ReadWrite);
        if (!_libraryDL.canWrite)
        {
            qCritical() << "Can't write library file: " << _libraryDL.file->fileName();
            // TODO: should send a signal to the GUI
            return;
        }
    }
    else if (!_libraryDL.canWrite)
        return; // ignore all the other chunks

    _libraryDL.chunkNumber = libChunk.chunk_number();
    _libraryDL.chunkCount  = libChunk.chunk_count();
    _libraryDL.fileSize    = libChunk.size();

    const std::string &data = libChunk.data();
    qint64 bytesWritten = 0, size = static_cast<qint64>(data.size());
    int iterMax = 10, iter = 0;
    do {
        qint64 bytes = _libraryDL.file->write(data.c_str(), size);
        if (bytes == -1)
        {
            qCritical() << tr("error writing library file: (%1)").arg(_libraryDL.file->errorString());
            break;
        }
        else
            bytesWritten += bytes;

        if (++iter == iterMax)
        {
            qCritical() << "Error writing library : iterMax reached " << iterMax;
            break;
        }
    } while (bytesWritten != size);

    _libraryDL.dowloadedSize += bytesWritten;

    emit _remote->downloadProgress(
                static_cast<double>(_libraryDL.dowloadedSize) / _libraryDL.fileSize);

    if (_libraryDL.chunkNumber == _libraryDL.chunkCount) {
        bool sha1Ok = sha1Hex(*_libraryDL.file) == libChunk.file_hash().c_str();
        qDebug() << "Library Dowloaded, sha1Ok : " << sha1Ok;
        if (!sha1Ok)
            _libraryDL.file->remove();
        else
            _libraryDL.file->close();

        delete _libraryDL.file;
        _libraryDL.file = nullptr;

        emit _remote->libraryDownloaded();
    }
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

