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
#include "PlayListModel.h"
#include "player/RemotePlaylist.h"

#include <QTcpSocket>
#include <QDataStream>

const QString ClementineRemote::sAppName = "ClementineRemote";
const QString ClementineRemote::sVersion = "0.2";
const QString ClementineRemote::sAppTitle = tr("Clementine Remote");
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
    _shuffleMode(pb::remote::Shuffle_Off), _repeatMode(pb::remote::Repeat_Off),
    _playlists(),
#ifdef __USE_CONNECTION_THREAD__
    _securePlaylists(),
#endif
    _dispPlaylist(nullptr), _dispPlaylistId(0), _dispPlaylistIndex(0),
    _songs(), _activeSong(), _activeSongIndex(0),
#ifdef __USE_CONNECTION_THREAD__
    _secureSongs(), _songsData(),
#endif
    _activePlaylistId(1), _trackPostition(0),
    _initialized(false),
    _playlistModel(new PlayListModel),
    _playlistProxyModel(new PlayListProxyModel),

    _clemFilesSupport(false),
    _remoteFilesPath(_settings.value("remotePath", "./").toString()),
    _remoteFiles(),
#ifdef __USE_CONNECTION_THREAD__
    _secureRemoteFilesData(), _remoteFilesData(),
#endif
    _secureFilesToAppend(), _filesToAppend()
{
    setObjectName("ClementineRemote");
    _playlistModel->setRemote(this);
    _playlistProxyModel->setSourceModel(_playlistModel);
#ifdef __USE_CONNECTION_THREAD__
    connect(this, &ClementineRemote::songsUpdatedByWorker,
            this, &ClementineRemote::onSongsUpdatedByWorker, Qt::QueuedConnection);
    connect(this, &ClementineRemote::remoteFilesUpdatedByWorker,
            this, &ClementineRemote::onRemoteFilesUpdatedByWorker, Qt::QueuedConnection);
    _connection->moveToThread(&_thread);
    _thread.start();
    _thread.setObjectName("ConnectionWorkerThread");
#endif

    _filesToAppend.set_type(pb::remote::APPEND_FILES);
}

ClementineRemote::~ClementineRemote()
{
    qDebug() << "[MB_TRACE] destruction ClementineRemote";
    close();
}

void ClementineRemote::close()
{
    qDebug() << "[MB_TRACE] close ClementineRemote";
    _settings.setValue("remotePath", _remoteFilesPath);
    _settings.sync();
#ifdef __USE_CONNECTION_THREAD__
    emit _connection->killSocket();
    _thread.quit();
    _thread.wait();
#endif

    qDeleteAll(_playlists);
    _playlists.clear();
    if (_playlistProxyModel)
    {
        delete _playlistProxyModel;
        _playlistProxyModel = nullptr;
    }
    if (_playlistModel)
    {
        delete _playlistModel;
        _playlistModel = nullptr;
    }
    if (_connection)
    {
        delete _connection;
        _connection = nullptr;
    }
}

int ClementineRemote::sendSelectedFiles(const QString &newPlaylistName)
{
    _secureFilesToAppend.lock();
    _filesToAppend.clear_response_list_files();
    QStringList selectedFiles;
    for (const RemoteFile &f : _remoteFiles)
    {
        if (!f.isDir && f.selected)
            selectedFiles << f.filename;
    }
    if (selectedFiles.isEmpty())
    {
        _secureFilesToAppend.unlock();
        return 0;
    }
    else
    {
        pb::remote::RequestAppendFiles *files = _filesToAppend.mutable_request_append_files();
        files->set_relative_path(_remoteFilesPath.toStdString());
        if (!newPlaylistName.isEmpty())
            files->set_new_playlist_name(newPlaylistName.toStdString());
        for (const QString &filename : selectedFiles)
            *files->add_files() = filename.toStdString();

        emit sendFilesToAppend();
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
    _secureFilesToAppend.unlock();
}

void ClementineRemote::setSongsFilter(const QString &searchTxt)
{
    qDebug() << "[MB_TRACE][ClementineRemote::setSongsFilter] searchTxt: " << searchTxt;
    Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive;
    QRegExp regExp(searchTxt, caseSensitivity);
    _playlistProxyModel->setFilterRegExp(regExp);
}

QStringList ClementineRemote::playlistsList()
{
#ifdef __USE_CONNECTION_THREAD__
    QMutexLocker lock(&_securePlaylists);
#endif
    QStringList plist;
    for (RemotePlaylist *p : _playlists)
        plist << p->name;

    if (plist.isEmpty())
        plist << tr("no playlist");
    return plist;
}

void ClementineRemote::updateCurrentSongIdx(qint32 currentSongIndex)
{
    int idx = 0;
    for (const RemoteSong &s : _songs)
    {
        if (s.index == currentSongIndex)
        {
            _activeSongIndex = idx;
            emit currentSongIdx(_playlistProxyModel->mapFromSource(_playlistModel->index(idx)).row());
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


void ClementineRemote::rcvAllActivePlaylists(const pb::remote::ResponsePlaylists &playlists)
{
#ifdef __USE_CONNECTION_THREAD__
    QMutexLocker lock(&_securePlaylists);
#endif
    qDeleteAll(_playlists);
    _playlists.clear();
    _dispPlaylist = nullptr;
    _dispPlaylistId = 0;

    for (const auto& pb_playlist : playlists.playlist())
        _playlists << new RemotePlaylist(pb_playlist);

    emit updatePlaylists();
    qDebug() << "[MsgType::PLAYLISTS] Nb Playlists: " << _playlists.size();
    dumpPlaylists();
}


void ClementineRemote::updateCurrentPlaylist()
{
    _dispPlaylist       = nullptr;
    _dispPlaylistIndex = 0;
    int idx = 0;
    for (RemotePlaylist *p : _playlists)
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

void ClementineRemote::dumpPlaylists()
{
    for (RemotePlaylist *p : _playlists)
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

//        // HACK to resolve ListView issue (displaying empty Rows after the number of elements)
//        int nbFiles = _remoteFiles.size();
//        Q_UNUSED(nbFiles)

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
    qDebug() << "[MsgType::REQUEST_SAVED_RADIOS]";
    for (const auto &radio: radios.streams())
    {
        qDebug() << "Radio: " << radio.name().c_str()
                 << " : " << radio.url().c_str()
                 << ", logo: " << radio.url_logo().c_str();
    }
}

#ifdef __USE_CONNECTION_THREAD__
void ClementineRemote::onSongsUpdatedByWorker()
{
    rcvPlaylistSongs(_songsData.response_playlist_songs());
    _songsData.clear_response_playlist_songs();
    _secureSongs.unlock();
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
    if (!_initialized)
        _activePlaylistId = _dispPlaylistId;
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

        checkClementineVersion();
        qDebug() << "[MsgType::INFO] version: " << _clemVersion
                 << ", state: " << _clemState
                 << ", support files: " << _clemFilesSupport
                 << ", music extensions: " << _musicExtensions;
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
        rcvAllActivePlaylists(msg.response_playlists());
        break;

    case pb::remote::PLAYLIST_SONGS:
#ifdef __USE_CONNECTION_THREAD__
        _secureSongs.lock();
        _songsData = std::move(msg);
        emit songsUpdatedByWorker();
#else
        rcvPlaylistSongs(msg.response_playlist_songs());
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
        _activePlaylistId  = _dispPlaylistId;
        updateCurrentPlaylist();
        break;

    case pb::remote::LIST_FILES:
#ifdef __USE_CONNECTION_THREAD__
        _secureRemoteFilesData.lock();
        _remoteFilesData = std::move(msg);
        emit remoteFilesUpdatedByWorker();
#else
        rcvListOfRemoteFiles(msg.response_files());
#endif
        break;

    case pb::remote::REQUEST_SAVED_RADIOS:
        rcvSavedRadios(msg.response_saved_radios());
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
