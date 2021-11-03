TEMPLATE=aux

CONFIG += sailfish-qdoc-template
SAILFISH_QDOC.project = nemo-qml-plugin-thumbnailer
SAILFISH_QDOC.config = nemo-qml-plugin-thumbnailer.qdocconf
SAILFISH_QDOC.style = offline
SAILFISH_QDOC.path = /usr/share/doc/nemo-qml-plugin-thumbnailer

OTHER_FILES += \
    src/index.qdoc
