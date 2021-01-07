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

#include "Downloader.h"
#include <QFile>

Downloader::Downloader():
    chunkNumber(0), chunkCount(0),
    fileNumber(0), fileSize(0),
    downloadPath(), file(nullptr), canWrite(false),
    dowloadedSize(0)
{}

Downloader::~Downloader()
{
    if (file)
    {
        file->remove();
        delete file;
    }
}

void Downloader::init()
{
    chunkNumber     = 0;
    chunkCount      = 0;
    fileNumber      = 0;
    fileSize        = 0;
    if (file){
        file->remove();
        delete file;
        file = nullptr;
    }
    canWrite = false;
    dowloadedSize = 0;
}




SongsDownloader::SongsDownloader(): Downloader(),
    nbFiles(0), totalSize(0),
    downloadedFiles(0),
    cancel(0x0), hasCancelError(false),
     song(), errorByFileNum()
{}

void SongsDownloader::init(qint32 nbFiles_, qint32 totalSize_)
{
    Downloader::init();
    nbFiles         = nbFiles_;
    totalSize       = totalSize_;
    song            = RemoteSong();
    downloadedFiles = 0;
    errorByFileNum.clear();
    cancel = 0x0;
    hasCancelError = false;
}
