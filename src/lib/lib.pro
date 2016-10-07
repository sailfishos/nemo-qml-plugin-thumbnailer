TEMPLATE = lib
TARGET = nemothumbnailer-qt5

CONFIG += qt hide_symbols create_pc create_prl c++11 link_pkgconfig

PKGCONFIG += mlite5

SOURCES += \
    nemoimagemetadata.cpp \
    nemothumbnailcache.cpp
HEADERS += \
    nemoimagemetadata.h \
    nemothumbnailcache.h

PLUGIN_IMPORT_PATH = $$[QT_INSTALL_QML]/org/nemomobile/thumbnailer
DEFINES += NEMO_THUMBNAILER_DIR=\\\"$$PLUGIN_IMPORT_PATH/thumbnailers\\\"

target.path = $$[QT_INSTALL_LIBS]
pkgconfig.files = $$TARGET.pc
pkgconfig.path = $$target.path/pkgconfig
headers.path = /usr/include/nemothumbnailer-qt5
headers.files =\
    nemothumbnailcache.h \
    nemoimagemetadata.h

QMAKE_PKGCONFIG_NAME = lib$$TARGET
QMAKE_PKGCONFIG_DESCRIPTION = Library for generating and accessing thumbnail images
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = $$headers.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig

INSTALLS += target headers pkgconfig

include(../src.pri)
