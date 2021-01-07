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

#include "RemoteSong.h"

bool RemoteSong::sDispArtistInName = true;

QString RemoteSong::name() const
{
    QString n(title);
    if (track != -1)
        n.prepend(QString("%1-").arg(track));
    if (sDispArtistInName && !artist.isEmpty())
        n.prepend(QString("%1: ").arg(artist));
    return n;
}
