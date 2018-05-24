QT += core
QT -= gui

CONFIG += c++11
CONFIG += debug_and_release

CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += ../shared

CONFIG(debug, debug|release) {
    DESTDIR = ../../build/convertInnervationToCSV/debug
}
CONFIG(release, debug|release) {
    DESTDIR = ../../build/convertInnervationToCSV/release
}

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc

SOURCES += \
    convertInnervationToCSV.cpp \
    ../shared/CIS3DAxonRedundancyMap.cpp \
    ../shared/CIS3DBoundingBoxes.cpp \
    ../shared/CIS3DCellTypes.cpp \
    ../shared/CIS3DConstantsHelpers.cpp \
    ../shared/CIS3DNetworkProps.cpp \
    ../shared/CIS3DNeurons.cpp \
    ../shared/CIS3DPSTDensities.cpp \
    ../shared/CIS3DRegions.cpp \
    ../shared/CIS3DSparseVectorSet.cpp \
    ../shared/SparseVectorCache.cpp \
    ../shared/CIS3DVec3.cpp


HEADERS += \
    ../shared/CIS3DAxonRedundancyMap.h \
    ../shared/CIS3DBoundingBoxes.h \
    ../shared/CIS3DCellTypes.h \
    ../shared/CIS3DConstantsHelpers.h \
    ../shared/CIS3DNetworkProps.h \
    ../shared/CIS3DNeurons.h \
    ../shared/CIS3DPSTDensities.h \
    ../shared/CIS3DRegions.h \
    ../shared/CIS3DSparseVectorSet.h \
    ../shared/SparseVectorCache.h \
    ../shared/CIS3DVec3.h
