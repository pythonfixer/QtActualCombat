#-------------------------------------------------
#
# Project created by QtCreator 2022-06-01T09:36:21
#
#-------------------------------------------------

QT       += core gui
QT += network
TARGET = chat
TEMPLATE = app


SOURCES += main.cpp\
        widget.cpp \
    tcpserver.cpp \
    tcpclient.cpp

HEADERS  += widget.h \
    tcpserver.h \
    tcpclient.h

FORMS    += widget.ui \
    tcpserver.ui \
    tcpclient.ui

RESOURCES += \
    images.qrc
