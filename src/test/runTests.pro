QT += core
QT += testlib
QT -= gui

CONFIG += c++11
CONFIG += testcase
CONFIG += debug_and_release

CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += ../shared

QMAKE_LFLAGS += -fuse-ld=gold

CONFIG(debug, debug|release) {
    DESTDIR = ../../build/test/debug
}
CONFIG(release, debug|release) {
    DESTDIR = ../../build/test/release
}

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc

SOURCES += \
    ../shared/CIS3DBoundingBoxes.cpp \
    ../shared/CIS3DSparseField.cpp \
    ../shared/CIS3DSparseVectorSet.cpp \
    ../shared/CIS3DVec3.cpp \
    SparseFieldTest.cpp \
    SparseVectorSetTest.cpp \
    runTests.cpp

HEADERS += \
    ../shared/CIS3DBoundingBoxes.h \
    ../shared/CIS3DSparseField.h \
    ../shared/CIS3DSparseVectorSet.h \
    ../shared/CIS3DVec3.h \
    SparseFieldTest.h \
    SparseVectorSetTest.h
