TARGET = nemothumbnailer
PLUGIN_IMPORT_PATH = Nemo/Thumbnailer

TEMPLATE = lib
CONFIG += qt plugin hide_symbols c++17 link_pkgconfig
QT += qml quick

INCLUDEPATH += ../lib
LIBS += -L../lib -lnemothumbnailer-qt$${QT_MAJOR_VERSION}

target.path = $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH
INSTALLS += target

qmldir.files += qmldir plugins.qmltypes
qmldir.path +=  $$target.path
INSTALLS += qmldir

qmltypes.commands = qmlplugindump -nonrelocatable Nemo.Thumbnailer 1.0 > $$PWD/plugins.qmltypes
QMAKE_EXTRA_TARGETS += qmltypes

SOURCES += plugin.cpp \
           nemothumbnailprovider.cpp \
           nemothumbnailitem.cpp
HEADERS += nemothumbnailprovider.h \
           nemothumbnailitem.h
