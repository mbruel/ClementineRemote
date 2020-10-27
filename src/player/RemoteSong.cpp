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

#include "RemoteSong.h"
RemoteSong::RemoteSong(const pb::remote::SongMetadata &m):
    id(m.id()), index(m.index()), title(m.title().c_str()), album(m.album().c_str()),
    artist(m.artist().c_str()), albumartist(m.albumartist().c_str()), track(m.track()),
    disc(m.disc()), pretty_year(m.pretty_year().c_str()), genre(m.genre().c_str()),
    playcount(m.playcount()), pretty_length(m.pretty_length().c_str()), length(m.length()),
    is_local(m.is_local()), filename(m.filename().c_str()), file_size(m.file_size()), rating(m.rating()),
    url(m.url().c_str()), art_automatic(m.art_automatic().c_str()), art_manual(m.art_manual().c_str()),
    type(m.type()), art()
{
    if (m.has_art() && m.art().size())
        art = QImage(m.art().c_str());
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
