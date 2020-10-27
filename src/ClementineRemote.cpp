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
const QString ClementineRemote::sVersion = "0.1";
const QString ClementineRemote::sAppTitle = tr("Clementine Remote");

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
    _thread(),
    _connection(new ConnectionWorker(this)),
    #if defined( Q_OS_WIN )
    _settings("clemRemote.ini", QSettings::Format::IniFormat),
    #else
    _settings(QSettings::NativeFormat, QSettings::UserScope, sAppName),
    #endif
    _clemVersion(), _clemState(pb::remote::Idle), _previousClemState(pb::remote::Idle),
    _volume(0), _shuffleMode(pb::remote::Shuffle_Off), _repeatMode(pb::remote::Repeat_Off),
    _playlists(), _dispPlaylist(nullptr), _dispPlaylistId(0), _dispPlaylistIndex(0),
    _songs(), _activeSong(), _activeSongIndex(0),
    _activePlaylistId(1), _trackPostition(0),
    _initialized(false),
    _playlistModel(new PlayListModel)
#ifdef __USE_PLAYLIST_PROXY_MODEL__
    , _playlistProxyModel(new PlayListProxyModel)
#endif
{
    _playlistModel->setRemote(this);
#ifdef __USE_PLAYLIST_PROXY_MODEL__
    _playlistProxyModel->setSourceModel(_playlistModel);
#endif
    _connection->moveToThread(&_thread);
    _thread.start();
}

ClementineRemote::~ClementineRemote()
{
    _thread.quit();

    qDeleteAll(_playlists);
#ifdef __USE_PLAYLIST_PROXY_MODEL__
    delete _playlistProxyModel;
#endif
    delete _playlistModel;
    delete _connection;
}

void ClementineRemote::setSongsFilter(const QString &searchTxt)
{
    qDebug() << "[MB_TRACE][ClementineRemote::setSongsFilter] searchTxt: " << searchTxt;
#ifdef __USE_PLAYLIST_PROXY_MODEL__
    Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive;
    QRegExp regExp(searchTxt, caseSensitivity);
    _playlistProxyModel->setFilterRegExp(regExp);
#endif
}

QStringList ClementineRemote::playlistsList() const
{
    QStringList plist;
    for (RemotePlaylist *p : _playlists)
        plist << p->name;

    if (plist.isEmpty())
        plist << tr("no playlist");
    return plist;
}

void ClementineRemote::updateCurrentSongIdx(qint32 currentSongID)
{
    int idx = 0;
    for (const RemoteSong &s : _songs)
    {
        if (s.id == currentSongID)
        {
            _activeSongIndex = idx;
#ifdef __USE_PLAYLIST_PROXY_MODEL__
            emit currentSongIdx(_playlistProxyModel->mapFromSource(_playlistModel->index(idx)).row());
#else
            emit currentSongIdx(idx);
#endif
            break;
        }
        ++idx;
    }
}


void ClementineRemote::rcvPlaylistSongs(pb::remote::ResponsePlaylistSongs *songs)
{
    _dispPlaylistId = songs->mutable_requested_playlist()->id();
    qDebug() << "[MsgType::PLAYLIST_SONGS] playlist ID: " << _dispPlaylistId;
    if (!_initialized)
        _activePlaylistId = _dispPlaylistId;
    updateCurrentPlaylist();

    // HACK to resolve ListView issue (displaying empty Rows after the number of elements)
    // cf https://forum.qt.io/topic/120302/listview-with-qsortfilterproxymodel-displays-empty-lines-resulting-to-the-error-unable-to-assign-undefined-to-qstring
    qint32 proxyRows = _playlistProxyModel->rowCount();
    Q_UNUSED(proxyRows)
//    qDebug() << "proxy row count before removal: " << proxyRows;


    emit preClearSongs(_songs.size() - 1);
    _songs.clear();
    emit postSongRemoved();


    emit preAddSongs(songs->songs_size() -1 );
    _songs.reserve(songs->songs_size());
    qint32 idx = 0;
    for (auto it = songs->songs().cbegin(), itEnd = songs->songs().cend();
         it != itEnd; ++it)
    {
        _songs.append(*it);
        if (it->id() == _activeSong.id)
        {
            qDebug() << "[MsgType::PLAYLIST_SONGS] current song id: " << _activeSongIndex;
            _activeSongIndex = idx;
        }
        ++idx;
    }
    emit postSongAppended();


    qDebug() << "[MsgType::PLAYLIST_SONGS] Nb Songs: " << _songs.size();
//    dumpCurrentPlaylist();
}

void ClementineRemote::dumpCurrentPlaylist()
{
    for (const RemoteSong &s : _songs)
        qDebug() << "  - " << s.str();
}


void ClementineRemote::rcvAllActivePlaylists(pb::remote::ResponsePlaylists *playlists)
{
    qDeleteAll(_playlists);
    _playlists.clear();
    _dispPlaylist = nullptr;
    _dispPlaylistId = 0;

    for (auto it = playlists->playlist().cbegin(), itEnd = playlists->playlist().cend(); it != itEnd; ++it)
        _playlists.append(new RemotePlaylist(*it));

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
        _connection->setDisconnectReason(disconnectReason(msg.mutable_response_disconnect()->reason_disconnect()));
        qDebug() << "[MsgType::DISCONNECT]" << _connection->disconnectReason();
        break;

    case pb::remote::INFO:
        _clemVersion = msg.mutable_response_clementine_info()->version().c_str();
        _clemState   = msg.mutable_response_clementine_info()->state();
        qDebug() << "[MsgType::INFO] version: " << _clemVersion << ", state: " << _clemState;
        break;

    case pb::remote::MsgType::CURRENT_METAINFO:
        _activeSong = RemoteSong(*msg.mutable_response_current_metadata()->mutable_song_metadata());
        updateCurrentSongIdx(_activeSong.id);
        emit currentSongLength(_activeSong.length, _activeSong.pretty_length);
        qDebug() << "[MsgType::CURRENT_METAINFO] " << _activeSong.str();
        break;

    case pb::remote::SET_VOLUME:
        _volume = msg.mutable_request_set_volume()->volume();
        qDebug() << "[MsgType::SET_VOLUME] " << _volume;
        emit updateVolume(_volume);
        break;

    case pb::remote::UPDATE_TRACK_POSITION:
        _trackPostition = msg.mutable_response_update_track_position()->position();
        emit currentTrackPosition(_trackPostition);
        qDebug() << "[MsgType::UPDATE_TRACK_POSITION] " << _trackPostition;
        break;

    case pb::remote::PLAYLISTS:
        rcvAllActivePlaylists(msg.mutable_response_playlists());
        break;

    case pb::remote::PLAYLIST_SONGS:
        rcvPlaylistSongs(msg.mutable_response_playlist_songs());
        break;

    case pb::remote::SHUFFLE:
        _shuffleMode = msg.mutable_shuffle()->shuffle_mode();
        qDebug() << "[MsgType::SHUFFLE] " << _shuffleMode;
        emit updateShuffle(sQmlShuffleCodes.value(_shuffleMode));
        break;

    case pb::remote::REPEAT:
        _repeatMode = msg.mutable_repeat()->repeat_mode();
        qDebug() << "[MsgType::REPEAT] " << _repeatMode;
        emit updateRepeat(sQmlRepeatCodes.value(_repeatMode));
        break;

    case pb::remote::FIRST_DATA_SENT_COMPLETE:
        _initialized = true;
        qDebug() << "[MsgType::FIRST_DATA_SENT_COMPLETE] fully Initialized \\o/";
        emit connected();
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
