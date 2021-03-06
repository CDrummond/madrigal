#ifndef UPNP_PLAY_COMMAND_H
#define UPNP_PLAY_COMMAND_H

#include "upnp/device.h"
namespace Upnp {
struct Command {
    enum Type {
        None,
        Append,
        ReplaceAndPlay,
        Insert,
        Move
    };
    Command() : pos(0), type(None) { }
    virtual ~Command() { qDeleteAll(tracks); }
    QList<const Device::MusicTrack *> tracks;
    qint32 pos;
    Type type;
};

}

#endif
