#-------------------------------------------------
#
# Project created by QtCreator 2016-02-07T14:51:51
#
#-------------------------------------------------

QT       += core gui network dbus
CONFIG   += c++11
LIBS     += -lparted -lparted-fs-resize

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = install_theos
TEMPLATE = app

INCLUDEPATH += /usr/include/KF5 /usr/include/KF5/NetworkManagerQt /usr/include/NetworkManager /usr/include/glib-2.0 /usr/lib/glib-2.0/include /usr/include/libnm

SOURCES += main.cpp\
        mainwindow.cpp \
    internetconnection.cpp \
    worker.cpp \
    installworker.cpp \
    partitionwindow.cpp \
    erasedrivedialog.cpp \
    driveinfo.cpp \
    partitionframe.cpp

HEADERS  += mainwindow.h \
    internetconnection.h \
    worker.h \
    installworker.h \
    partitionwindow.h \
    erasedrivedialog.h \
    driveinfo.h \
    partitionframe.h

FORMS    += mainwindow.ui \
    internetconnection.ui \
    partitionwindow.ui \
    erasedrivedialog.ui

RESOURCES += \
    resources.qrc
