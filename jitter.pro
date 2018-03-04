TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

CONFIG += c++11

LIBS += \
    -lpthread

SOURCES += main.cpp \
    videojitter.cpp \
    smooth.cpp

HEADERS += \
    videojitter.h \
    sync_queue.h \
    random_generator.h \
    simple_lock.h \
    smooth.h \
    simple_array.h


