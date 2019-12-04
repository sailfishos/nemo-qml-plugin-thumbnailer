TEMPLATE = subdirs
SUBDIRS = src
CONFIG += mer-qdoc-template
MER_QDOC.project = nemo-qml-plugin-thumbnailer
MER_QDOC.config = doc/nemo-qml-plugin-thumbnailer.qdocconf
MER_QDOC.style = offline
MER_QDOC.path = /usr/share/doc/nemo-qml-plugin-thumbnailer

OTHER_FILES += \
    rpm/nemo-qml-plugin-thumbnailer-qt5.spec \
    doc/src/index.qdoc
