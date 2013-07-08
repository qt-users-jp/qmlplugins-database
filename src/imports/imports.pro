TEMPLATE = lib
CONFIG += plugin c++11

QT = core sql qml

TARGET = database-qml
TARGETPATH = me/qtquick/Database

HEADERS += \
    database.h \
    tablemodel.h \
    sqlmodel.h \
    plugin.h

SOURCES += \
    database.cpp \
    tablemodel.cpp \
    sqlmodel.cpp

target.path = $$[QT_INSTALL_QML]/$$TARGETPATH

qmldir.files = qmldir
qmldir.path = $$[QT_INSTALL_QML]/$$TARGETPATH

INSTALLS = target qmldir

OTHER_FILES += qmldir
