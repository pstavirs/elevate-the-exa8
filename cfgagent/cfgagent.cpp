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

#include "cfgagent.h"

#include "cfgport.h"
#include "../common/framevalueattrib.h"

// FIXME: remove #pragma
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

ConfigAgent::ConfigAgent(int numPorts)
{
    for (int i = 0; i < numPorts; i++) {
        ports_.append(new ConfigPort(i));
    }

    for (int i = 0; i < numPorts; i++) {
        ports_[i]->setPeer(peerPort(i));
    }

    statsTimer_.setInterval(1000); // 1sec periodic
    connect(&statsTimer_, SIGNAL(timeout()),
            this, SLOT(updateStats()));
}

ConfigAgent::~ConfigAgent()
{
    while (!ports_.isEmpty())
        delete ports_.takeFirst();
}

/* Methods provided by the service */
void ConfigAgent::getPortIdList(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::Void* request,
    ::OstProto::PortIdList* response,
    ::google::protobuf::Closure* done)
{
    for (int i = 0; i < ports_.size(); i++) {
        OstProto::PortId *p = response->add_port_id();
        p->set_id(ports_.at(i)->id());
    }
}

void ConfigAgent::getPortConfig(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::PortIdList* request,
    ::OstProto::PortConfigList* response,
    ::google::protobuf::Closure* done)
{
    for (int i = 0; i < request->port_id_size(); i++) {
        int id = request->port_id(i).id();
        if (id < ports_.size()) {
            OstProto::Port *p = response->add_port();
            //ports_[id]->protoDataCopyInto(p);
            p->CopyFrom(ports_[id]->data_);
        }
    }
}

void ConfigAgent::modifyPort(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::PortConfigList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    // TODO
    // FIXME: interleaved streams mode not supported in web demo
    controller->SetFailed("Pending implementation");
}

void ConfigAgent::getStreamIdList(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::PortId* request,
    ::OstProto::StreamIdList* response,
    ::google::protobuf::Closure* done)
{
    quint32 portId = request->id();
    response->mutable_port_id()->set_id(ports_[portId]->id());
    for (OstProto::Stream *s: ports_[portId]->streams_) {
        response->add_stream_id()->CopyFrom(s->stream_id());
    }
}

void ConfigAgent::getStreamConfig(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::StreamIdList* request,
    ::OstProto::StreamConfigList* response,
    ::google::protobuf::Closure* done)
{
    quint32 portId = request->port_id().id();
    response->mutable_port_id()->set_id(ports_[portId]->id());
    for (int i = 0; i < request->stream_id_size(); i++) {
        quint32 streamId = request->stream_id(i).id();
        if (ports_[portId]->streams_.contains(streamId)) {
            OstProto::Stream *s = response->add_stream();
            s->CopyFrom(*ports_[portId]->streams_.value(streamId));
        }
    }
}

void ConfigAgent::addStream(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::StreamIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    quint32 portId = request->port_id().id();
    for (int i = 0; i < request->stream_id_size(); i++) {
        quint32 streamId = request->stream_id(i).id();
        ports_[portId]->streams_.insert(streamId, new OstProto::Stream);
    }
    response->set_status(OstProto::Ack::kRpcSuccess);
}

void ConfigAgent::deleteStream(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::StreamIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    quint32 portId = request->port_id().id();
    for (int i = 0; i < request->stream_id_size(); i++) {
        quint32 streamId = request->stream_id(i).id();
        ports_[portId]->streams_.remove(streamId);
    }
    response->set_status(OstProto::Ack::kRpcSuccess);
}

void ConfigAgent::modifyStream(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::StreamConfigList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    quint32 portId = request->port_id().id();
    for (int i = 0; i < request->stream_size(); i++) {
        quint32 streamId = request->stream(i).stream_id().id();
        if (ports_[portId]->streams_.contains(streamId)) {
            ports_[portId]->streams_[streamId]->CopyFrom(request->stream(i));
        }
    }
    response->set_status(OstProto::Ack::kRpcSuccess);
}

void ConfigAgent::startTransmit(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::PortIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    bool multiStreams = false;
    for (int i = 0; i < request->port_id_size(); i++) {
        int id = request->port_id(i).id();
        if (id < ports_.size()) {
            ports_[id]->startTransmit();
            if (ports_[id]->activeStreamCount_ > 1)
                multiStreams = true;
        }
    }

    // At least one port is transmitting, so start timer if required
    if (!statsTimer_.isActive())
        statsTimer_.start();

    if (multiStreams) {
        response->set_status(OstProto::Ack::kRpcSuccess);
        controller->SetFailed("Packet transmit is not supported in the web demo on ports with more than one stream - please enable only one stream or download the Ostinato Trial for multi-stream transmit");
    } else {
        response->set_status(OstProto::Ack::kRpcSuccess);
        controller->SetFailed("Port Stats is incomplete and inaccurate in the web demo - please download the Ostinato Trial for full and accurate stats");
    }
}

void ConfigAgent::stopTransmit(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::PortIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    for (int i = 0; i < request->port_id_size(); i++) {
        int id = request->port_id(i).id();
        if (id < ports_.size())
            ports_[id]->stopTransmit();
    }

    // If no port is transmitting, stop timer
    if (statsTimer_.isActive()) {
        bool anyTransmitOn = false;
        for (int i = 0; i < ports_.size(); i++)
            anyTransmitOn |= ports_.at(i)->isTransmitOn();
        if (!anyTransmitOn)
            statsTimer_.stop();
    }
    response->set_status(OstProto::Ack::kRpcSuccess);
}

void ConfigAgent::startCapture(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::PortIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    response->set_status(OstProto::Ack::kRpcSuccess);
    controller->SetFailed("Packet Capture is not supported in the web demo - please download the Ostinato Trial for all features");
}

void ConfigAgent::stopCapture(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::PortIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    response->set_status(OstProto::Ack::kRpcSuccess);
    controller->SetFailed("Packet Capture is not supported in the web demo - please download the Ostinato Trial for all features");
}

void ConfigAgent::getCaptureBuffer(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::PortId* request,
    ::OstProto::CaptureBuffer* response,
    ::google::protobuf::Closure* done)
{
    controller->SetFailed("Packet Capture is not supported in the web demo - please download the Ostinato Trial for all features");
}

void ConfigAgent::getStats(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::PortIdList* request,
    ::OstProto::PortStatsList* response,
    ::google::protobuf::Closure* done)
{
    // XXX: Ostinato GUI client does not alloc a new response message
    // for every getStats RPC (unlike for other RPCs), so clear previous
    // response first
    response->clear_port_stats();

    for (int i = 0; i < request->port_id_size(); i++) {
        int portId = request->port_id(i).id();
        if (portId < ports_.size()) {
            OstProto::PortStats *s = response->add_port_stats();
            s->CopyFrom(ports_[portId]->stats_);
        }
    }
}

void ConfigAgent::clearStats(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::PortIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    for (int i = 0; i < request->port_id_size(); i++) {
        int portId = request->port_id(i).id();
        if (portId < ports_.size()) {
            ports_[portId]->clearStats();
        }
    }
    response->set_status(OstProto::Ack::kRpcSuccess);
}


void ConfigAgent::getStreamStats(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::StreamGuidList* request,
    ::OstProto::StreamStatsList* response,
    ::google::protobuf::Closure* done)
{
    controller->SetFailed("Per Stream stats are not supported in the web demo - please download the Ostinato Trial for all features");
}

void ConfigAgent::clearStreamStats(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::StreamGuidList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    response->set_status(OstProto::Ack::kRpcSuccess);
    controller->SetFailed("Per Stream stats are not supported in the web demo - please download the Ostinato Trial for all features");
}


void ConfigAgent::checkVersion(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::VersionInfo* request,
    ::OstProto::VersionCompatibility* response,
    ::google::protobuf::Closure* done)
{
    // Config agent is **always** compatible
    response->set_result(OstProto::VersionCompatibility::kCompatible);
}


void ConfigAgent::build(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::BuildConfig* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    QString notes;
    int frameError = 0;
    int portId = request->port_id().id();
    response->set_status(OstProto::Ack::kRpcSuccess);

    if (ports_[portId]->isTransmitOn())
        goto _port_busy;

    frameError = ports_[portId]->buildPacketList();

    if (frameError) {
        notes += frameValueErrorNotes(portId, frameError);
        response->set_status(OstProto::Ack::kRpcError);
        response->set_notes(notes.toStdString());
    } else
        response->set_status(OstProto::Ack::kRpcSuccess);
    return;

_port_busy:
    controller->SetFailed(QString("Port %1 build: operation disallowed "                                  "on transmitting port")
                            .arg(portId).toStdString());
}


// DeviceGroup and Protocol Emulation
void ConfigAgent::getDeviceGroupIdList(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::PortId* request,
    ::OstProto::DeviceGroupIdList* response,
    ::google::protobuf::Closure* done)
{
    quint32 portId = request->id();
    response->mutable_port_id()->set_id(ports_[portId]->id());
    for (OstProto::DeviceGroup *dg: ports_[portId]->deviceGroups_) {
        response->add_device_group_id()->CopyFrom(dg->device_group_id());
    }
}

void ConfigAgent::getDeviceGroupConfig(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::DeviceGroupIdList* request,
    ::OstProto::DeviceGroupConfigList* response,
    ::google::protobuf::Closure* done)
{
    quint32 portId = request->port_id().id();
    response->mutable_port_id()->set_id(ports_[portId]->id());
    for (int i = 0; i < request->device_group_id_size(); i++) {
        quint32 dgId = request->device_group_id(i).id();
        if (ports_[portId]->deviceGroups_.contains(dgId)) {
            OstProto::DeviceGroup *dg = response->add_device_group();
            dg->CopyFrom(*ports_[portId]->deviceGroups_.value(dgId));
        }
    }
}

void ConfigAgent::addDeviceGroup(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::DeviceGroupIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    quint32 portId = request->port_id().id();
    for (int i = 0; i < request->device_group_id_size(); i++) {
        quint32 dgId = request->device_group_id(i).id();
        ports_[portId]->deviceGroups_.insert(dgId, new OstProto::DeviceGroup);
    }
    response->set_status(OstProto::Ack::kRpcSuccess);
}

void ConfigAgent::deleteDeviceGroup(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::DeviceGroupIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    quint32 portId = request->port_id().id();
    for (int i = 0; i < request->device_group_id_size(); i++) {
        quint32 dgId = request->device_group_id(i).id();
        ports_[portId]->deviceGroups_.remove(dgId);
    }
    response->set_status(OstProto::Ack::kRpcSuccess);
}

void ConfigAgent::modifyDeviceGroup(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::DeviceGroupConfigList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    quint32 portId = request->port_id().id();
    for (int i = 0; i < request->device_group_size(); i++) {
        quint32 dgId = request->device_group(i).device_group_id().id();
        if (ports_[portId]->deviceGroups_.contains(dgId)) {
            ports_[portId]->deviceGroups_[dgId]->CopyFrom(
                    request->device_group(i));
        }
    }
    response->set_status(OstProto::Ack::kRpcSuccess);
}

void ConfigAgent::getDeviceList(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::PortId* request,
    ::OstProto::PortDeviceList* response,
    ::google::protobuf::Closure* done)
{
    quint32 portId = request->id();
    response->mutable_port_id()->set_id(ports_[portId]->id());

    // For the web demo, don't return failure during the init phase
    // when the client is retrieving state and not explicitly
    // user initiated - this avoids an unnecessary error log in
    // the web demo GUI at startup
    if (!initPhase_)
        controller->SetFailed("Get Devices - Device Groups can only be configured, not emulated in the web demo - please download the Ostinato Trial for all features");
}


void ConfigAgent::resolveDeviceNeighbors(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::PortIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    response->set_status(OstProto::Ack::kRpcSuccess);
    controller->SetFailed("Resolve Neighbors - ARP/NDP resolution is not supported in the web demo - please download the Ostinato Trial for all features");
}

void ConfigAgent::clearDeviceNeighbors(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::PortIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    response->set_status(OstProto::Ack::kRpcSuccess);
    controller->SetFailed("Clear Neighbors - ARP/NDP resolution is not supported in the web demo - please download the Ostinato Trial for all features");
}

void ConfigAgent::getDeviceNeighbors(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::PortId* request,
    ::OstProto::PortNeighborList* response,
    ::google::protobuf::Closure* done)
{
    quint32 portId = request->id();
    response->mutable_port_id()->set_id(ports_[portId]->id());

    // See note for similar check in getDeviceList() above
    if (!initPhase_)
        controller->SetFailed("Get Neighbors - ARP/NDP resolution is not supported in the web demo - please download the Ostinato Trial for all features");

    // getDeviceNeighbors is the last RPC during init - if this is for
    // the last port, mark init as done
    if ((int)portId == (ports_.size() - 1))
        initPhase_ = false;
}

// ----- Private slots/methods ------
//
void ConfigAgent::updateStats()
{
    for (int i = 0; i < ports_.size(); i++) {
        if (ports_.at(i)->isTransmitOn())
            ports_.at(i)->updateStats();
    }
}

ConfigPort* ConfigAgent::peerPort(int portId)
{
    // Back to back connections: 0-1, 2-3, ...
    int peer = portId & 0x1 ? portId - 1 : portId + 1;

    return ports_[peer];
}

QString ConfigAgent::frameValueErrorNotes(int portId, int error)
{
    if (!error)
        return QString();

    QString pfx = QString("Port %1: ").arg(portId);
    auto errorFlags = static_cast<FrameValueAttrib::ErrorFlags>(error);

    // If smac resolve fails, dmac will always fail - so check that first
    // and report only that so as not to confuse users (they may not realize
    // that without a source device, we have no ARP table to lookup for dmac)

    if (errorFlags & FrameValueAttrib::UnresolvedSrcMacError)
        return pfx + "Source mac resolve failed for one or more "
                     "streams - Device matching stream's source IP not found\n";

    if (errorFlags & FrameValueAttrib::UnresolvedDstMacError)
        return pfx + "Destination mac resolve failed for one or more "
                     "streams - possible ARP/ND failure\n";

    return QString();
}

#pragma GCC diagnostic pop
