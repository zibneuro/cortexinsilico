QT += core
QT += network

QT -= gui

CONFIG += c++11
CONFIG += debug_and_release

CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += ../shared

QMAKE_LFLAGS += -fuse-ld=gold

CONFIG(debug, debug|release) {
    DESTDIR = ../../build/computeStatistic/debug
}
CONFIG(release, debug|release) {
    DESTDIR = ../../build/computeStatistic/release
}

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc

SOURCES += \    
    computeStatistic.cpp \
    MyStatistic.cpp \
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
    ../shared/CIS3DSparseField.cpp \
    ../shared/CIS3DStatistics.cpp \
    ../shared/CIS3DVec3.cpp \
    ../shared/NetworkStatistic.cpp \
    ../shared/Histogram.cpp \
    ../shared/InnervationStatistic.cpp \
    ../shared/UtilIO.cpp \
    ../shared/Util.cpp \
    ../shared/TripletMotif.cpp \
    ../shared/MotifCombinations.cpp \
    ../shared/TripletStatistic.cpp \
    ../shared/NeuronSelection.cpp \
    ../shared/InnervationMatrix.cpp 

HEADERS += \
    MyStatistic.h \
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
    ../shared/CIS3DSparseField.h \
    ../shared/CIS3DStatistics.h \
    ../shared/CIS3DVec3.h \
    ../shared/Histogram.h \
    ../shared/NetworkStatistic.h \
    ../shared/InnervationStatistic.h \
    ../shared/Typedefs.h \
    ../shared/UtilIO.h \
    ../shared/Util.h \
    ../shared/TripletMotif.h \
    ../shared/MotifCombinations.h \
    ../shared/TripletStatistic.h \
    ../shared/NeuronSelection.h \
    ../shared/InnervationMatrix.h 
