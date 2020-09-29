TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -g -fPIC -Wextra  -finline-functions -O0 -fno-strict-aliasing -fvisibility=hidden -DAMD64
QMAKE_CXXFLAGS += -DLINUX

QMAKE_CXXFLAGS += -DUDT_SERVER
#QMAKE_CXXFLAGS += -DUDT_CLIENT

LIBS += -lm -lpthread

INCLUDEPATH += $$PWD/include
INCLUDEPATH += $$PWD/udt
INCLUDEPATH += $$PWD/zlink

HEADERS += \
    include/ZLink.h \
    udt/api.h \
    udt/buffer.h \
    udt/cache.h \
    udt/ccc.h \
    udt/channel.h \
    udt/common.h \
    udt/core.h \
    udt/epoll.h \
    udt/list.h \
    udt/md5.h \
    udt/packet.h \
    udt/queue.h \
    udt/udt.h \
    udt/window.h \
    zlink/utils.h \
    zlink/zlink_inner.h \
    test/demo.h

SOURCES += \
    udt/api.cpp \
    udt/buffer.cpp \
    udt/cache.cpp \
    udt/ccc.cpp \
    udt/channel.cpp \
    udt/common.cpp \
    udt/core.cpp \
    udt/epoll.cpp \
    udt/list.cpp \
    udt/md5.cpp \
    udt/packet.cpp \
    udt/queue.cpp \
    udt/window.cpp \
    zlink/utils.cpp \
    zlink/ZLink.cpp \
    zlink/zlink_inner.cpp \
    test/demo.cpp \
    test/main.cpp \
    test/test_client.cpp \
    test/test_server.cpp

DISTFILES +=
