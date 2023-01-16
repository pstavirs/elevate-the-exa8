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

// FIXME: remove #pragma
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

ConfigAgent::ConfigAgent(int numPorts)
{
    for (int i = 0; i < numPorts; i++) {
        ports_.append(new ConfigPort(i));
    }
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
    response->set_status(OstProto::Ack::kRpcSuccess);
    controller->SetFailed("Packet Transmit is not supported in the web demo - please download the Ostinato Trial for all features");
}

void ConfigAgent::stopTransmit(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::PortIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    response->set_status(OstProto::Ack::kRpcSuccess);
    controller->SetFailed("Packet Transmit is not supported in the web demo - please download the Ostinato Trial for all features");
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
    for (int i = 0; i < request->port_id_size(); i++) {
        int id = request->port_id(i).id();
        if (id < ports_.size()) {
            OstProto::PortStats *s = response->add_port_stats();
            s->mutable_port_id()->set_id(id);
            s->mutable_state()->set_link_state(OstProto::LinkStateDown);
        }
    }
}

void ConfigAgent::clearStats(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::PortIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    response->set_status(OstProto::Ack::kRpcSuccess);
    controller->SetFailed("Pending implementation");
}


void ConfigAgent::getStreamStats(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::StreamGuidList* request,
    ::OstProto::StreamStatsList* response,
    ::google::protobuf::Closure* done)
{
    controller->SetFailed("Pending implementation");
}

void ConfigAgent::clearStreamStats(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::StreamGuidList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    response->set_status(OstProto::Ack::kRpcSuccess);
    controller->SetFailed("Pending implementation");
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
    response->set_status(OstProto::Ack::kRpcSuccess);
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
    controller->SetFailed("Get Neighbors - ARP/NDP resolution is not supported in the web demo - please download the Ostinato Trial for all features");
}

#pragma GCC diagnostic pop
