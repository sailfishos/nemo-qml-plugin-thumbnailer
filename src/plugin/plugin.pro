TARGET = nemothumbnailer
PLUGIN_IMPORT_PATH = Nemo/Thumbnailer

TEMPLATE = lib
CONFIG += qt plugin hide_symbols c++11 link_pkgconfig
QT += qml quick

INCLUDEPATH += ../lib
LIBS += -L../lib -lnemothumbnailer-qt5

target.path = $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH
INSTALLS += target

qmldir.files += qmldir
qmldir.path +=  $$target.path
INSTALLS += qmldir

qmltypes.files += plugins.qmltypes
qmltypes.path +=  $$target.path
INSTALLS += qmltypes

SOURCES += plugin.cpp \
           nemothumbnailprovider.cpp \
           nemothumbnailitem.cpp
HEADERS += nemothumbnailprovider.h \
           nemothumbnailitem.h
