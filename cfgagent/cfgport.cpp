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

#include "cfgport.h"

#include <QMap>

#include <limits.h>

// XXX: Tx/Rx/Stats notes
// In this demo, no actual packets are crafted or sent out. Instead startTx
// only updates the port stats appropriately to give an illusion that packets
// are being transmitted. If a port has a back-to-back peer port configured,
// we update the peer's rx counters by the same as our Tx.
// The stats are updated by a 1s periodic stats timer which checks to see if
// transmit state is ON before incrementing the stats. Instead of one timer
// per port, cfgAgent runs a single timer for all ports as an optimization.
// The stats are updated assuming we are sending at the configured rates.
// While attempts have been made to make sure the stats are incremented
// correctly for all cases, this has not been verified thoroughly.
// Here are the cases that is checked -
//   * auto Tx stop after fixed no. of pkts sent
//   * startTx after auto stop
//   * stopTx before auto stop
//   * startTx after explicit stopTx
//   * goto first
//   * 0 rate (top speed)
//   * pps/bps rates
//   * rx == tx between peers for all the above cases
ConfigPort::ConfigPort(int id)
{
    data_.mutable_port_id()->set_id(id);
    data_.set_name(QString("eth%1").arg(id).toStdString());
    data_.set_is_enabled(true);
    // TODO: speed 'n mtu

    stats_.mutable_port_id()->CopyFrom(data_.port_id());
    stats_.mutable_state()->set_link_state(OstProto::LinkStateUp);
}

int ConfigPort::buildPacketList()
{
    // FIXME: FrameValueAttrib packetListAttrib;

    // First sort the streams by ordinalValue
    QMap<quint32, OstProto::Stream*> streamList;
    for (OstProto::Stream *s : streams_.values())
        streamList[s->core().ordinal()] = s;

    clearPacketList();

    // TODO: support multiple streams
    activeStreamCount_ = 0;
    for (OstProto::Stream *s : streamList) {
        if (s->core().is_enabled())
            activeStreamCount_++;
    }

    if (activeStreamCount_ > 1) {
        return 0;
    }

    for (OstProto::Stream *s : streamList) {
        if (!s->core().is_enabled())
            continue;

        quint32 numPackets;
        double packetsPerSec;
        if (s->control().unit() == OstProto::StreamControl::e_su_bursts) {
            numPackets = s->control().num_bursts()
                            * s->control().packets_per_burst();
            packetsPerSec = s->control().bursts_per_sec()
                                * s->control().packets_per_burst();
        } else {
            numPackets = s->control().num_packets();
            packetsPerSec = s->control().packets_per_sec();
        }

        // goto first - not really infinite loop, but large enough hopefully
        if (s->control().next() == OstProto::StreamControl::e_nw_goto_id)
            txPkts_.cfg.numPackets = ULLONG_MAX;
        else
            txPkts_.cfg.numPackets = numPackets;

        // Hard code max pps as 1000Mbps
        txPkts_.cfg.packetsPerSec = packetsPerSec;
        if (txPkts_.cfg.packetsPerSec == 0)
            txPkts_.cfg.packetsPerSec = 1e9/((s->core().frame_len()+20)*8);

        // Use frame length less 4 bytes of CRC similar to drone
        txPkts_.cfg.bytesPerSec = txPkts_.cfg.packetsPerSec
                                        * (s->core().frame_len() - 4);

        // Down counter to find out when we are done
        txPkts_.current.numPackets = txPkts_.cfg.numPackets;
    }
    return 0;
}

void ConfigPort::clearPacketList()
{
    txPkts_.cfg.numPackets = 0;
    txPkts_.cfg.packetsPerSec = 0;
    txPkts_.cfg.bytesPerSec = 0;

    txPkts_.current.numPackets = 0;
}

void ConfigPort::startTransmit()
{
    qDebug("setting port %u transmit ON", data_.port_id().id());
    stats_.mutable_state()->set_is_transmit_on(true);

    // Reset current numPackets so that we restart Tx
    txPkts_.current.numPackets = txPkts_.cfg.numPackets;
}

void ConfigPort::stopTransmit()
{
    qDebug("setting port %u transmit OFF", data_.port_id().id());
    stats_.mutable_state()->set_is_transmit_on(false);

    stats_.set_tx_pps(0);
    stats_.set_tx_bps(0);

    if (peerPort_) {
        peerPort_->stats_.set_rx_pps(0);
        peerPort_->stats_.set_rx_bps(0);
    }
}

void ConfigPort::updateStats()
{
    quint64 pkts = 0;

    // Packets left to Tx?
    if (txPkts_.current.numPackets) {
        pkts = qMin(txPkts_.current.numPackets, txPkts_.cfg.packetsPerSec);
        stats_.set_tx_pkts(stats_.tx_pkts() + pkts);
        stats_.set_tx_bytes(stats_.tx_bytes() + txPkts_.cfg.bytesPerSec);

        txPkts_.current.numPackets -= pkts;
    }

    // More packets left to Tx?
    if (txPkts_.current.numPackets) {
        stats_.set_tx_pps(pkts);
        stats_.set_tx_bps(txPkts_.cfg.bytesPerSec);
    } else {
        stopTransmit();
    }

    // Update peer port Rx stats, if we updated Tx stats
    if (peerPort_) {
        if (pkts) {
            peerPort_->stats_.set_rx_pkts(peerPort_->stats_.rx_pkts() + pkts);
            peerPort_->stats_.set_rx_bytes(peerPort_->stats_.rx_bytes() +
                                                txPkts_.cfg.bytesPerSec);
        }

        if (txPkts_.current.numPackets) {
            peerPort_->stats_.set_rx_pps(pkts);
            peerPort_->stats_.set_rx_bps(txPkts_.cfg.bytesPerSec);
        } else {
            peerPort_->stats_.set_rx_pps(0);
            peerPort_->stats_.set_rx_bps(0);
        }
    }
}

void ConfigPort::clearStats()
{
    // Clear only the stats that the web demo updates
    stats_.set_rx_pkts(0);
    stats_.set_rx_bytes(0);
    stats_.set_rx_pps(0);
    stats_.set_rx_bps(0);

    stats_.set_tx_pkts(0);
    stats_.set_tx_bytes(0);
    stats_.set_tx_pps(0);
    stats_.set_tx_bps(0);
}
