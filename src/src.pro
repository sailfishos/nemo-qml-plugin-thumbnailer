TARGET = nemothumbnailer
PLUGIN_IMPORT_PATH = org/nemomobile/thumbnailer

TEMPLATE = lib
CONFIG += qt plugin hide_symbols c++11
QT += qml quick

target.path = $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH
INSTALLS += target

qmldir.files += $$_PRO_FILE_PWD_/qmldir
qmldir.path +=  $$target.path
INSTALLS += qmldir

qmltypes.files += $$_PRO_FILE_PWD_/plugins.qmltypes
qmltypes.path +=  $$target.path
INSTALLS += qmltypes

SOURCES += plugin.cpp \
           nemothumbnailprovider.cpp \
           nemoimagemetadata.cpp \
           nemothumbnailitem.cpp    \
           nemothumbnailcache.cpp
HEADERS += nemothumbnailprovider.h \
           nemoimagemetadata.h \
           nemothumbnailitem.h \
           nemothumbnailcache.h

DEFINES += NEMO_THUMBNAILER_DIR=\\\"$$target.path/thumbnailers\\\"
