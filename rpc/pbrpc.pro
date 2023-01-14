TEMPLATE = lib
CONFIG += qt staticlib
QT += network
wasm:QT += websockets
DEFINES += HAVE_REMOTE
LIBS += -lprotobuf
HEADERS += rpcserver.h rpcconn.h pbrpccontroller.h pbrpcchannel.h pbqtio.h syncchannel.h
SOURCES += rpcserver.cpp rpcconn.cpp pbrpcchannel.cpp syncchannel.cpp

include (../options.pri)
