#-------------------------------------------------
#
# Project created by QtCreator 2017-07-27T19:30:21
#
#-------------------------------------------------

QT       += core svg

TARGET = imgtopdf
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += main.cpp

win32: QMAKE_LFLAGS += -Wl,--large-address-aware
