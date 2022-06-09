#-------------------------------------------------
#
# Project created by QtCreator 2022-05-30T09:21:41
#
#-------------------------------------------------

QT       += core gui
QT       += phonon
TARGET = myPlayer
TEMPLATE = app


SOURCES += main.cpp\
        mywidget.cpp \
    myplaylist.cpp \
    mylrc.cpp

HEADERS  += mywidget.h \
    myplaylist.h \
    mylrc.h

FORMS    += mywidget.ui

RESOURCES += \
    myImages.qrc
