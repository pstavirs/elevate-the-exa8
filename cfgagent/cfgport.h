/*
Copyright (C) 2023 Srivats P.

This file is part of "Ostinato"

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#ifndef _CFG_PORT_H
#define _CFG_PORT_H

#include <QObject>

#include "../common/protocol.pb.h"

#include <QHash>

struct ConfigPort : public QObject
{
    ConfigPort(int id);
    int id() { return data_.port_id().id(); }
    bool isTransmitOn() { return stats_.state().is_transmit_on(); }

    int buildPacketList();
    void clearPacketList();

    void startTransmit();
    void stopTransmit();

    void updateStats();
    void clearStats();

    OstProto::Port data_;
    OstProto::PortStats stats_;
    QHash<quint32, OstProto::Stream*> streams_;
    QHash<quint32, OstProto::DeviceGroup*> deviceGroups_;
    quint32 activeStreamCount_{0};
    struct TxPkts {
        struct {
            quint64 numPackets{0};
            quint64 packetsPerSec{0};
            quint64 bytesPerSec{0};
        } cfg;
        struct {
            quint64 numPackets{0};
        } current;
    } txPkts_;
};

#endif

