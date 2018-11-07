QT += core
QT += network

QT -= gui

CONFIG += c++11
CONFIG += debug

CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += ../../shared

CONFIG(debug) {
    DESTDIR = ../../../build/mt/debug
}
CONFIG(release) {
    DESTDIR = ../../../build/mt/release
}

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc

SOURCES += \    
    mt.cpp \
    ../../shared/RandomGenerator.cpp \

HEADERS += \
    ../../shared/RandomGenerator.h 
    
