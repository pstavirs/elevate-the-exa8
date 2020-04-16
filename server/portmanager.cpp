/*
Copyright (C) 2010-2012 Srivats P.

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

#include "portmanager.h"

#include "bsdport.h"
#include "interfaceinfo.h"
#include "linuxport.h"
#include "pcapport.h"
#include "settings.h"
#include "winpcapport.h"

#include <QHostAddress>
#include <QStringList>

PortManager *PortManager::instance_ = NULL;

#if defined(Q_OS_WIN32)
#include <QUuid>
#include <ipHlpApi.h>
// Define the function prototypes since they are not defined in ipHlpApi.h
NETIO_STATUS WINAPI ConvertInterfaceGuidToLuid(
        const GUID *InterfaceGuid, PNET_LUID InterfaceLuid);
NETIO_STATUS WINAPI ConvertInterfaceLuidToAlias(
        const NET_LUID *InterfaceLuid, PWSTR InterfaceAlias, SIZE_T Length);

#define MyGetProcAddress(hDll, function) \
    hDll ? reinterpret_cast<decltype(&function)> (GetProcAddress(hDll, #function)) : NULL;
#endif

PortManager::PortManager()
{
    int i;
    AbstractPort::Accuracy txRateAccuracy;

    qDebug("PCAP Lib: %s", pcap_lib_version());
    qDebug("Retrieving the device list from the local machine\n"); 

#if defined(Q_OS_WIN32)
    WinPcapPort::fetchHostNetworkInfo();
#elif defined(Q_OS_LINUX)
    LinuxPort::fetchHostNetworkInfo();
#elif defined(Q_OS_BSD4)
    BsdPort::fetchHostNetworkInfo();
#endif

    txRateAccuracy = rateAccuracy();

    InterfaceList *interfaceList = GetInterfaceList();
    for (i = 0; i < interfaceList->size(); i++)
    {
        const Interface &intf = interfaceList->at(i);
        AbstractPort *port;
      
        qDebug("%d. %s (%s) [%s]\n", i,
                intf.name(),
                intf.description(),
                intf.alias());

#if defined(Q_OS_WIN32)
        port = new WinPcapPort(i, intf.name(), intf.description());
#elif defined(Q_OS_LINUX)
        port = new LinuxPort(i, intf.name());
#elif defined(Q_OS_BSD4)
        port = new BsdPort(i, intf.name());
#else
        port = new PcapPort(i, intf.name());
#endif

        if (!port->isUsable())
        {
            qDebug("%s: unable to open %s. Skipping!", __FUNCTION__,
                    intf.name());
            delete port;
            i--;
            continue;
        }

        port->setAlias(intf.alias());

        const InterfaceInfo *intfInfo = port->interfaceInfo();
        if (intfInfo) {
            qDebug("Mac: %012llx", intfInfo->mac);
            foreach(Ip4Config ip, intfInfo->ip4)
                qDebug("Ip4: %s/%d gw: %s",
                        qPrintable(QHostAddress(ip.address).toString()),
                        ip.prefixLength,
                        qPrintable(QHostAddress(ip.gateway).toString()));
            foreach(Ip6Config ip, intfInfo->ip6)
                qDebug("Ip6: %s/%d gw: %s",
                        qPrintable(QHostAddress(ip.address.toArray()).toString()),
                        ip.prefixLength,
                        qPrintable(QHostAddress(ip.gateway.toArray()).toString()));
        }

        if (!port->setRateAccuracy(txRateAccuracy))
            qWarning("failed to set rateAccuracy (%d)", txRateAccuracy);

        portList_.append(port);
    }

    FreeInterfaceList(interfaceList);

    foreach(AbstractPort *port, portList_)
        port->init();
    
#if defined(Q_OS_WIN32)
    WinPcapPort::freeHostNetworkInfo();
#elif defined(Q_OS_LINUX)
    LinuxPort::freeHostNetworkInfo();
#elif defined(Q_OS_BSD4)
    BsdPort::freeHostNetworkInfo();
#endif

    return;
}

PortManager::~PortManager()
{
    while (!portList_.isEmpty())
        delete portList_.takeFirst();
}

PortManager* PortManager::instance()
{
    if (!instance_)
        instance_ = new PortManager;

    return instance_;       
}

PortManager::InterfaceList* PortManager::GetInterfaceList()
{
    pcap_if_t *deviceList = NULL;
    char errbuf[PCAP_ERRBUF_SIZE];
    InterfaceList *interfaceList = new InterfaceList();

    if (pcap_findalldevs(&deviceList, errbuf) == -1)
        qDebug("Error in pcap_findalldevs_ex: %s\n", errbuf);

#if defined(Q_OS_WIN32)
    // Use windows' connection name as the description for a better UX
    HMODULE ipHlpApi = LoadLibrary(TEXT("ipHlpApi.dll"));
    auto guid2Luid = MyGetProcAddress(ipHlpApi, ConvertInterfaceGuidToLuid);
    auto luid2Alias = MyGetProcAddress(ipHlpApi, ConvertInterfaceLuidToAlias);
#endif

    // Read include/exclude port lists
    QRegExp pattern;
    QStringList includeList = appSettings->value(kPortListIncludeKey)
                                    .toStringList();
    QStringList excludeList = appSettings->value(kPortListExcludeKey)
                                    .toStringList();

    // NOTE: A blank "IncludeList=" is read as a stringlist with one
    // string which is empty => treat it same as an empty stringlist
    if (includeList.size() == 1 && includeList.at(0).isEmpty())
        includeList.clear();

    pattern.setPatternSyntax(QRegExp::Wildcard);

    // Read port alias list
    QMap<QByteArray, QByteArray> aliasMap;
    QStringList aliasList = appSettings->value(kPortListAliasKey)
                                    .toStringList();
    foreach (QString aliasSpec, aliasList) {
        QStringList nameAlias = aliasSpec.split(':');
        if (nameAlias.size() >= 2)
            aliasMap.insert(nameAlias.at(0).toLatin1(),
                            nameAlias.at(1).toLatin1());
    }

    for(pcap_if_t *device = deviceList; device != NULL; device = device->next) {

        Interface intf;
        intf.name_ = QByteArray(device->name);
        intf.description_ = QByteArray(device->description);
#if defined(Q_OS_WIN32)
        // Use windows' connection name as the description for a better UX
        if (guid2Luid && luid2Alias) {
            GUID guid = static_cast<GUID>(QUuid(
                            QString(intf.name()).remove("\\Device\\NPF_")));
            NET_LUID luid;
            if (guid2Luid(&guid, &luid) == NO_ERROR) {
                WCHAR conn[256];
                if (luid2Alias(&luid, conn, 256) == NO_ERROR)
                    intf.description_ = QString::fromWCharArray(conn)
                        .toLocal8Bit();
            }
        }
#endif
        intf.alias_ = aliasMap.value(intf.eponym());

        // Run intf through filter rules - include/exclude
        bool pass = false;

        // An empty (or missing) includeList accepts all ports
        if (includeList.isEmpty())
            pass = true;
        else {
            foreach (QString str, includeList) {
                pattern.setPattern(str);
                if (pattern.exactMatch(intf.eponym())) {
                    pass = true;
                    break;
                }
            }
        }

        // If include list passes, check against exclude list
        if (pass) {
            foreach (QString str, excludeList) {
                pattern.setPattern(str);
                if (pattern.exactMatch(intf.eponym())) {
                    pass = false;
                    break;
                }
            }
        }

        if (pass)
            interfaceList->append(intf);
        else
            qDebug("%s (%s) rejected by filter. Skipping!",
                    intf.name(), intf.description());
    }

    std::sort(interfaceList->begin(), interfaceList->end());

#if defined(Q_OS_WIN32)
    if (ipHlpApi)
        FreeLibrary(ipHlpApi);
#endif
    pcap_freealldevs(deviceList);
    return interfaceList;
}

void PortManager::FreeInterfaceList(InterfaceList *interfaceList)
{
    free(interfaceList);
}

AbstractPort::Accuracy PortManager::rateAccuracy()
{
    QString rateAccuracy = appSettings->value(kRateAccuracyKey, 
                                  kRateAccuracyDefaultValue).toString();
    if (rateAccuracy == "High")
        return AbstractPort::kHighAccuracy;
    else if (rateAccuracy == "Low")
        return AbstractPort::kLowAccuracy;
    else
        qWarning("Unsupported RateAccuracy setting - %s", 
                 qPrintable(rateAccuracy));

    return AbstractPort::kHighAccuracy;
}
