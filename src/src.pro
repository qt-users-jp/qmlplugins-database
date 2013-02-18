TEMPLATE = subdirs

!isEmpty(QT.qml.name) {
    SUBDIRS += imports
}
