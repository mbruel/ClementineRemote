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

#include "ClementineRemote.h"
#include "ConnectionWorker.h"
#include "model/RemoteSongModel.h"
#include "model/PlaylistModel.h"
#include "player/RemotePlaylist.h"

#include <QTcpSocket>
#include <QDataStream>
#include <QStandardPaths>
#include <QDir>
const QString ClementineRemote::sAppName    = "ClemRemote";
const QString ClementineRemote::sVersion    = "0.2";
const QString ClementineRemote::sAppTitle   = tr("Clementine Remote");
const QString ClementineRemote::sProjectUrl = "https://github.com/mbruel/ClementineRemote";
const QString ClementineRemote::sBTCaddress = "3BGbnvnnBCCqrGuq1ytRqUMciAyMXjXAv6";
const QString ClementineRemote::sDonateUrl  = "https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=W2C236U6JNTUA&item_name=ClementineRemote&currency_code=EUR";

const QPair<ushort, ushort> ClementineRemote::sClemFilesSupportMinVersion = {1, 4};

const QMap<pb::remote::RepeatMode, ushort> ClementineRemote::sQmlRepeatCodes = {
    {pb::remote::RepeatMode::Repeat_Off,      0},
    {pb::remote::RepeatMode::Repeat_Track,    1},
    {pb::remote::RepeatMode::Repeat_Album,    2},
    {pb::remote::RepeatMode::Repeat_Playlist, 3},
};
const QMap<pb::remote::ShuffleMode, ushort> ClementineRemote::sQmlShuffleCodes = {
    {pb::remote::ShuffleMode::Shuffle_Off,         0},
    {pb::remote::ShuffleMode::Shuffle_All,         1},
    {pb::remote::ShuffleMode::Shuffle_InsideAlbum, 2},
    {pb::remote::ShuffleMode::Shuffle_Albums,      3}
};

ClementineRemote::ClementineRemote(QObject *parent):
    QObject(parent), Singleton<ClementineRemote>(),
#ifdef __USE_CONNECTION_THREAD__
    _thread(),
#endif
    _connection(new ConnectionWorker(this)),
    #if defined( Q_OS_WIN )
    _settings("clemRemote.ini", QSettings::Format::IniFormat),
    #else
    _settings(QSettings::NativeFormat, QSettings::UserScope, sAppName),
    #endif
    _clemVersion(), _clemState(pb::remote::Idle), _previousClemState(pb::remote::Idle),
    _musicExtensions(), _volume(0),
    _downloadsAllowed(true), _downloadPath(),
    _shuffleMode(pb::remote::Shuffle_Off), _repeatMode(pb::remote::Repeat_Off),
    _playlistsOpened(), _playlistsClosed(),
#ifdef __USE_CONNECTION_THREAD__
    _securePlaylists(), _playlistData(),
#endif
    _dispPlaylist(nullptr), _dispPlaylistId(0), _dispPlaylistIndex(0),
    _plOpenedModel(new PlaylistModel(this, false)), _plClosedModel(new PlaylistModel(this, true)),
    _songs(), _activeSong(), _activeSongIndex(0),
#ifdef __USE_CONNECTION_THREAD__
    _secureSongs(), _songsData(),
#endif
    _songsModel(new RemoteSongModel),
    _songsProxyModel(new RemoteSongProxyModel),
    _songsToRemove(), _songsToDownload(),
    _activePlaylistId(1), _trackPostition(0),
    _initialized(false),
    _clemFilesSupport(false),
    _remoteFilesPath(_settings.value("remotePath", "./").toString()),
    _remoteFiles(),
#ifdef __USE_CONNECTION_THREAD__
    _secureRemoteFilesData(), _remoteFilesData(),
    _secureFilesToAppend(),
#endif
    _filesToAppend(),
    _radioStreams(),
#ifdef __USE_CONNECTION_THREAD__
    _secureRadioStreams(), _radioStreamsData(),
#endif
    _isDownloading(0x0)
{
    setObjectName("ClemRemote");
    _songsModel->setRemote(this);
    _songsProxyModel->setSourceModel(_songsModel);
#ifdef __USE_CONNECTION_THREAD__
    connect(this, &ClementineRemote::playlistsOpenedUpdatedByWorker,
            this, &ClementineRemote::onPlaylistsOpenedUpdatedByWorker, Qt::QueuedConnection);
    connect(this, &ClementineRemote::songsUpdatedByWorker,
            this, &ClementineRemote::onSongsUpdatedByWorker, Qt::QueuedConnection);
    connect(this, &ClementineRemote::remoteFilesUpdatedByWorker,
            this, &ClementineRemote::onRemoteFilesUpdatedByWorker, Qt::QueuedConnection);
    _connection->moveToThread(&_thread);
    _thread.start();
    _thread.setObjectName("ConnectionWorkerThread");
#endif

    _filesToAppend.set_type(pb::remote::APPEND_FILES);
    _songsToRemove.set_type(pb::remote::REMOVE_SONGS);
    _songsToDownload.set_type(pb::remote::DOWNLOAD_SONGS);
}

ClementineRemote::~ClementineRemote()
{
    qDebug() << "[MB_TRACE] destruction ClementineRemote";
    close();
}

void ClementineRemote::cancelDownload() const
{
    _connection->cancelDownload();
}

const QString ClementineRemote::hostname() const
{
    if (_connection)
        return _connection->hostname();
    else
        return "nowhere..."; // should never happen
}

void ClementineRemote::close()
{
    qDebug() << "[MB_TRACE] close ClementineRemote";
    _settings.setValue("remotePath", _remoteFilesPath);
    _settings.setValue("downloadPath", _downloadPath);
    _settings.sync();
#ifdef __USE_CONNECTION_THREAD__
    emit _connection->killSocket();
    _thread.quit();
    _thread.wait();
#endif

    qDeleteAll(_playlistsOpened);
    _playlistsOpened.clear();
    if (_plOpenedModel)
    {
        delete _plOpenedModel;
        _plOpenedModel = nullptr;
    }
    qDeleteAll(_playlistsClosed);
    _playlistsClosed.clear();
    if (_plClosedModel)
    {
        delete _plClosedModel;
        _plClosedModel = nullptr;
    }
    if (_songsProxyModel)
    {
        delete _songsProxyModel;
        _songsProxyModel = nullptr;
    }
    if (_songsModel)
    {
        delete _songsModel;
        _songsModel = nullptr;
    }
    if (_connection)
    {
        delete _connection;
        _connection = nullptr;
    }
}

int ClementineRemote::sendSelectedFiles(const QString &newPlaylistName)
{
#ifdef __USE_CONNECTION_THREAD__
    _secureFilesToAppend.lock();
#endif
    _filesToAppend.clear_response_list_files();
    QStringList selectedFiles;
    for (const RemoteFile &f : _remoteFiles)
    {
        if (!f.isDir && f.selected)
            selectedFiles << f.filename;
    }
    if (selectedFiles.isEmpty())
    {
#ifdef __USE_CONNECTION_THREAD__
        _secureFilesToAppend.unlock();
#endif
        return 0;
    }
    else
    {
        pb::remote::RequestAppendFiles *files = _filesToAppend.mutable_request_append_files();
        files->set_relative_path(_remoteFilesPath.toStdString());
        if (!newPlaylistName.isEmpty())
            files->set_new_playlist_name(newPlaylistName.toStdString());
        else
            files->set_playlist_id(_dispPlaylistId);

        for (const QString &filename : selectedFiles)
            *files->add_files() = filename.toStdString();

        emit sendFilesToAppend();
        return selectedFiles.size();
    }
}

int ClementineRemote::downloadSelectedFiles()
{
#ifdef __USE_CONNECTION_THREAD__
    _secureFilesToAppend.lock();
#endif
    _songsToDownload.clear_request_download_songs();
    QStringList selectedFiles;
    for (const RemoteFile &f : _remoteFiles)
    {
        if (!f.isDir && f.selected)
            selectedFiles << f.filename;
    }
    if (selectedFiles.isEmpty())
    {
#ifdef __USE_CONNECTION_THREAD__
        _secureFilesToAppend.unlock();
#endif
        return 0;
    }
    else
    {
        pb::remote::RequestDownloadSongs *files = _songsToDownload.mutable_request_download_songs();
        files->set_download_item(pb::remote::DownloadItem::Urls);
        files->set_relative_path(_remoteFilesPath.toStdString());
        for (const QString &filename : selectedFiles)
            *files->add_urls() = filename.toStdString();

#ifdef __USE_CONNECTION_THREAD__
        _secureFilesToAppend.unlock();
        _secureSongs.lock(); // this one will be unlocked in ClementineRemote::doSendSongsToDownload
#endif
        emit sendSongsToDownload(QFileInfo(_remoteFilesPath).fileName());
        return selectedFiles.size();
    }
}

bool ClementineRemote::allFilesSelected() const
{
    for (const RemoteFile &f : _remoteFiles)
    {
        if (!f.isDir && !f.selected)
            return false;
    }
    return true;
}

void ClementineRemote::doSendFilesToAppend()
{
    _connection->sendDataToServer(_filesToAppend);
    _filesToAppend.clear_request_append_files();
#ifdef __USE_CONNECTION_THREAD__
    _secureFilesToAppend.unlock();
#endif
}

void ClementineRemote::setSongsFilter(const QString &searchTxt)
{
    qDebug() << "[MB_TRACE][ClementineRemote::setSongsFilter] searchTxt: " << searchTxt;
    Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive;
    QRegExp regExp(searchTxt, caseSensitivity);
    _songsProxyModel->setFilterRegExp(regExp);
}

#include <QMutexLocker>
bool ClementineRemote::allSongsSelected()
{
#ifdef __USE_CONNECTION_THREAD__
    QMutexLocker lock(&_secureSongs);
#endif
    return _songsProxyModel->allSongsSelected();
}

void ClementineRemote::selectAllSongsFromProxyModel(bool selectAll)
{
#ifdef __USE_CONNECTION_THREAD__
    QMutexLocker lock(&_secureSongs);
#endif
    _songsProxyModel->selectAllSongs(selectAll);
}

void ClementineRemote::deleteSelectedSongs()
{
#ifdef __USE_CONNECTION_THREAD__
    _secureSongs.lock();
#endif
    _songsToRemove.clear_request_remove_songs();
    QList<int> selectedSongsIdexes = _songsProxyModel->selectedSongsIdexes();
    if (selectedSongsIdexes.isEmpty())
    {
#ifdef __USE_CONNECTION_THREAD__
        _secureSongs.unlock();
#endif
        return ;
    }
    else
    {
        pb::remote::RequestRemoveSongs *req = _songsToRemove.mutable_request_remove_songs();
        req->set_playlist_id(_dispPlaylistId);
        for (int songIndex : selectedSongsIdexes)
            req->add_songs(songIndex);

        emit sendSongsToRemove();
    }
}

void ClementineRemote::downloadSelectedSongs()
{
#ifdef __USE_CONNECTION_THREAD__
    _secureSongs.lock();
#endif
    _songsToDownload.clear_request_download_songs();
    QList<int> selectedSongsIDs = _songsProxyModel->selectedSongsIDs();
    if (selectedSongsIDs.isEmpty())
    {
#ifdef __USE_CONNECTION_THREAD__
        _secureSongs.unlock();
#endif
        return ;
    }
    else
    {
        pb::remote::RequestDownloadSongs *req = _songsToDownload.mutable_request_download_songs();
        req->set_playlist_id(_dispPlaylistId);
        req->set_download_item(pb::remote::DownloadItem::APlaylist);
        for (int songID : selectedSongsIDs)
            req->add_songs_ids(songID);

        emit sendSongsToDownload(QString());//playlistName());
    }
}

void ClementineRemote::doSendSongsToRemove()
{
    _connection->sendDataToServer(_songsToRemove);
    _songsToRemove.clear_request_append_files();
#ifdef __USE_CONNECTION_THREAD__
    _secureSongs.unlock();
#endif
}

void ClementineRemote::doSendSongsToDownload()
{
    _connection->sendDataToServer(_songsToDownload);
    _songsToDownload.clear_request_download_songs();
#ifdef __USE_CONNECTION_THREAD__
    _secureSongs.unlock();
#endif
}

QString ClementineRemote::playlistName() const
{
    return _dispPlaylist ? _dispPlaylist->name : QString("Playlist #%1").arg(_dispPlaylistId);
}

bool ClementineRemote::isCurrentPlaylistSaved() const
{
    if (_dispPlaylist)
        return _dispPlaylist->favorite;
    else
        return false;
}


void ClementineRemote::updateCurrentSongIdx(qint32 currentSongIndex)
{
    int idx = 0;
    for (const RemoteSong &s : _songs)
    {
        if (s.index == currentSongIndex)
        {
            _activeSongIndex = idx;
            emit currentSongIdx(_songsProxyModel->mapFromSource(_songsModel->index(idx)).row());
            break;
        }
        ++idx;
    }
}




void ClementineRemote::dumpCurrentPlaylist()
{
    for (const RemoteSong &s : _songs)
        qDebug() << "  - " << s.str();
}


void ClementineRemote::rcvPlaylists(const pb::remote::ResponsePlaylists &playlists)
{
    bool includeClosedPlaylists = playlists.has_include_closed() && playlists.include_closed();
    if (_playlistsOpened.size())
    {
        emit _plOpenedModel->preClearPlaylists(_playlistsOpened.size() - 1);
        qDeleteAll(_playlistsOpened);
        _playlistsOpened.clear();
        emit _plOpenedModel->postClearPlaylists();
    }
    if (includeClosedPlaylists && _playlistsClosed.size())
    {
        emit _plClosedModel->preClearPlaylists(_playlistsClosed.size() - 1);
        qDeleteAll(_playlistsClosed);
        _playlistsClosed.clear();
        emit _plClosedModel->postClearPlaylists();
    }

    qint32 nbPlaylists = playlists.playlist_size();
    int idxOpened = 0, idxClosed = 0;
    if (nbPlaylists)
    {
        for (const auto& pb_playlist : playlists.playlist())
        {
            if(pb_playlist.closed())
            {
                emit _plClosedModel->preAddPlaylist(idxClosed);
                _playlistsClosed << new RemotePlaylist(pb_playlist);
                emit _plClosedModel->postAddPlaylist(idxClosed++);
            }
            else
            {
                emit _plOpenedModel->preAddPlaylist(idxOpened);
                _playlistsOpened << new RemotePlaylist(pb_playlist);
                emit _plOpenedModel->postAddPlaylist(idxOpened++);
            }
        }
    }

    updateCurrentPlaylist();

    if (includeClosedPlaylists)
        emit closedPlaylistsReceived(idxClosed);

//    emit updatePlaylists();
    qDebug() << "[MsgType::PLAYLISTS] Nb Playlists: " << idxOpened
             << " (closed ones: " << idxClosed << ")";
    dumpPlaylists();
}


void ClementineRemote::updateCurrentPlaylist()
{
    _dispPlaylist       = nullptr;
    _dispPlaylistIndex = 0;
    int idx = 0;
    for (RemotePlaylist *p : _playlistsOpened)
    {
        if (p->id == _dispPlaylistId)
        {
            _dispPlaylist       = p;
            _dispPlaylistIndex = idx;
            qDebug() << "[ClementineRemote::updateCurrentPlaylist] currentPlaylist: #" << idx
                     << " : " << p->name;

            emit updatePlaylist(idx);
            break;
        }
        ++idx;
    }
}

void ClementineRemote::updateActivePlaylist()
{
    _activePlaylistId = _dispPlaylistId;
    int row = 0;
    for (RemotePlaylist *p : _playlistsOpened)
    {
        if (p->id == _activePlaylistId)
        {
            if (!p->playing)
            {
                p->playing = true;
                QModelIndex index = _plOpenedModel->index(row);
                emit _plOpenedModel->dataChanged(index, index, {PlaylistModel::iconSrc});
            }
        }
        else if (p->playing)
        {
            p->playing = false;
            QModelIndex index = _plOpenedModel->index(row);
            emit _plOpenedModel->dataChanged(index, index, {PlaylistModel::iconSrc});
        }
        ++row;
    }
}

void ClementineRemote::dumpPlaylists()
{
    for (RemotePlaylist *p : _playlistsOpened)
        qDebug() << "  - " << p->str();
}


void ClementineRemote::rcvListOfRemoteFiles(const pb::remote::ResponseListFiles &files)
{
    if (files.has_error() && files.error() != pb::remote::ResponseListFiles::NONE)
        qDebug() << "[MsgType::LIST_FILES] ERROR: " << files.error();
    else
    {
        if (_remoteFiles.size())
        {
            emit preClearRemoteFiles(_remoteFiles.size() - 1);
            _remoteFiles.clear();
            emit postClearRemoteFiles();
        }

        qint32 nbRemoteFiles = files.files_size();
        if (nbRemoteFiles)
        {
            emit preAddRemoteFiles(nbRemoteFiles -1 );
            _remoteFiles.reserve(nbRemoteFiles);
            for (const auto& pb_file : files.files())
                _remoteFiles << RemoteFile(pb_file.filename(), pb_file.is_dir());
            emit postAddRemoteFiles();
        }

        qDebug() << "[MsgType::LIST_FILES] Nb Files: " << _remoteFiles.size();


        _remoteFilesPath = files.relative_path().c_str();
        emit updateRemoteFilesPath(_remoteFilesPath);
    }
}

void ClementineRemote::rcvSavedRadios(const pb::remote::ResponseSavedRadios &radios)
{
    if (_radioStreams.size())
    {
        emit preClearRadioStreams(_radioStreams.size() - 1);
        _radioStreams.clear();
        emit postClearRadioStreams();

    }
    qint32 nbStreams = radios.streams_size();
    if (nbStreams)
    {
        emit preAddRadioStreams(nbStreams -1 );
        _radioStreams.reserve(nbStreams);
        for (const auto &radio: radios.streams())
            _radioStreams << Stream(radio.name(), radio.url(), radio.url_logo());
        emit postAddRadioStreams();
    }

    qDebug() << "[MsgType::REQUEST_SAVED_RADIOS] Nb Radio Streams: " << _radioStreams.size();
}

QString ClementineRemote::setDownloadFolder()
{
    _downloadPath = _settings.value("downloadPath", "").toString();
    if (_downloadPath.isEmpty())
    {
#if defined(Q_OS_ANDROID)
        _downloadPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#elif defined(Q_OS_IOS)
        _downloadPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#else
        _downloadPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
#endif
        qDebug() << "Download path: " << _downloadPath;
        QFileInfo fi(_downloadPath);
        if (!fi.isDir())
            return tr("the download folder is not a directory...");
        else if (!fi.isWritable())
            return tr("the download folder is not writable...");

        fi = QFileInfo(QString("%1/ClemRemote").arg(_downloadPath));
        if (!fi.exists())
        {
            QDir dir(_downloadPath);
            if (!dir.mkdir("ClemRemote"))
                return tr("error creating default Download folder: %1").arg(fi.absoluteFilePath());
        }
        _downloadPath = fi.absoluteFilePath();
    }
    qDebug() << "Download Path: " << _downloadPath;
    return QString();
}

#ifdef __USE_CONNECTION_THREAD__
void ClementineRemote::onPlaylistsOpenedUpdatedByWorker()
{
    rcvPlaylists(_playlistData.response_playlists());
    _playlistData.clear_response_playlists();
    _securePlaylists.unlock();
}
void ClementineRemote::onSongsUpdatedByWorker(bool initialized)
{
    rcvPlaylistSongs(_songsData.response_playlist_songs());
    _songsData.clear_response_playlist_songs();
    _secureSongs.unlock();
    if (!initialized)
        updateActivePlaylist();
}
void ClementineRemote::onRemoteFilesUpdatedByWorker()
{
    rcvListOfRemoteFiles(_remoteFilesData.response_list_files());
    _remoteFilesData.clear_response_list_files();
    _secureRemoteFilesData.unlock();
}
#endif

void ClementineRemote::rcvPlaylistSongs(const pb::remote::ResponsePlaylistSongs &songs)
{
    _dispPlaylistId = songs.requested_playlist().id();
    qDebug() << "[MsgType::PLAYLIST_SONGS] playlist ID: " << _dispPlaylistId;
    updateCurrentPlaylist();

    if (_songs.size())
    {
        emit preClearSongs(_songs.size() - 1);
        _songs.clear();
        emit postSongRemoved();
    }


    qint32 nbSongs = songs.songs_size();
    if (nbSongs > 0)
    {
        emit preAddSongs(songs.songs_size() -1 );
        _songs.reserve(songs.songs_size());
        qint32 idx = 0;
        for (const auto& song : songs.songs())
        {
            _songs << song;
            if (song.index() == _activeSong.index)
            {
                qDebug() << "[MsgType::PLAYLIST_SONGS] current song id: " << _activeSongIndex;
                _activeSongIndex = idx;
            }
            ++idx;
        }
        emit postSongAppended();
    }

    qDebug() << "[MsgType::PLAYLIST_SONGS] Nb Songs: " << _songs.size();
//    dumpCurrentPlaylist();
}

void ClementineRemote::checkClementineVersion()
{
    QRegularExpression regExp(sClemVersionRegExpStr);
    QRegularExpressionMatch match = regExp.match(_clemVersion);
    if (match.hasMatch())
    {
        ushort major = match.captured(1).toUShort();
        ushort minor = match.captured(2).toUShort();
        if (major > sClemFilesSupportMinVersion.first
                || (major == sClemFilesSupportMinVersion.first && minor >= sClemFilesSupportMinVersion.second))
            _clemFilesSupport = true;
        else
            _clemFilesSupport = false;
    }
    else
        qCritical() << "Couldn't parse Clementine version...";
}


void ClementineRemote::parseMessage(const QByteArray &data)
{
    pb::remote::Message msg;
    if (!msg.ParseFromArray(data.constData(), data.size())) {
        qCritical() << "Couldn't parse data";
        return;
    }

    pb::remote::MsgType msgType = msg.type();
    switch (msgType) {

    case pb::remote::KEEP_ALIVE:
        qDebug() << "[MsgType::KEEP_ALIVE]";
        break;

    case pb::remote::DISCONNECT:
        _connection->setDisconnectReason(disconnectReason(msg.response_disconnect().reason_disconnect()));
        qDebug() << "[MsgType::DISCONNECT]" << _connection->disconnectReason();
        break;

    case pb::remote::INFO:
        _clemVersion = msg.response_clementine_info().version().c_str();
        _clemState   = msg.response_clementine_info().state();
        _musicExtensions.clear();
        for (const auto& ext : msg.response_clementine_info().files_music_extensions())
            _musicExtensions << QString(ext.c_str());

        if (msg.response_clementine_info().has_allow_downloads())
            _downloadsAllowed = msg.response_clementine_info().allow_downloads();

        checkClementineVersion();
        qDebug() << "[MsgType::INFO] version: " << _clemVersion
                 << ", state: " << _clemState
                 << ", support files: " << _clemFilesSupport
                 << ", music extensions: " << _musicExtensions
                 << ", Downloads Allowed: " << _downloadsAllowed;

        break;

    case pb::remote::MsgType::CURRENT_METAINFO:
        _activeSong = RemoteSong(msg.response_current_metadata().song_metadata());
        updateCurrentSongIdx(_activeSong.index);
        emit currentSongLength(_activeSong.length, _activeSong.pretty_length);
        qDebug() << "[MsgType::CURRENT_METAINFO] " << _activeSong.str();
        break;

    case pb::remote::SET_VOLUME:
        _volume = msg.request_set_volume().volume();
        qDebug() << "[MsgType::SET_VOLUME] " << _volume;
        emit updateVolume(_volume);
        break;

    case pb::remote::UPDATE_TRACK_POSITION:
        _trackPostition = msg.response_update_track_position().position();
        emit currentTrackPosition(_trackPostition);
        qDebug() << "[MsgType::UPDATE_TRACK_POSITION] " << _trackPostition;
        break;

    case pb::remote::PLAYLISTS:
#ifdef __USE_CONNECTION_THREAD__
        _securePlaylists.lock();
        _playlistData = std::move(msg);
        emit playlistsOpenedUpdatedByWorker();
#else
        rcvPlaylists(msg.response_playlists());
#endif
        break;

    case pb::remote::PLAYLIST_SONGS:
#ifdef __USE_CONNECTION_THREAD__
        _secureSongs.lock();
        _songsData = std::move(msg);
        emit songsUpdatedByWorker(_initialized);
#else
        rcvPlaylistSongs(msg.response_playlist_songs());
        if (!_initialized)
            updateActivePlaylist();
#endif
        break;

    case pb::remote::SHUFFLE:
        _shuffleMode = msg.shuffle().shuffle_mode();
        qDebug() << "[MsgType::SHUFFLE] " << _shuffleMode;
        emit updateShuffle(sQmlShuffleCodes.value(_shuffleMode));
        break;

    case pb::remote::REPEAT:
        _repeatMode = msg.repeat().repeat_mode();
        qDebug() << "[MsgType::REPEAT] " << _repeatMode;
        emit updateRepeat(sQmlRepeatCodes.value(_repeatMode));
        break;

    case pb::remote::FIRST_DATA_SENT_COMPLETE:
        _initialized = true;
        qDebug() << "[MsgType::FIRST_DATA_SENT_COMPLETE] fully Initialized \\o/";
        emit connected();
        if (_clemFilesSupport)
            _connection->requestSavedRadios();
        break;

    case pb::remote::PLAY:
        qDebug() << "[MsgType::PLAY]";
        _clemState = pb::remote::EngineState::Playing;
        emit updateEngineState();
        break;
    case pb::remote::PAUSE:
        qDebug() << "[MsgType::PAUSE]";
        _clemState = pb::remote::EngineState::Paused;
        emit updateEngineState();
        break;

    case pb::remote::STOP:
        qDebug() << "[MsgType::STOP]";
        _clemState = pb::remote::EngineState::Idle;
        emit updateEngineState();
        break;

    case pb::remote::ACTIVE_PLAYLIST_CHANGED:
        _dispPlaylistId =  msg.response_active_changed().id();
        qDebug() << "[MsgType::ACTIVE_PLAYLIST_CHANGED] id: " << _dispPlaylistId;
        updateCurrentPlaylist();
        updateActivePlaylist();
        break;

    case pb::remote::LIST_FILES:
#ifdef __USE_CONNECTION_THREAD__
        _secureRemoteFilesData.lock();
        _remoteFilesData = std::move(msg);
        emit remoteFilesUpdatedByWorker();
#else
        rcvListOfRemoteFiles(msg.response_list_files());
#endif
        break;

    case pb::remote::REQUEST_SAVED_RADIOS:
        rcvSavedRadios(msg.response_saved_radios());
        break;

    case pb::remote::DOWNLOAD_TOTAL_SIZE:
        _connection->prepareDownload(msg.response_download_total_size());
        break;
    case pb::remote::DOWNLOAD_QUEUE_EMPTY:
        qDebug() << "[MsgType::DOWNLOAD_QUEUE_EMPTY] nothing left to download!";
        _connection->downloadFinished();
        break;
    case pb::remote::SONG_FILE_CHUNK:
        _connection->downloadSong(msg.response_song_file_chunk());
        break;

    default:
        qDebug() << "Msg type not yet implemented: " << msgType;
        break;
    }
}



qint32 ClementineRemote::currentPlaylistID() const
{
    if (_dispPlaylist)
        return _dispPlaylist->id;
    else
        return -1;
}

qint32 ClementineRemote::activePlaylistID() const
{
    return _activePlaylistId;
}

void ClementineRemote::closingPlaylist(qint32 playlistID)
{
    int idx = 0;
    RemotePlaylist *newDispPlaylist = nullptr;
    for (RemotePlaylist *p : _playlistsOpened)
    {
        if (p->id != playlistID)
        {
            newDispPlaylist = p;
            break;
        }
        ++idx;
    }
    _dispPlaylist      = newDispPlaylist;
    _dispPlaylistIndex = idx;
    if (_activePlaylistId == playlistID)
        _activePlaylistId = newDispPlaylist->id;
}
