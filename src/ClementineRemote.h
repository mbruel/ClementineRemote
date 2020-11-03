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

#ifndef CLEMENTINEREMOTE_H
#define CLEMENTINEREMOTE_H

#include "Singleton.h"
#include "PlayListModel.h"
#include "player/RemoteSong.h"
#include "player/RemoteFile.h"
#include <QSettings>
#ifdef __USE_CONNECTION_THREAD__
#include <QThread>
#include <QMutex>
#endif
class ConnectionWorker;
class RemotePlaylist;

class ClementineRemote : public QObject, public Singleton<ClementineRemote>
{
    Q_OBJECT
    friend class Singleton<ClementineRemote>;

    static const QString sVersion;  //!< Version of the Application
    static const QString sAppName;  //!< Name of the Application
    static const QString sAppTitle;
    static const int     sSockTimeoutMs = 2000;

    static const QMap<pb::remote::RepeatMode,  ushort> sQmlRepeatCodes;
    static const QMap<pb::remote::ShuffleMode, ushort> sQmlShuffleCodes;

    static const QPair<ushort, ushort> sClemFilesSupportMinVersion;
    static constexpr const char *sClemVersionRegExpStr = "^Clementine (\\d+)\\.(\\d+).*";

private:
#ifdef __USE_CONNECTION_THREAD__
    QThread                 _thread;            //!< all the network communication is done in a Thread to let the GUI reactive
#endif
    ConnectionWorker       *_connection;        //!< all network communication active object

    QSettings               _settings;          //!< save last server details

    QString                 _clemVersion;       //!< Clementine server version (to make sure last updates are available)
    pb::remote::EngineState _clemState;         //!< Empty, Idle, Playing or Paused
    pb::remote::EngineState _previousClemState; //!< used to restart previous selected song on the Remote
    QStringList             _musicExtensions;   //!< extensions allowed to be seen remotely
    qint32                  _volume;            //!< server volume level

    pb::remote::ShuffleMode _shuffleMode;       //!< server shuffle mode
    pb::remote::RepeatMode  _repeatMode;        //!< server repeat mode

    QList<RemotePlaylist*>  _playlists;         //!< list of all the Playlists (both locally and on server)
    RemotePlaylist         *_dispPlaylist;      //!< Playlist displayed on the Remote
    qint32                  _dispPlaylistId;    //!< ID of the displayed Playlist
    qint32                  _dispPlaylistIndex; //!< index in _playlists of the displayed Playlist

    QList<RemoteSong>       _songs;             //!< list of Song of the Playlist displayed on the Remote
    RemoteSong              _activeSong;        //!< song played (or about to) on the server (pb::remote::CURRENT_METAINFO)
    qint32                  _activeSongIndex;   //!< active song index in _songs
#ifdef __USE_CONNECTION_THREAD__
    QMutex                  _secureSongs;
    pb::remote::Message     _songsData;
#endif

    qint32                  _activePlaylistId;  //!<  ID of the playlist of the active song

    qint32                  _trackPostition;    //!< position in the track of the active song (pb::remote::UPDATE_TRACK_POSITION)


    bool                    _initialized;       //!< did we receive pb::remote::FIRST_DATA_SENT_COMPLETE ?

    PlayListModel          *_playlistModel;     //!< Model used to expose the songs to the View
#ifdef __USE_PLAYLIST_PROXY_MODEL__
    PlayListProxyModel     *_playlistProxyModel;//!< Proxy model used by QML ListView
#endif

    bool                    _clemFilesSupport;
    QString                 _remoteFilesPath;
    QList<RemoteFile>       _remoteFiles;
#ifdef __USE_CONNECTION_THREAD__
    QMutex                  _secureRemoteFilesData;
    pb::remote::Message     _remoteFilesData;
#endif

    QMutex                  _secureFilesToAppend;
    pb::remote::Message     _filesToAppend;

private:
    ClementineRemote(QObject *parent = nullptr);

    inline QString disconnectReason(short reason) const;

    void checkClementineVersion();

public:
    ~ClementineRemote();
#ifdef __USE_PLAYLIST_PROXY_MODEL__
    inline int modelRowFromProxyRow(int proxyRow) const;
#endif

    inline Q_INVOKABLE bool hideServerFilesPreviousNextNavButtons() const;

    Q_INVOKABLE void close();

    Q_INVOKABLE int sendSelectedFiles(const QString &newPlaylistName = "");
    Q_INVOKABLE bool allFilesSelected() const;
    void doSendFilesToAppend();

    inline const QString &remoteFilesPath() const;
    inline Q_INVOKABLE QString remoteFilesPath_QML() const; //!< can't use refs in QML...
    inline Q_INVOKABLE const QString clemVersion() const;
    inline static Q_INVOKABLE QString clementineFilesSupportMinVersion();
    inline Q_INVOKABLE bool clementineFilesSupport() const;

    inline Q_INVOKABLE int activeSongIndex() const;

    inline Q_INVOKABLE QAbstractItemModel *playListModel() const;
    inline int nbSongs() const;
    Q_INVOKABLE void setSongsFilter(const QString &searchTxt);

    inline Q_INVOKABLE QString settingHost() const;
    inline Q_INVOKABLE QString settingPort() const;
    inline Q_INVOKABLE QString settingPass() const;
    inline Q_INVOKABLE void saveSettings(const QString &host, const QString &port, const QString &pass);

    inline Q_INVOKABLE qint32 playerState() const;
    inline qint32 playerPreviousState() const;
    inline Q_INVOKABLE bool isPlaying() const;
    inline Q_INVOKABLE bool isPaused() const;

    inline Q_INVOKABLE qint32 volume() const;
    inline Q_INVOKABLE QString volumePct() const;
    inline Q_INVOKABLE void setCurrentVolume(qint32 vol);

    inline Q_INVOKABLE void playpause();
    inline Q_INVOKABLE void stop();

    inline Q_INVOKABLE ushort repeatMode() const;
    inline Q_INVOKABLE ushort shuffleMode()const;

    Q_INVOKABLE QStringList playlistsList() const;
    inline const QList<RemotePlaylist*> &playlists() const;
    inline RemotePlaylist *playlist(int idx) const;
    inline Q_INVOKABLE int playlistIndex() const;


    inline void setPlay();


    inline const RemoteSong & currentSong() const;
    inline Q_INVOKABLE int currentSongIndex() const;
    void updateCurrentSongIdx(qint32 currentSongIndex);

    inline int numberOfPlaylistSongs() const;
    inline const RemoteSong &playlistSong(int index) const;
    inline RemoteSong &playlistSong(int index);

    inline Q_INVOKABLE const QString currentTrackDuration() const;
    inline Q_INVOKABLE qint32 currentTrackLength() const;


    inline int numberOfRemoteFiles() const;
    inline const RemoteFile &remoteFile(int index) const;
    inline RemoteFile &remoteFile(int index);


    qint32 currentPlaylistID() const;
    qint32 activePlaylistID() const;

    void parseMessage(const QByteArray& data);


private:
    void dumpCurrentPlaylist();


    void rcvAllActivePlaylists(const pb::remote::ResponsePlaylists &playlists);
    void updateCurrentPlaylist();
    void dumpPlaylists();

    void rcvPlaylistSongs(const pb::remote::ResponsePlaylistSongs &songs);
    void rcvListOfRemoteFiles(const pb::remote::ResponseListFiles &files);





signals:
    // signals sent from QML to ConnectionWorker
    void connectToServer(const QString &host, ushort port, int auth_code = -1);
    void disconnectFromServer();
    void nextSong();
    void previousSong();

    void changeToSong(int proxyRow);
    void setTrackPostion(qint32 newPos);
    void setVolume(qint32 vol);

    void setEngineState(qint32 state);
    void shuffle(ushort mode);
    void repeat(ushort mode);

    void getServerFiles(QString currentPath, QString subFolder = "");

    void sendFilesToAppend();


    // signals sent from ConnectionWorker to QML
    void connected();
    void disconnected(QString reason);
    void connectionError(const QString &err);

    void currentSongIdx(qint32 idx);
    void currentSongLength(qint32 length, const QString &pretty_length);
    void currentTrackPosition(qint32 pos);
    void updateVolume(qint32 vol);

    void updateEngineState();
    void updateShuffle(ushort mode);
    void updateRepeat(ushort mode);

    void changePlaylist(qint32 idx);
    void updatePlaylist(int idx);

    void updateRemoteFilesPath(QString newRemotePath);



    // signals for PlayListModel
    void preSongAppended();
    void preAddSongs(int lastSongIdx);
    void postSongAppended();
    void preSongRemoved(int index);
    void preClearSongs(int lastSongIdx);
    void postSongRemoved();

    // signals for RemoteFileModel
    void preAddRemoteFiles(int lastIdx);
    void postAddRemoteFiles();
    void preClearRemoteFiles(int lastIdx);
    void postClearRemoteFiles();

#ifdef __USE_CONNECTION_THREAD__
    void songsUpdatedByWorker();
    void remoteFilesUpdatedByWorker();

private slots:
    void onSongsUpdatedByWorker();
    void onRemoteFilesUpdatedByWorker();
#endif

    //static methods
public:
    inline Q_INVOKABLE static const QString appTitle();
    inline             static const QString &appName();
    inline             static const QString &appVersion();
    inline             static int sockTimeoutMs();

    inline Q_INVOKABLE static QString prettyLength(qint32 sec);

};

QAbstractItemModel *ClementineRemote::playListModel() const
{
#ifdef __USE_PLAYLIST_PROXY_MODEL__
    return _playlistProxyModel;
#else
    return _playlistModel;
#endif
}

int ClementineRemote::nbSongs() const { return _songs.size(); }


qint32 ClementineRemote::playerState() const { return _clemState; }
qint32 ClementineRemote::playerPreviousState() const { return _previousClemState; }

qint32 ClementineRemote::volume() const { return _volume; }
QString ClementineRemote::volumePct() const { return QString("%1%").arg(_volume); }

void ClementineRemote::setCurrentVolume(qint32 vol)
{
    _volume = vol;
    emit setVolume(vol);
}

void ClementineRemote::playpause()
{
    _previousClemState = _clemState;
    if (_clemState == pb::remote::EngineState::Playing)
        _clemState = pb::remote::EngineState::Paused;
    else
        _clemState = pb::remote::EngineState::Playing;

    emit setEngineState(_clemState);
}

void ClementineRemote::stop()
{
    _previousClemState = _clemState;
    _clemState         =  pb::remote::EngineState::Idle;
    emit setEngineState(_clemState);
}

ushort ClementineRemote::repeatMode()  const { return sQmlRepeatCodes.value(_repeatMode); }
ushort ClementineRemote::shuffleMode() const { return sQmlShuffleCodes.value(_shuffleMode); }

const QList<RemotePlaylist *> &ClementineRemote::playlists() const { return _playlists; }
RemotePlaylist *ClementineRemote::playlist(int idx) const { return idx < _playlists.size() ? _playlists.at(idx) : nullptr; }
int ClementineRemote::playlistIndex() const { return _dispPlaylistIndex; }

void ClementineRemote::setPlay()
{
    _clemState = pb::remote::EngineState::Playing;
    emit updateEngineState();
}

const RemoteSong & ClementineRemote::currentSong() const { return _activeSong; }

//uint ClementineRemote::currentSongIndex() const { return _activeSongIndex; }
int ClementineRemote::currentSongIndex() const
{
#ifdef __USE_PLAYLIST_PROXY_MODEL__
    return _playlistProxyModel->mapFromSource(_playlistModel->index(_activeSongIndex)).row();
#else
    return _activeSongIndex;
#endif
}

int ClementineRemote::numberOfPlaylistSongs() const { return _songs.size(); }
const RemoteSong &ClementineRemote::playlistSong(int index) const { return _songs.at(index); }
RemoteSong &ClementineRemote::playlistSong(int index) { return _songs[index]; }

bool ClementineRemote::isPlaying() const { return _clemState == pb::remote::EngineState::Playing; }
bool ClementineRemote::isPaused() const { return _clemState == pb::remote::EngineState::Paused; }

const QString ClementineRemote::currentTrackDuration() const { return _activeSong.pretty_length; }
qint32 ClementineRemote::currentTrackLength() const{ return _activeSong.length; }

int ClementineRemote::numberOfRemoteFiles() const { return _remoteFiles.size(); }
const RemoteFile &ClementineRemote::remoteFile(int index) const { return _remoteFiles.at(index); }
RemoteFile &ClementineRemote::remoteFile(int index) { return _remoteFiles[index]; }

const QString ClementineRemote::appTitle() { return QString("%1 v%2").arg(sAppTitle).arg(sVersion); }
const QString &ClementineRemote::appName() { return sAppName; }
const QString &ClementineRemote::appVersion() { return sVersion; }

int ClementineRemote::sockTimeoutMs() { return sSockTimeoutMs; }

QString ClementineRemote::prettyLength(qint32 sec)
{
    if (sec < 60)
        return QString("0:%1").arg(sec, 2, 10, QLatin1Char('0'));
    else
    {
        int min = sec / 60;
        if (min < 60)
            return QString("%1:%2").arg(min).arg(sec - min*60, 2, 10, QLatin1Char('0'));
        else
        {
            int h = min / 60;
            return QString("%1:%2:%3").arg(h).arg(min - 60*h, 2, 10, QLatin1Char('0')).arg(sec - min*60, 2, 10, QLatin1Char('0'));
        }
    }
}

QString ClementineRemote::disconnectReason(short reason) const
{
    switch (reason) {
    case pb::remote::ReasonDisconnect::Server_Shutdown:
        return tr("Server Shutdown");
    case pb::remote::ReasonDisconnect::Wrong_Auth_Code:
        return tr("Wrong Authentication code");
    case pb::remote::ReasonDisconnect::Not_Authenticated:
        return tr("Not authenticated");
    case pb::remote::ReasonDisconnect::Download_Forbidden:
        return tr("Download fobidden");
    default:
        return tr("Unknown Reason...");
    }
}

#ifdef __USE_PLAYLIST_PROXY_MODEL__
int ClementineRemote::modelRowFromProxyRow(int proxyRow) const
{
    QModelIndex proxyIndex = _playlistProxyModel->index(proxyRow, 0);
    if (proxyIndex.isValid())
    {
        QModelIndex modelIndex = _playlistProxyModel->mapToSource(proxyIndex);
        if (modelIndex.isValid())
            return modelIndex.row();
    }
    return -1;
}
#endif

bool ClementineRemote::hideServerFilesPreviousNextNavButtons() const { return true; }

const QString &ClementineRemote::remoteFilesPath() const { return _remoteFilesPath; }
QString ClementineRemote::remoteFilesPath_QML() const{ return _remoteFilesPath; }

const QString ClementineRemote::clemVersion() const { return _clemVersion; }
QString ClementineRemote::clementineFilesSupportMinVersion() { return QString("%1.%2").arg(sClemFilesSupportMinVersion.first).arg(sClemFilesSupportMinVersion.second);}
bool ClementineRemote::clementineFilesSupport() const { return _clemFilesSupport; }

int ClementineRemote::activeSongIndex() const
{
    QModelIndex modelIndex = _playlistModel->index(_activeSongIndex, 0);
    if (modelIndex.isValid())
    {
        QModelIndex proxyIndex = _playlistProxyModel->mapFromSource(modelIndex);
        if (proxyIndex.isValid())
            return proxyIndex.row();
    }
    return -1;
}

QString ClementineRemote::settingHost() const { return _settings.value("host", "").toString(); }
QString ClementineRemote::settingPort() const { return _settings.value("port", "").toString(); }
QString ClementineRemote::settingPass() const { return _settings.value("pass", "").toString(); }

void ClementineRemote::saveSettings(const QString &host, const QString &port, const QString &pass)
{
    _settings.setValue("host", host);
    _settings.setValue("port", port);
    _settings.setValue("pass", pass);
    _settings.sync();
}


#endif // CLEMENTINEREMOTE_H
