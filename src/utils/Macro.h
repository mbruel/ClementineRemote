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

#ifndef MACROS_H
#define MACROS_H
#include <QtGlobal>

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    #define M_FLUSH flush
#else
    #define M_FLUSH Qt::flush
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    #define M_LoadAtomic(atom) atom.load()
#else
    #define M_LoadAtomic(atom) atom.loadRelaxed()
#endif

using AtomicBool = QAtomicInteger<unsigned short>; // 16 bit only (faster than using 8 bit variable...)

#endif // MACROS_H
