TEMPLATE = lib
TARGET = nemothumbnailer-qt5

CONFIG += qt hide_symbols create_pc create_prl c++11 link_pkgconfig

packagesExist(mlite5) {
    message("Building with mlite5 support")
    PKGCONFIG += mlite5
    DEFINES += HAS_MLITE5
} else {
    warning("mlite5 not available;")
}

SOURCES += \
    nemoimagemetadata.cpp \
    nemothumbnailcache.cpp
HEADERS += \
    nemoimagemetadata.h \
    nemothumbnailcache.h

PLUGIN_IMPORT_PATH = $$[QT_INSTALL_QML]/Nemo/Thumbnailer
DEFINES += NEMO_THUMBNAILER_DIR=\\\"$$PLUGIN_IMPORT_PATH/thumbnailers\\\"

target.path = $$[QT_INSTALL_LIBS]
headers.path = /usr/include/nemothumbnailer-qt5
headers.files =\
    nemothumbnailcache.h \
    nemoimagemetadata.h

QMAKE_PKGCONFIG_NAME = lib$$TARGET
QMAKE_PKGCONFIG_DESCRIPTION = Library for generating and accessing thumbnail images
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = $$headers.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig
QMAKE_PKGCONFIG_VERSION = $$VERSION

INSTALLS += target headers
