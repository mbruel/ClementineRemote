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

#ifndef DOWNLOADER_H
#define DOWNLOADER_H
#include "Macro.h"
#include "player/RemoteSong.h"
#include <QString>
#include <QMap>
class QFile;

struct Downloader {
    qint32 chunkNumber;
    qint32 chunkCount;
    qint32 fileNumber;
    qint32 fileSize;

    QString downloadPath;
    QFile  *file;
    bool    canWrite;

    int dowloadedSize;

    Downloader();
    virtual ~Downloader();

    void init();

    Downloader(const Downloader&) = delete;
    Downloader(Downloader&&) = delete;
    Downloader &operator=(const Downloader&) = delete;
    Downloader &operator=(Downloader&&) = delete;

};

struct SongsDownloader : public Downloader
{
    qint32 nbFiles;
    qint32 totalSize;

    int downloadedFiles;

    AtomicBool cancel;
    bool hasCancelError;

    RemoteSong song;

    QMap<int, QString> errorByFileNum;


    SongsDownloader();

    void init(qint32 nbFiles_, qint32 totalSize_);

    ~SongsDownloader() = default;
    SongsDownloader(const Downloader&) = delete;
    SongsDownloader(Downloader&&) = delete;
    SongsDownloader &operator=(const Downloader&) = delete;
    SongsDownloader &operator=(Downloader&&) = delete;

    inline void cancelDownload();
    inline bool isCancelled();

    inline void addError(const QString &err);
};

void SongsDownloader::cancelDownload() { cancel = 0x1; }
bool SongsDownloader::isCancelled() { return M_LoadAtomic(cancel); }
void SongsDownloader::addError(const QString &err)
{
    errorByFileNum[fileNumber] = QString("[%1 / %2] %3").arg(
                fileNumber).arg(nbFiles).arg(err);
}

#endif // DOWNLOADER_H
