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

#ifndef _SYNC_RPC_CHANNEL_H
#define _SYNC_RPC_CHANNEL_H

#include <QString>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>

#include "pbrpccommon.h"
#include "pbrpccontroller.h"

#include <QAbstractSocket>
#include <QObject>

class SyncChannel : public QObject, public ::google::protobuf::RpcChannel
{
    Q_OBJECT
public:
    SyncChannel(QString serverName, quint16 port,
                 const ::google::protobuf::Message &notifProto);
    ~SyncChannel();

    void establish();
    void establish(QString serverName, quint16 port);
    void tearDown();

    const QString serverName() const { return QString("web-demo"); }
    quint16 serverPort() const { return 0; }

    QAbstractSocket::SocketState state() const
        { return QAbstractSocket::ConnectedState; }

    void CallMethod(const ::google::protobuf::MethodDescriptor *method,
        ::google::protobuf::RpcController *controller,
        const ::google::protobuf::Message *req,
        ::google::protobuf::Message *response,
        ::google::protobuf::Closure* done);

signals:
    void connected();
    void disconnected();
    void error(QAbstractSocket::SocketError socketError);
    void stateChanged(QAbstractSocket::SocketState socketState);

    void notification(int notifType, ::google::protobuf::Message *notifData);

    // FIXME: use QPrivateSignal
    void rpcDone(::google::protobuf::Closure *done);

private slots:
    void processRpcReply(::google::protobuf::Closure *done);

private:
    ::google::protobuf::Service *cfgAgent_;
    const ::google::protobuf::Message &notifPrototype;
#if 0 // FIXME: needed?
    ::google::protobuf::Message *notif;
#endif
};

#endif
