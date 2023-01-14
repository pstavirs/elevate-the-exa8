TEMPLATE = lib
CONFIG += qt staticlib
LIBS += -lprotobuf
HEADERS += \
    cfgagent.h \
    cfgport.h \

SOURCES += \
    cfgagent.cpp \
    cfgport.cpp \

#include (../options.pri)
