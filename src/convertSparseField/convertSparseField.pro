QT += core
QT -= gui

CONFIG += c++11
CONFIG += debug_and_release

CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += ../shared

CONFIG(debug, debug|release) {
    DESTDIR = ../../build/convertSparseField/debug
}
CONFIG(release, debug|release) {
    DESTDIR = ../../build/convertSparseField/release
}

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc

SOURCES += \
    convertSparseField.cpp \
    ../shared/CIS3DBoundingBoxes.cpp \    
    ../shared/CIS3DSparseField.cpp \
    ../shared/CIS3DVec3.cpp


HEADERS += \    
    ../shared/CIS3DSparseField.h \
    ../shared/CIS3DVec3.h
