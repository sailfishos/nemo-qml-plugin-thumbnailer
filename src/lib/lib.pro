TEMPLATE = lib
TARGET = nemothumbnailer-qt$${QT_MAJOR_VERSION}

CONFIG += qt hide_symbols create_pc create_prl c++17 link_pkgconfig

QT += \
    gui-private

packagesExist(mlite$${QT_MAJOR_VERSION}) {
    message("Building with mlite$${QT_MAJOR_VERSION} support")
    PKGCONFIG += mlite$${QT_MAJOR_VERSION}
    DEFINES += HAS_MLITE5
} else {
    warning("mlite$${QT_MAJOR_VERSION} not available;")
}

DEFINES += BUILD_NEMO_QML_PLUGIN_THUMBNAILER_LIB


SOURCES += \
    nemoimagemetadata.cpp \
    nemothumbnailcache.cpp
HEADERS += \
    nemoimagemetadata.h \
    nemothumbnailcache.h \
    nemothumbnailexports.h

PLUGIN_IMPORT_PATH = $$[QT_INSTALL_QML]/Nemo/Thumbnailer
DEFINES += NEMO_THUMBNAILER_DIR=\\\"$$PLUGIN_IMPORT_PATH/thumbnailers\\\"

target.path = $$[QT_INSTALL_LIBS]
headers.path = /usr/include/nemothumbnailer-qt$${QT_MAJOR_VERSION}
headers.files =\
    nemothumbnailcache.h \
    nemoimagemetadata.h \
    nemothumbnailexports.h

QMAKE_PKGCONFIG_NAME = lib$$TARGET
QMAKE_PKGCONFIG_DESCRIPTION = Library for generating and accessing thumbnail images
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = $$headers.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig
QMAKE_PKGCONFIG_VERSION = $$VERSION

INSTALLS += target headers
