TEMPLATE = lib
CONFIG += qt staticlib
# websockets required for wasm, but otherwise
QT += network websockets
DEFINES += HAVE_REMOTE
LIBS += -lprotobuf
HEADERS += rpcserver.h rpcconn.h pbrpccontroller.h pbrpcchannel.h pbqtio.h
SOURCES += rpcserver.cpp rpcconn.cpp pbrpcchannel.cpp

include (../options.pri)
