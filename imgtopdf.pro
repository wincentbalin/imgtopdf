#-------------------------------------------------
#
# Project created by QtCreator 2017-07-27T19:30:21
#
#-------------------------------------------------

QT       += core gui svg

TARGET = imgtopdf
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += main.cpp

win32: QMAKE_LFLAGS += -Wl,--large-address-aware

CONFIG(static) {
    DEFINES += USE_STATIC
    CONFIG += static_qt_plugins
    QTPLUGIN += qico qsvg qtga
    LIBS += -L$$[QT_INSTALL_PLUGINS]/iconengines
    QMAKE_LFLAGS *= -static -lpthread -static-libstdc++ -static-libgcc
}
