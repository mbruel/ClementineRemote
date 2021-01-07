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

#ifndef REMOTESONG_H
#define REMOTESONG_H
#include "protobuf/remotecontrolmessages.pb.h"

#include <QtGlobal>
#include <QString>
#include <QImage>
typedef struct RemoteSong
{
    qint32 id; // unique id of the song
    qint32 index; // Index of the current row of the active playlist
    QString title;
    QString album;
    QString artist ;
    QString albumartist;
    qint32 track;
    qint32 disc;
    QString pretty_year;
    QString genre;
    qint32 playcount;
    QString pretty_length;
    qint32 length;
    bool is_local;
    QString filename;
    qint32 file_size;
    float rating; // 0 (0 stars) to 1 (5 stars)
    QString url;
    QString art_automatic;
    QString art_manual;
    pb::remote::SongMetadata_Type type;
    QImage art;

    bool selected; // for its selection in ListView

    static bool sDispArtistInName;

public:
    RemoteSong() = default;
    RemoteSong(const RemoteSong &) = default;
    RemoteSong(RemoteSong &&) = default;

    RemoteSong(const pb::remote::SongMetadata &m):
        id(m.id()), index(m.index()), title(m.title().c_str()), album(m.album().c_str()),
        artist(m.artist().c_str()), albumartist(m.albumartist().c_str()), track(m.track()),
        disc(m.disc()), pretty_year(m.pretty_year().c_str()), genre(m.genre().c_str()),
        playcount(m.playcount()), pretty_length(m.pretty_length().c_str()), length(m.length()),
        is_local(m.is_local()), filename(m.filename().c_str()), file_size(m.file_size()), rating(m.rating()),
        url(m.url().c_str()), art_automatic(m.art_automatic().c_str()), art_manual(m.art_manual().c_str()),
        type(m.type()), art(),
        selected(false)
    {
        if (m.has_art() && m.art().size())
            art = QImage(m.art().c_str());
    }

    RemoteSong& operator=(const RemoteSong &) = default;
    RemoteSong& operator=(RemoteSong &&) = default;

    ~RemoteSong() = default;

    inline QString str() const;
    QString name() const;

//    inline RemoteSong& operator=(const pb::remote::SongMetadata &m);
} RemoteSong;


QString RemoteSong::str() const
{
    return QString("#%1 %2 (title: %3, length: %4 (%5), size: %6, index: %7)").arg(
                id).arg(filename).arg(title).arg(pretty_length).arg(length).arg(file_size).arg(index);
}

/*
RemoteSong &RemoteSong::operator=(const pb::remote::SongMetadata &m)
{
    id = m.id();
    index = m.index();
    title = m.title().c_str();
    album = m.album().c_str();
    artist = m.artist().c_str();
    albumartist = m.albumartist().c_str();
    track = m.track();
    disc = m.disc();
    pretty_year = m.pretty_year().c_str();
    genre = m.genre().c_str();
    playcount = m.playcount();
    pretty_length = m.pretty_length().c_str();
    length = m.length();
    is_local = m.is_local();
    filename = m.filename().c_str();
    file_size = m.file_size();
    rating = m.rating();
    url = m.url().c_str();
    art_automatic = m.art_automatic().c_str();
    art_manual = m.art_manual().c_str();

    return *this;
}
*/

#endif // REMOTESONG_H
