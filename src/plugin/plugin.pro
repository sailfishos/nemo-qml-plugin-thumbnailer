TARGET = nemothumbnailer
PLUGIN_IMPORT_PATH = org/nemomobile/thumbnailer

TEMPLATE = lib
CONFIG += qt plugin hide_symbols c++11 link_pkgconfig
QT += qml quick

PKGCONFIG += mlite5

INCLUDEPATH += ../lib
LIBS += -L../lib -lnemothumbnailer-qt5

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
           nemothumbnailitem.cpp
HEADERS += nemothumbnailprovider.h \
           nemothumbnailitem.h

include(../src.pri)
