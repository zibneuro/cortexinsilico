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
    ../shared/CIS3DSparseField.cpp \
    ../shared/CIS3DStatistics.cpp \
    ../shared/CIS3DVec3.cpp \
    ../shared/NetworkStatistic.cpp \
    ../shared/Histogram.cpp \
    ../shared/InnervationStatistic.cpp \
    ../shared/TripletMotif.cpp \
    ../shared/TripletStatistic.cpp \
    ../shared/InDegreeStatistic.cpp \
    ../shared/MotifCombinations.cpp \
    ../shared/Util.cpp \
    ../shared/UtilIO.cpp \
    ../shared/SparseVectorCache.cpp \
    ../shared/NeuronSelection.cpp \
    ../shared/Columns.cpp \
    ../shared/InnervationMatrix.cpp \
    ../shared/RandomGenerator.cpp \
    ../shared/FeatureProvider.cpp \
    EvaluationQueryHandler.cpp \
    MotifQueryHandler.cpp \
    InDegreeQueryHandler.cpp \
    SelectionQueryHandler.cpp \
    NetworkDataUploadHandler.cpp \
    SpatialInnervationQueryHandler.cpp \
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
    ../shared/CIS3DSparseField.h \
    ../shared/CIS3DStatistics.h \
    ../shared/CIS3DVec3.h \
    ../shared/Histogram.h \
    ../shared/NetworkStatistic.h \
    ../shared/InnervationStatistic.h \
    ../shared/TripletStatistic.h \
    ../shared/InDegreeStatistic.h \
    ../shared/TripletMotif.h \
    ../shared/MotifCombinations.h \
    ../shared/Typedefs.h \
    ../shared/Util.h \
    ../shared/UtilIO.h \
    ../shared/SparseVectorCache.h \
    ../shared/NeuronSelection.h \
    ../shared/Columns.h \
    ../shared/InnervationMatrix.h \
    ../shared/RandomGenerator.h \
    ../shared/FormulaParser.h \
    ../shared/FeatureProvider.h \
    EvaluationQueryHandler.h \
    MotifQueryHandler.h \
    InDegreeQueryHandler.h \
    SelectionQueryHandler.h \
    NetworkDataUploadHandler.h \
    SpatialInnervationQueryHandler.h \
    QueryHelpers.h
