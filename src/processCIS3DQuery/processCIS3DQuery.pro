QT += core
QT += network

QT -= gui

CONFIG += c++11
CONFIG += debug_and_release

CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

QMAKE_CXXFLAGS += -fopenmp
QMAKE_LFLAGS += -fopenmp

INCLUDEPATH += ../shared

CONFIG(debug, debug|release) {
    DESTDIR = ../../build/processCIS3DQuery/debug
}
CONFIG(release, debug|release) {
    DESTDIR = ../../build/processCIS3DQuery/release
}

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc

SOURCES += \
    processCIS3DQuery.cpp \
    ../shared/CIS3DAxonRedundancyMap.cpp \
    ../shared/CIS3DBoundingBoxes.cpp \
    ../shared/CIS3DCellTypes.cpp \
    ../shared/CIS3DConstantsHelpers.cpp \
    ../shared/CIS3DNetworkProps.cpp \
    ../shared/CIS3DNeurons.cpp \
    ../shared/CIS3DPSTDensities.cpp \
    ../shared/CIS3DRegions.cpp \
    ../shared/CIS3DSparseVectorSet.cpp \
    ../shared/CIS3DStatistics.cpp \
    ../shared/CIS3DVec3.cpp \
    ../shared/NetworkStatistic.cpp \
    ../shared/Histogram.cpp \
    ../shared/InnervationStatistic.cpp \
    ../shared/Util.cpp \
    ../shared/SparseVectorCache.cpp \
    ../shared/NeuronSelection.cpp \
    EvaluationQueryHandler.cpp \
    SelectionQueryHandler.cpp \
    NetworkDataUploadHandler.cpp \
    QueryHelpers.cpp

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
    ../shared/CIS3DStatistics.h \
    ../shared/CIS3DVec3.h \
    ../shared/Histogram.h \
    ../shared/NetworkStatistic.h \
    ../shared/InnervationStatistic.h \
    ../shared/Typedefs.h \
    ../shared/Util.h \
    ../shared/SparseVectorCache.h \
    ../shared/NeuronSelection.h \
    EvaluationQueryHandler.h \
    SelectionQueryHandler.h \
    NetworkDataUploadHandler.h \
    QueryHelpers.h
