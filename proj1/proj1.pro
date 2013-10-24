TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    src/app.c \
    src/link.c \
    src/main.c \
    src/misc.c

HEADERS += \
    include/app.h \
    include/link.h \
    include/misc.h

INCLUDEPATH += include

QMAKE_CFLAGS += -std=gnu99
