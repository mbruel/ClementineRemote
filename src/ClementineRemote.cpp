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

#include "ClementineRemote.h"
#include "ClementineSession.h"
#include "ConnectionWorker.h"
#include "model/RemoteSongModel.h"
#include "model/PlaylistModel.h"
#include "model/LibraryModel.h"
#include "player/RemotePlaylist.h"

#include <QTcpSocket>
#include <QDataStream>
#include <QStandardPaths>
#include <QDir>
#include <QUrl>
#include <QElapsedTimer>
#include <QSqlQuery>

#if defined(Q_OS_ANDROID)
#include <QtAndroid>
#endif

const QString ClementineRemote::sAppName    = QStringLiteral("ClemRemote");
const QString ClementineRemote::sVersion    = "1.1";
const QString ClementineRemote::sAppTitle   = tr("Clementine Remote");
const QString ClementineRemote::sProjectUrl = QStringLiteral("https://github.com/mbruel/ClementineRemote");
const QString ClementineRemote::sBTCaddress = QStringLiteral("3BGbnvnnBCCqrGuq1ytRqUMciAyMXjXAv6");
const QString ClementineRemote::sDonateUrl  = QStringLiteral("https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=W2C236U6JNTUA&item_name=ClementineRemote&currency_code=EUR");
const QString ClementineRemote::sClementineReleaseURL = QStringLiteral("https://github.com/mbruel/Clementine/releases/tag/1.4.0rc1ClemRemote");

const QPair<ushort, ushort> ClementineRemote::sClemFilesSupportMinVersion = {1, 4};

const QString ClementineRemote::sLibrarySQL =
        QStringLiteral("select artist, album, title, track, filename from songs order by artist, album, track, title");

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

const QMap<ClementineRemote::Settings, QString> ClementineRemote::sSettings = {
    {Settings::session,               QStringLiteral("session")},
    {Settings::host,                  QStringLiteral("host")},
    {Settings::port,                  QStringLiteral("port")},
    {Settings::pass,                  QStringLiteral("pass")},
    {Settings::remotePath,            QStringLiteral("remotePath")},
    {Settings::lastSession,           QStringLiteral("lastSession")},
    {Settings::downloadPath,          QStringLiteral("downloadPath")},
    {Settings::verticalVolume,        QStringLiteral("verticalVolume")},
    {Settings::iconSize,              QStringLiteral("iconSize")},
    {Settings::dispArtistInTrackName, QStringLiteral("dispArtistInTrackName")},
    {Settings::delayLibraryLoading,   QStringLiteral("delayLibraryLoading")},
};


const QString ClementineRemote::sQuickSessionName = ClementineRemote::tr("Quick Session");

ClementineRemote::ClementineRemote(QObject *parent):
    QObject(parent), Singleton<ClementineRemote>(),
#ifdef __USE_CONNECTION_THREAD__
    _thread(),
#endif
    _connection(new ConnectionWorker(this)),
    #if defined( Q_OS_WIN )
    _settings("clemRemote.ini", QSettings::Format::IniFormat),
    #else
    _settings(QSettings::UserScope),
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
    _activePlaylistId(-1), _requestSongsForPlaylistID(-1),
    _trackPostition(0),
    _initialized(false),
    _clemFilesSupport(false),
    _remoteFilesPath("./"),
    _remoteFiles(),
#ifdef __USE_CONNECTION_THREAD__
    _secureRemoteFilesData(), _remoteFilesData(),
#endif
    _radioStreams(),
#ifdef __USE_CONNECTION_THREAD__
    _secureRadioStreams(), _radioStreamsData(),
#endif
    _isDownloading(0x0),
#if defined(Q_OS_ANDROID)
    _libraryPath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)),
#elif defined(Q_OS_IOS)
    _libraryPath(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)),
#else
    _libraryPath(QFileInfo(_settings.fileName()).absolutePath()),
#endif
    _libDB(), _libModel(new LibraryModel), _libProxyModel(new LibraryProxyModel),
#ifdef __USE_CONNECTION_THREAD__
    _secureUserMsg(),
#endif
    _userMsg(),
    _forceRePlayActiveSong(false),
    _sessionsSaved(), _sessionSelected(0),
    _libraryLoaded(false)
{
    setObjectName(sAppName);

    _songsModel->setRemote(this);
    _songsProxyModel->setSourceModel(_songsModel);
    _libProxyModel->setSourceModel(_libModel);

#ifdef __USE_CONNECTION_THREAD__
    connect(this, &ClementineRemote::initialized,
            this, &ClementineRemote::onInitialized, Qt::QueuedConnection);
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

    // load sessions
    QString lastSession = _settings.value(sSettings[Settings::lastSession], sQuickSessionName).toString();
    int nbSessions = _settings.beginReadArray(sSettings[Settings::session]);
    for (int i = 0; i < nbSessions; ++i) {
        _settings.setArrayIndex(i);
        QString session = _settings.value(sSettings[Settings::session]).toString();
        _sessionsSaved <<  new ClementineSession(session, this,
                                                 _settings.value(sSettings[Settings::host]).toString(),
                                                 static_cast<ushort>(_settings.value(sSettings[Settings::port]).toUInt()),
                                                 _settings.value(sSettings[Settings::pass]).toInt(),
                                                 _settings.value(sSettings[Settings::remotePath]).toString());
        if (session == lastSession) {
            _sessionSelected = i;
            qDebug() << "Last session : " << session << " (#" << _sessionSelected << ")";
        }
    }
    _settings.endArray();
    if (_sessionsSaved.isEmpty())
        _sessionsSaved << new ClementineSession(sQuickSessionName, this);

    qDebug() << "Settings filename: " << _settings.fileName()
             << " => libraryPath: " << _libraryPath;

    connect(this, &ClementineRemote::libraryDownloaded, this, &ClementineRemote::onLibraryDownloaded, Qt::QueuedConnection);

    RemoteSong::sDispArtistInName = _settings.value(sSettings[Settings::dispArtistInTrackName], true).toBool();

#ifdef Q_OS_IOS
    if (!_settings.contains(sSettings[Settings::verticalVolume]))
        setVerticalVolumeSlider(true);
#endif
}

ClementineRemote::~ClementineRemote()
{
    qDebug() << "[MB_TRACE] destruction ClementineRemote";
    close();
}

void ClementineRemote::saveSessions()
{
    _settings.beginWriteArray(sSettings[Settings::session]);
    int sessionIdx = 0;
    for (ClementineSession *session : _sessionsSaved) {
        qDebug() << session->str();
        _settings.setArrayIndex(sessionIdx++);
        _settings.setValue(sSettings[Settings::session],    session->name());
        _settings.setValue(sSettings[Settings::host],       session->host());
        _settings.setValue(sSettings[Settings::port],       session->port());
        _settings.setValue(sSettings[Settings::pass],       session->pass());
        _settings.setValue(sSettings[Settings::remotePath], session->remotePath());
    }
    _settings.endArray();
    _settings.setValue(sSettings[Settings::lastSession],  sessionName());
    _settings.sync();
}

void ClementineRemote::close()
{
    qDebug() << "[MB_TRACE] close ClementineRemote";
    saveSessions();

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
    if (_songsModel)
    {
        delete _songsModel;
        _songsModel = nullptr;
        delete _songsProxyModel;
        _songsProxyModel = nullptr;
    }
    if (_libModel)
    {
        delete _libModel;
        _libModel = nullptr;
        delete _libProxyModel;
        _libProxyModel = nullptr;
    }
    if (_connection)
    {
        delete _connection;
        _connection = nullptr;
    }
}

void ClementineRemote::clearData(const QString &reason)
{
    emit disconnected(reason); // Update QML view to Login Page

    _initialized = false;
    _forceRePlayActiveSong = false;

    qDeleteAll(_playlistsOpened);
    _playlistsOpened.clear();

    qDeleteAll(_playlistsClosed);
    _playlistsClosed.clear();


    _dispPlaylist = nullptr;
    _dispPlaylistId = 0;
    _dispPlaylistIndex = 0;

    _songs.clear();

    _activeSongIndex = 0;
    _activePlaylistId = 0;

    _remoteFiles.clear();
    _radioStreams.clear();

    _libModel->clear();
    _libDB.close();
    _libraryLoaded = false;

    _isDownloading = 0x0;    
}


QString ClementineRemote::testDownloadPath()
{
    downloadPath();
    qDebug() << "Download path: " << _downloadPath;
    QFileInfo fi(_downloadPath);
    if (!fi.isDir())
        return tr("the download folder '%1' is not a directory...").arg(_downloadPath);
    else if (!fi.isWritable())
        return tr("the download folder '%1' is not writable...").arg(_downloadPath);
    else
        return QString();
}

QString ClementineRemote::downloadPath()
{
    qDebug() << "standard writable app data loc: " << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    qDebug() << "standard writable app conf loc: "     << QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    qDebug() << "standard writable data loc: "     << QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    qDebug() << "standard writable apps loc: "     << QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    qDebug() << "standard writable doc loc: "      << QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    qDebug() << "standard writable music loc: "    << QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    qDebug() << "standard writable down loc: "     << QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);


    if (_settings.contains(sSettings[Settings::downloadPath]))
        _downloadPath = _settings.value(sSettings[Settings::downloadPath]).toString();
    else
    {
#if defined(Q_OS_ANDROID)
        _downloadPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        QtAndroid::PermissionResultMap res = QtAndroid::requestPermissionsSync({"android.permission.WRITE_EXTERNAL_STORAGE"});
        if (res["android.permission.WRITE_EXTERNAL_STORAGE"] != QtAndroid::PermissionResult::Granted)
            return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#elif defined(Q_OS_IOS)
        _downloadPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#else
        _downloadPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
#endif
        qDebug() << "Download path: " << _downloadPath;
        QFileInfo fi(_downloadPath);
        if (!fi.isDir())
            qCritical() << tr("the download folder is not a directory...");
        else if (!fi.isWritable())
            qCritical() << tr("the download folder is not writable...");
        else {
#if defined(Q_OS_IOS)
            QString downFolder = "Downloads"; // using UIFileSharingEnabled, a ClemRemote folder is already visible in Files
#else
            QString downFolder = sAppName;
#endif
            fi = QFileInfo(QString("%1/%2").arg(_downloadPath).arg(downFolder));
            if (!fi.exists())
            {
                QDir dir(_downloadPath);
                if (!dir.mkdir(downFolder))
                    qCritical() << tr("error creating default Download folder: %1").arg(fi.absoluteFilePath());
            }
            _downloadPath = fi.absoluteFilePath();
        }
    }
    _settings.setValue(sSettings[Settings::downloadPath], _downloadPath);
    _settings.sync();
    qDebug() << "Download path: " << _downloadPath;
    return _downloadPath;
}

QUrl ClementineRemote::downloadPathURL() { return QUrl::fromLocalFile(downloadPath()); }

void ClementineRemote::updateDownloadPath(const QString &newPath)
{
    QFileInfo fi(newPath);
    if (!fi.exists())
        qCritical() << "New path doesn't exist... " << newPath;
    else if (!fi.isDir())
        qCritical() << "New path is not a directory... " << newPath;
    else if (!fi.isWritable())
        qCritical() << "New path is not writable... " << newPath;
    else
    {
        _downloadPath = newPath;
        _settings.setValue(sSettings[Settings::downloadPath], _downloadPath);
    }
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
        updateActiveSong(msg.response_current_metadata().song_metadata());
        break;

    case pb::remote::SET_VOLUME:
        _volume = msg.request_set_volume().volume();
        qDebug() << "[MsgType::SET_VOLUME] " << _volume;
        emit updateVolume(_volume);
        break;

    case pb::remote::UPDATE_TRACK_POSITION:
        _trackPostition = msg.response_update_track_position().position();
        emit activeTrackPosition(_trackPostition);
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
        if (_initialized)
            return; // it's for another remote        
#ifdef __USE_CONNECTION_THREAD__
        emit initialized();
#else
        _initialized = true;
        qDebug() << "[MsgType::FIRST_DATA_SENT_COMPLETE] fully Initialized \\o/";
        emit connected();
        if (!delayLibraryLoading())
            requestLibrary();
#endif
        if (_clemFilesSupport)
            _connection->requestSavedRadios();
        break;

    case pb::remote::PLAY:
        qDebug() << "[MsgType::PLAY]";
        _clemState = pb::remote::EngineState::Playing;
        emit updateEngineState();
        if (_forceRePlayActiveSong)
        {
            // Hack to make sure last played song will be played
            // as Clementine would start the one from it's current opened tab
            qDebug() << "Force replay OUR active song: " << _activeSongAndPlaylistIndexes.first
                     << " (playlist: " << _activeSongAndPlaylistIndexes.second << ")";
            _connection->sendChangeSong(_activeSongAndPlaylistIndexes.first, _activeSongAndPlaylistIndexes.second);
            _forceRePlayActiveSong = false;
        }

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
        _activePlaylistId =  msg.response_active_changed().id();
        qDebug() << "[MsgType::ACTIVE_PLAYLIST_CHANGED] id: " << _activePlaylistId;
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
    case pb::remote::LIBRARY_CHUNK:
        _connection->downloadLibrary(msg.response_library_chunk());
        break;

    default:
        qDebug() << "Msg type not yet implemented: " << msgType;
        break;
    }
}



////////////////////////////////
/// Library methods
////////////////////////////////

void ClementineRemote::setLibraryFilter(const QString &searchTxt)
{
    qDebug() << "[MB_TRACE][ClementineRemote::setSongsFilter] searchTxt: " << searchTxt;
    Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive;
    QRegExp regExp(searchTxt, caseSensitivity);
    _libProxyModel->setFilterRegExp(regExp);
}

void ClementineRemote::appendLibraryItem(const QModelIndex &proxyIndex, const QString &newPlaylistName)
{
    if (!proxyIndex.isValid())
    {
        sendError(tr("Nothing selected"), tr("Select either a single Track or an Album"));
        return;
    }
    int itemType = _libProxyModel->data(proxyIndex, LibraryModel::type).toInt();
    if (itemType == LibraryModel::Album)
    {
        qDebug() << "Library album activated: " << _libProxyModel->data(proxyIndex, LibraryModel::name).toString();
        int childCount = _libProxyModel->rowCount(proxyIndex);
        if (childCount == 0)
        {
            sendError("", tr("No tracks in the selected album %1").arg(
                          _libProxyModel->data(proxyIndex, LibraryModel::name).toString()));
            return;
        }
        lockUserMutex();
        _userMsg.Clear();
        _userMsg.set_type(pb::remote::INSERT_URLS);
        pb::remote::RequestInsertUrls *req = _userMsg.mutable_request_insert_urls();
        for (int i = 0; i < childCount; ++i)
        {
            QModelIndex childIndex = _libProxyModel->index(i, 0, proxyIndex);
            QString url = _libProxyModel->data(childIndex, LibraryModel::url).toString();
            qDebug() << "Adding Library track from album: " << url;
            *req->add_urls() = url.toStdString();
        }
        emit insertUrls(_dispPlaylistId, newPlaylistName);
        sendInfo("", tr("%1 Tracks have been added to the playlist %2").arg(
                     childCount).arg(
                     newPlaylistName.isEmpty() ? playlistName() : newPlaylistName));
    }
    else if (itemType == LibraryModel::Track)
    {
        lockUserMutex();
        _userMsg.Clear();
        _userMsg.set_type(pb::remote::INSERT_URLS);
        pb::remote::RequestInsertUrls *req = _userMsg.mutable_request_insert_urls();
        QString url = _libProxyModel->data(proxyIndex, LibraryModel::url).toString();
        *req->add_urls() = url.toStdString();
        qDebug() << "Library track activated: " << url;
        emit insertUrls(_dispPlaylistId, newPlaylistName);
        sendInfo("", tr("the track %1 has been added to the playlist %2").arg(
                     _libProxyModel->data(proxyIndex, LibraryModel::name).toString()).arg(
                     newPlaylistName.isEmpty() ? playlistName() : newPlaylistName));
    }
    else
        sendError("", tr("You can't add an Artist directly.<br/> Please select either a single Track or an Album"));
}


void ClementineRemote::downloadLibraryItem(const QModelIndex &proxyIndex)
{
    if (!proxyIndex.isValid())
    {
        sendError(tr("Nothing selected"), tr("Select either a single Track or an Album"));
        return;
    }
    int itemType = _libProxyModel->data(proxyIndex, LibraryModel::type).toInt();
    if (itemType == LibraryModel::Artist)
    {
        sendError("", tr("You can't add an Artist directly.<br/> Please select either a single Track or an Album"));
        return;
    }

    lockUserMutex();
    _userMsg.Clear();
    _userMsg.set_type(pb::remote::DOWNLOAD_SONGS);
    pb::remote::RequestDownloadSongs *req = _userMsg.mutable_request_download_songs();
    req->set_download_item(pb::remote::DownloadItem::Urls);
    if (itemType == LibraryModel::Album)
    {
        QString albumName = _libProxyModel->data(proxyIndex, LibraryModel::name).toString();
        qDebug() << "Library album to download: " << albumName;
        int childCount = _libProxyModel->rowCount(proxyIndex);
        if (childCount == 0)
        {
            sendError("", tr("No tracks in the selected album %1").arg(albumName));
            return;
        }
        for (int i = 0; i < childCount; ++i)
        {
            QModelIndex childIndex = _libProxyModel->index(i, 0, proxyIndex);
            QString url = _libProxyModel->data(childIndex, LibraryModel::url).toString();
            qDebug() << "Adding Library track from album: " << url;
            *req->add_urls() = url.toStdString();
        }
        emit sendSongsToDownload(albumName);
    }
    else if (itemType == LibraryModel::Track)
    {
        QString url = _libProxyModel->data(proxyIndex, LibraryModel::url).toString();
        qDebug() << "Library track to download: " << url;
        *req->add_urls() = url.toStdString();
        emit sendSongsToDownload(QString());
    }
}

void ClementineRemote::getLibrary(){
    _libraryLoaded = false;
    emit _connection->getLibrary();
}

void ClementineRemote::requestLibrary()
{
    if (_libraryLoaded)
        return;
    if (_sessionSelected > 0 // We force redownload for Quick Session
            && QFileInfo(QString("%1/%2.db").arg(_libraryPath).arg(sessionName())).exists())
        emit libraryDownloaded();
    else
        getLibrary();
}


////////////////////////////////
/// QML getter/setters
////////////////////////////////

bool ClementineRemote::isConnected() const { return _connection->isConnected(); }
void ClementineRemote::cancelDownload() const { _connection->cancelDownload(); }

QString ClementineRemote::hostname() const
{
    if (_sessionSelected == 0) // Quick Session
        return _sessionsSaved.at(_sessionSelected)->host();
    else
        return _sessionsSaved.at(_sessionSelected)->name();
}

QString ClementineRemote::sessionName() const
{
    return _sessionsSaved.at(_sessionSelected)->name();
}

QStringList ClementineRemote::sessionNames() const
{
    QStringList sessions;
    for (ClementineSession *s : _sessionsSaved)
        sessions << s->name();
    return sessions;
}

ClementineSession *ClementineRemote::getSession(int index)
{
    if (index < 0){
        _sessionSelected = 0;
        qCritical() << "WRONG session index...";
    }
    else if (index < _sessionsSaved.size())
        _sessionSelected = index;
    else
    {
        _sessionSelected = 0;
        qCritical() << "WRONG session index...";
    }

    return _sessionsSaved.at(_sessionSelected);
}

void ClementineRemote::tryConnectToServer(int sessionIndex, const QString &host, ushort port, int auth_code)
{
    _sessionSelected = sessionIndex;
    ClementineSession *session = _sessionsSaved.at(sessionIndex);
    if (sessionIndex == 0)
    {
        session->setHost(host);
        session->setPort(port);
        session->setPass(auth_code);
    }
    setRemotePathForHost();
    emit _connection->connectToServer(session);
}

int ClementineRemote::createNewSession(const QString &sessionName, const QString &host, int port, int pass)
{
    _sessionsSaved << new ClementineSession(sessionName, this, host, static_cast<ushort>(port), pass);
    return _sessionsSaved.size() - 1;
}

void ClementineRemote::deleteCurrentSession()
{
    if (_sessionSelected == 0)
        return; // can't delete Quick Session

    ClementineSession *session = _sessionsSaved[_sessionSelected];
    _sessionsSaved.removeAt(_sessionSelected);
    delete session;
    _sessionSelected = 0;
}


////////////////////////////////
/// Playlist methods
////////////////////////////////

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

int ClementineRemote::getAtivePlaylistIndex()
{
    int idx = 0;
    for (RemotePlaylist *p : qAsConst(_playlistsOpened))
    {
        if (p->id == _activePlaylistId)
            return idx;
        ++idx;
    }
    return idx;
}

qint32 ClementineRemote::displayedPlaylistID() const
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

bool ClementineRemote::isActivePlaylistDisplayed() const
{
    return _dispPlaylist ? _dispPlaylist->id == _activePlaylistId : false;
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
    if (newDispPlaylist && _activePlaylistId == playlistID)
        _activePlaylistId = newDispPlaylist->id;
}

void ClementineRemote::updateActivePlaylist()
{
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

void ClementineRemote::changeAndPlaySong(int songIndex, qint32 playlistID)
{
    _connection->sendChangeSong(songIndex, playlistID);

    // this will play music so we need to update the remote
    setPlay();
    emit updateEngineState();
}

void ClementineRemote::setRemotePathForHost()
{
    if (_sessionSelected < _sessionsSaved.size())
        _remoteFilesPath = _sessionsSaved[_sessionSelected]->remotePath();
    else
        _remoteFilesPath = "./"; // should never happen
}

void ClementineRemote::updateCurrentPlaylist()
{
    _dispPlaylist       = nullptr;
    _dispPlaylistIndex = 0;
    int idx = 0;
    for (RemotePlaylist *p : qAsConst(_playlistsOpened))
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



////////////////////////////////
/// RemoteFile methods
////////////////////////////////

int ClementineRemote::sendSelectedFiles(const QString &newPlaylistName)
{
    lockUserMutex();
    _userMsg.Clear();
    _userMsg.set_type(pb::remote::APPEND_FILES);
    QStringList selectedFiles;
    for (const RemoteFile &f : _remoteFiles)
    {
        if (!f.isDir && f.selected)
            selectedFiles << f.filename;
    }
    if (selectedFiles.isEmpty())
    {
        releaseUserMutex();
        sendError(tr("No track selected"),
                  tr("Please select at least one Track<br/>You need to be in <b>selection mode</b>.<br/>\
For that either do a long press an a Track or use one of the 2 bottoms on the right hand."));
        return 0;
    }
    else
    {
        pb::remote::RequestAppendFiles *files = _userMsg.mutable_request_append_files();
        files->set_relative_path(_remoteFilesPath.toStdString());
        if (!newPlaylistName.isEmpty())
            files->set_new_playlist_name(newPlaylistName.toStdString());
        else
            files->set_playlist_id(_dispPlaylistId);

        for (const QString &filename : selectedFiles)
            *files->add_files() = filename.toStdString();

        sendInfo("", tr("%1 Tracks have been added to the playlist %2").arg(
                     selectedFiles.size()).arg(
                     newPlaylistName.isEmpty() ? playlistName() : newPlaylistName));
        emit sendFilesToAppend();
        return selectedFiles.size();
    }
}

void ClementineRemote::downloadSelectedFiles()
{
    lockUserMutex();
    _userMsg.Clear();
    _userMsg.set_type(pb::remote::DOWNLOAD_SONGS);
    QStringList selectedFiles;
    for (const RemoteFile &f : _remoteFiles)
    {
        if (!f.isDir && f.selected)
            selectedFiles << f.filename;
    }
    if (selectedFiles.isEmpty())
    {
        releaseUserMutex();
        sendError("No selected files",
                  tr("You should at least select one music file to download..."));
    }
    else
    {
        pb::remote::RequestDownloadSongs *files = _userMsg.mutable_request_download_songs();
        files->set_download_item(pb::remote::DownloadItem::Urls);
        files->set_relative_path(_remoteFilesPath.toStdString());
        for (const QString &filename : qAsConst(selectedFiles))
            *files->add_urls() = filename.toStdString();

        emit sendSongsToDownload(QFileInfo(_remoteFilesPath).fileName());
    }
}

void ClementineRemote::doSendFilesToAppend()
{
    _connection->sendDataToServer(_userMsg);
    _userMsg.clear_request_append_files();
    releaseUserMutex();
}


////////////////////////////////
/// RemoteSong methods
////////////////////////////////

int ClementineRemote::getActiveSongIndex() const
{
    if (isActivePlaylistDisplayed())
        return _songsProxyModel->mapFromSource(_songsModel->index(_activeSongIndex)).row();
    else
        return _activeSongIndex;
}
void ClementineRemote::updateActiveSong(RemoteSong &&activeSong)
{
    _activeSong = activeSong;

    int idx = 0;
    for (const RemoteSong &s : qAsConst(_songs))
    {
        if (s.index == _activeSong.index)
        {
            _activeSongIndex = idx;
            if (isActivePlaylistDisplayed())
                emit activeSongIdx(_songsProxyModel->mapFromSource(_songsModel->index(idx)).row());
            break;
        }
        ++idx;
    }

    emit activeSongDetails(_activeSong.name(), _activeSong.length, _activeSong.pretty_length);
    qDebug() << "[MsgType::CURRENT_METAINFO] " << _activeSong.str();
}

void ClementineRemote::setSongsFilter(const QString &searchTxt)
{
    qDebug() << "[MB_TRACE][ClementineRemote::setSongsFilter] searchTxt: " << searchTxt;
    Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive;
    QRegExp regExp(searchTxt, caseSensitivity);
    _songsProxyModel->setFilterRegExp(regExp);
}

void ClementineRemote::deleteSelectedSongs()
{
#ifdef __USE_CONNECTION_THREAD__
    QMutexLocker lock(&_secureSongs);
#endif
    lockUserMutex();
    _userMsg.Clear();
    _userMsg.set_type(pb::remote::REMOVE_SONGS);
    QList<int> selectedSongsIdexes = _songsProxyModel->selectedSongsIdexes();
    if (selectedSongsIdexes.isEmpty())
    {
        releaseUserMutex();
        return ;
    }
    else
    {
        pb::remote::RequestRemoveSongs *req = _userMsg.mutable_request_remove_songs();
        req->set_playlist_id(_dispPlaylistId);
        for (int songIndex : selectedSongsIdexes)
            req->add_songs(songIndex);

        emit sendSongsToRemove();
    }
}

void ClementineRemote::downloadSelectedSongs()
{
#ifdef __USE_CONNECTION_THREAD__
    QMutexLocker lock(&_secureSongs);
#endif
    lockUserMutex();
    _userMsg.Clear();
    _userMsg.set_type(pb::remote::DOWNLOAD_SONGS);
    QList<int> selectedSongsIDs = _songsProxyModel->selectedSongsIDs();
    if (selectedSongsIDs.isEmpty())
    {
        releaseUserMutex();
        return ;
    }
    else
    {
        pb::remote::RequestDownloadSongs *req = _userMsg.mutable_request_download_songs();
        req->set_playlist_id(_dispPlaylistId);
        req->set_download_item(pb::remote::DownloadItem::APlaylist);
        for (int songID : selectedSongsIDs)
            req->add_songs_ids(songID);

        emit sendSongsToDownload(QString());//playlistName());
    }
}

bool ClementineRemote::appendSongsToOtherPlaylist()
{
#ifdef __USE_CONNECTION_THREAD__
    QMutexLocker lock(&_secureSongs);
#endif
    lockUserMutex();
    _userMsg.Clear();
    _userMsg.set_type(pb::remote::INSERT_URLS);
    QStringList selectedSongsURLs = _songsProxyModel->selectedSongsURLs();

    if (selectedSongsURLs.isEmpty())
    {
        releaseUserMutex();
        return false;
    }
    else
    {
        pb::remote::RequestInsertUrls *req = _userMsg.mutable_request_insert_urls();
        for (const QString &url : selectedSongsURLs)
            *req->add_urls() = url.toStdString();
        emit askPlaylistDestID();
        return true;
    }
}

void ClementineRemote::doSendSongsToRemove()
{
    _connection->sendDataToServer(_userMsg);
    _userMsg.clear_request_append_files();
    releaseUserMutex();
}

void ClementineRemote::doSendSongsToDownload()
{
    _connection->sendDataToServer(_userMsg);
    _userMsg.clear_request_download_songs();
    releaseUserMutex();
}

void ClementineRemote::doSendInsertUrls(qint32 playlistID, const QString &newPlaylistName)
{
    pb::remote::RequestInsertUrls *req = _userMsg.mutable_request_insert_urls();
    if (newPlaylistName.isEmpty())
        req->set_playlist_id(playlistID);
    else
        req->set_new_playlist_name(newPlaylistName.toStdString());
    _connection->sendDataToServer(_userMsg);
    _userMsg.clear_request_insert_urls();
    releaseUserMutex();
}

void ClementineRemote::shallForceRePlayActiveSong()
{
    if (_previousClemState == pb::remote::EngineState::Idle)
    {
        _activeSongAndPlaylistIndexes = {_activeSongIndex, _activePlaylistId};
        _forceRePlayActiveSong = true;
    }
}



////////////////////////////////
/// private methods
////////////////////////////////


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
                _playlistsClosed << new RemotePlaylist(pb_playlist, _activePlaylistId);
                emit _plClosedModel->postAddPlaylist(idxClosed++);
            }
            else
            {
                emit _plOpenedModel->preAddPlaylist(idxOpened);
                _playlistsOpened << new RemotePlaylist(pb_playlist, _activePlaylistId);
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




void ClementineRemote::rcvPlaylistSongs(const pb::remote::ResponsePlaylistSongs &songs)
{
    qint32 playlistID = songs.requested_playlist().id();
    qDebug() << "[MsgType::PLAYLIST_SONGS] playlist ID: " << playlistID;
    if (playlistID != _dispPlaylistId && // always update displayed playlist
            _initialized && playlistID != _requestSongsForPlaylistID.loadRelaxed())
    {
        qDebug() << "[MsgType::PLAYLIST_SONGS] ignoring msg, _dispPlaylistId: " << _dispPlaylistId
                 << ", _requestSongsForPlaylistID: " << _requestSongsForPlaylistID.loadRelaxed();
        return;
    }
    else if (playlistID == _requestSongsForPlaylistID.loadRelaxed())
        _requestSongsForPlaylistID = -1; // unset for next request

    _dispPlaylistId = playlistID;
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

void ClementineRemote::rcvListOfRemoteFiles(const pb::remote::ResponseListFiles &files)
{
    if (files.has_error() && files.error() != pb::remote::ResponseListFiles::NONE)
        sendError("", _remoteFilesListError(files.error(), files.relative_path()));
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
        _sessionsSaved[_sessionSelected]->setRemotePath(_remoteFilesPath);
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

void ClementineRemote::dumpPlaylists()
{
    for (RemotePlaylist *p : _playlistsOpened)
        qDebug() << "  - " << p->str();
}

void ClementineRemote::dumpCurrentPlaylist()
{
    for (const RemoteSong &s : _songs)
        qDebug() << "  - " << s.str();
}




////////////////////////////////
/// slots
////////////////////////////////

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
    {
        _activePlaylistId = _dispPlaylistId;
        updateActivePlaylist();
    }
}
void ClementineRemote::onRemoteFilesUpdatedByWorker()
{
    rcvListOfRemoteFiles(_remoteFilesData.response_list_files());
    _remoteFilesData.clear_response_list_files();
    _secureRemoteFilesData.unlock();
}

void ClementineRemote::onInitialized()
{
    _initialized = true;
    qDebug() << "[MsgType::FIRST_DATA_SENT_COMPLETE] fully Initialized \\o/";
    emit connected();

    if (!delayLibraryLoading())
        requestLibrary();
}
#endif

void ClementineRemote::onLibraryDownloaded()
{
    QString host = sessionName();
    if (QSqlDatabase::contains(host))
        _libDB = QSqlDatabase::database(host);
    else
        _libDB = QSqlDatabase::addDatabase("QSQLITE", host);
    _libDB.setDatabaseName(QString("%1/%2.db").arg(_libraryPath).arg(host));
    if(!_libDB.open()){
        qCritical() << "Can't open sqlite DB... " << _libDB.databaseName();
        return;
    }

    emit _libModel->beginReset();
    _libModel->clear();

    QElapsedTimer timeStart;
    timeStart.start();

    int nbArtists = 0, nbAlbums = 0, nbTracks = 0;
    QSqlQuery query(_libDB);
    if(!query.exec(sLibrarySQL))
        qDebug() << "Can't Execute Query !";
    else
    {
        QStandardItem *rootItem = _libModel->invisibleRootItem(),
                *artistItem = nullptr, *albumItem = nullptr;
        QString currentArtist, currentAlbum;
        while(query.next())
        {
            QString artist   = query.value(0).toString();
            QString album    = query.value(1).toString();
            QString title    = query.value(2).toString();
            int     track    = query.value(3).toInt();
            QString filename = query.value(4).toString();
            ++nbTracks;

            if (!artistItem || artist != currentArtist)
            {
                artistItem = new QStandardItem();
                artistItem->setData(artist.isEmpty() ? tr("unset artist") : artist,
                                    LibraryModel::name);
                artistItem->setData(LibraryModel::Artist, LibraryModel::type);
                rootItem->appendRow(artistItem);
                currentArtist = artist;
                ++nbArtists;
            }

            if (!albumItem || album != currentAlbum)
            {
                albumItem = new QStandardItem();
                albumItem->setData(album.isEmpty() ? tr("unset album") : album,
                                    LibraryModel::name);
                albumItem->setData(LibraryModel::Album, LibraryModel::type);
                artistItem->appendRow(albumItem);
                currentAlbum = album;
                ++nbAlbums;
            }

            QStandardItem *trackItem = new QStandardItem();
            if (track == -1)
                trackItem->setData(title, LibraryModel::name);
            else
                trackItem->setData(QString("%1 - %2").arg(track, 2, 10, QChar('0')).arg(title),
                                   LibraryModel::name);
            trackItem->setData(filename.toLower().endsWith("m3u")?LibraryModel::Playlist:LibraryModel::Track,
                               LibraryModel::type);
            trackItem->setData(filename, LibraryModel::url);
            albumItem->appendRow(trackItem);
        }
    }
    emit _libModel->endReset();
    _libraryLoaded = true;
    emit libraryLoaded(); // warn QML for easy-loading

    qint64 durationMS = timeStart.elapsed();

//   QTime::fromMSecsSinceStartOfDay(static_cast<int>(duration)).toString("hh:mm:ss.zzz"));
    sendInfo(tr("Library loaded in %1 ms").arg(durationMS),
             tr("<ul><li>Number of Artists: %1</li><li>Number of Albums: %2</li><li>Number of Tracks: %3</li></ul>").arg(
                 nbArtists).arg(nbAlbums).arg(nbTracks));
}


