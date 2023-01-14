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

#include "syncchannel.h"

#include "../cfgagent/cfgagent.h"

// FIXME: rename to SyncChannel
SyncChannel::SyncChannel(QString /*serverName*/, quint16 /*port*/,
                           const ::google::protobuf::Message &notifProto)
    : notifPrototype(notifProto)
{
    cfgAgent_ = new ConfigAgent();
    // FIXME: recheck if queued connection is required
    connect(this, &SyncChannel::rpcDone,
            this, &SyncChannel::processRpcReply,
            Qt::QueuedConnection);
}

SyncChannel::~SyncChannel()
{
    delete cfgAgent_;
}

void SyncChannel::establish()
{
    qDebug("In %s", __FUNCTION__);

    emit stateChanged(QAbstractSocket::ConnectedState);
    emit connected();
}

void SyncChannel::establish(QString /*serverName*/, quint16 /*port*/)
{
    establish();
}

void SyncChannel::tearDown()
{
    qDebug("In %s", __FUNCTION__);

    emit stateChanged(QAbstractSocket::UnconnectedState);
    emit disconnected();
}

void SyncChannel::CallMethod(
    const ::google::protobuf::MethodDescriptor *method,
    ::google::protobuf::RpcController *controller,
    const ::google::protobuf::Message *req,
    ::google::protobuf::Message *response,
    ::google::protobuf::Closure* done)
{
    if (!req->IsInitialized()) {
        qWarning("SyncChannel: missing required fields in request <----");
        qDebug("req = %s\n%s", method->input_type()->name().c_str(),
                req->DebugString().c_str());
        qDebug("error = \n%s\n--->", req->InitializationErrorString().c_str());

        controller->SetFailed("Required fields missing");
        done->Run();
        return;
    }

    // Avoid printing stats since it happens every couple of seconds
    if (method->index() != 13) {
        qDebug("method = %d:%s\n req = %s\n%s\n---->",
                method->index(), method->name().c_str(),
                method->input_type()->name().c_str(),
                req->DebugString().c_str());
    }

    cfgAgent_->CallMethod(method, controller, req, response, done);
    // FIXME: handle other message types than response
    if (!response->IsInitialized()) {
        qWarning("SyncChannel: missing required fields in response <----");
        qDebug("resp = %s\n%s",
                method->output_type()->name().c_str(),
                response->DebugString().c_str());
        qDebug("error = \n%s\n--->",
                response->InitializationErrorString().c_str());

        controller->SetFailed(method->name() + '-' +
                        method->output_type()->name()
                        + " Response required fields missing");
    }
    if (method->index() != 13) {
        qDebug("method = %d:%s\nresp = %s\n%s---->",
                method->index(), method ? method->name().c_str() : "",
                method ? method->output_type()->name().c_str() : "",
                response->DebugString().c_str());
    }

    emit rpcDone(done);
}

void SyncChannel::processRpcReply(::google::protobuf::Closure *done)
{
    done->Run();
}

