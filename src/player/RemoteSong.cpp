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
