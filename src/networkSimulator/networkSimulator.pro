QT += core
QT -= gui

CONFIG += c++11
CONFIG += debug_and_release

CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += ../shared 

CONFIG(debug, debug|release) {
    DESTDIR = ../../build/networkSimulator/debug
}
CONFIG(release, debug|release) {
    DESTDIR = ../../build/networkSimulator/release
}

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc

SOURCES += \
    networkSimulator.cpp \
    FeatureExtractor.cpp \
    FeatureReader.cpp \
    SynapseDistributor.cpp \
    SynapseWriter.cpp \
    ../shared/CIS3DAxonRedundancyMap.cpp \
    ../shared/CIS3DBoundingBoxes.cpp \
    ../shared/CIS3DCellTypes.cpp \
    ../shared/CIS3DConstantsHelpers.cpp \
    ../shared/CIS3DNetworkProps.cpp \
    ../shared/CIS3DNeurons.cpp \
    ../shared/CIS3DPSTDensities.cpp \
    ../shared/CIS3DRegions.cpp \
    ../shared/CIS3DSparseField.cpp \
    ../shared/CIS3DSparseVectorSet.cpp \
    ../shared/CIS3DVec3.cpp \
    ../shared/Util.cpp \
    ../shared/UtilIO.cpp \
    ../shared/CIS3DStatistics.cpp \
    ../shared/NetworkStatistic.cpp \
    ../shared/Histogram.cpp \
    ../shared/InnervationStatistic.cpp \
    ../shared/SparseFieldCalculator.cpp \
    ../shared/SparseVectorCache.cpp \
    ../shared/NeuronSelection.cpp


HEADERS += \
    FeatureExtractor.h \
    FeatureReader.h \
    SynapseDistributor.h \
    SynapseWriter.h \
    ../shared/CIS3DAxonRedundancyMap.h \
    ../shared/CIS3DBoundingBoxes.h \
    ../shared/CIS3DCellTypes.h \
    ../shared/CIS3DConstantsHelpers.h \
    ../shared/CIS3DNetworkProps.h \
    ../shared/CIS3DNeurons.h \
    ../shared/CIS3DPSTDensities.h \
    ../shared/CIS3DRegions.h \
    ../shared/CIS3DSparseField.h \
    ../shared/CIS3DSparseVectorSet.h \
    ../shared/CIS3DVec3.h \
    ../shared/Typedefs.h\
    ../shared/Util.h \
    ../shared/UtilIO.h \
    ../shared/CIS3DStatistics.h \
    ../shared/NetworkStatistic.h \
    ../shared/Histogram.h \
    ../shared/InnervationStatistic.h \
    ../shared/SparseFieldCalculator.h \
    ../shared/SparseVectorCache.h \
    ../shared/NeuronSelection.h
