TARGET = nemothumbnailer
PLUGIN_IMPORT_PATH = org/nemomobile/thumbnailer

TEMPLATE = lib
CONFIG += qt plugin hide_symbols
QT += declarative

target.path = $$[QT_INSTALL_IMPORTS]/$$PLUGIN_IMPORT_PATH
INSTALLS += target

qmldir.files += $$_PRO_FILE_PWD_/qmldir
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$$$PLUGIN_IMPORT_PATH
INSTALLS += qmldir

SOURCES += plugin.cpp \
           nemothumbnailprovider.cpp \
           nemoimagemetadata.cpp \
           nemothumbnailitem.cpp    \
           nemovideothumbnailer.cpp
HEADERS += nemothumbnailprovider.h \
           nemoimagemetadata.h \
           nemothumbnailitem.h \
           nemovideothumbnailer.h

DEFINES += NEMO_THUMBNAILER_DIR=\\\"$$[QT_INSTALL_IMPORTS]/$$$$PLUGIN_IMPORT_PATH/thumbnailers\\\"
