######################################################################
# PRad Event Viewer, project file for qmake
# It provides several optional components, they can be disabled by
# comment out the corresponding line
# Chao Peng
# 10/07/2016
######################################################################

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets concurrent
}

# enable multi threading
DEFINES += MULTI_THREAD

######################################################################
# optional components
######################################################################

# empty components
COMPONENTS = 

# enable online mode, it requires Event Transfer,
# it is the monitoring process from CODA group
#COMPONENTS += ONLINE_MODE

# enable high voltage control, it requires CAENHVWrapper library
#COMPONENTS += HV_CONTROL

# use standard evio libraries instead of self-defined function to read
# evio data files
#COMPONENTS += STANDARD_EVIO

# enable the reconstruction display in GUI
COMPONENTS += RECON_DISPLAY

######################################################################
# optional components end
######################################################################

######################################################################
# general config for qmake
######################################################################

CONFIG += c++11
QMAKE_CXXFLAGS += -std=c++11

OBJECTS_DIR = obj
MOC_DIR = qt_moc

TEMPLATE = app
TARGET = PRadEventViewer
DEPENDPATH += 
INCLUDEPATH += include \
               $$(ROOTSYS)/include

# Input
HEADERS += include/PRadEventViewer.h \
           include/HyCalModule.h \
           include/HyCalScene.h \
           include/HyCalView.h \
           include/Spectrum.h \
           include/SpectrumSettingPanel.h \
           include/HtmlDelegate.h \
           include/QRootCanvas.h \
           include/ConfigParser.h \
           include/ConfigValue.h \
           include/ConfigObject.h \
           include/PRadHistCanvas.h \
           include/PRadLogBox.h \
           include/PRadDetector.h \
           include/PRadEvioParser.h \
           include/PRadDSTParser.h \
           include/PRadDataHandler.h \
           include/PRadInfoCenter.h \
           include/datastruct.h \
           include/PRadEventStruct.h \
           include/PRadException.h \
           include/PRadBenchMark.h \
           include/PRadEventFilter.h \
           include/PRadCoordSystem.h \
           include/PRadDetMatch.h \
           include/PRadHyCalSystem.h \
           include/PRadHyCalDetector.h \
           include/PRadHyCalModule.h \
           include/PRadCalibConst.h \
           include/PRadDAQChannel.h \
           include/PRadADCChannel.h \
           include/PRadTDCChannel.h \
           include/PRadClusterProfile.h \
           include/PRadHyCalCluster.h \
           include/PRadSquareCluster.h \
           include/PRadIslandCluster.h \
           include/PRadGEMSystem.h \
           include/PRadGEMDetector.h \
           include/PRadGEMPlane.h \
           include/PRadGEMFEC.h \
           include/PRadGEMAPV.h \
           include/PRadGEMCluster.h \
           include/PRadEPICSystem.h \
           include/PRadTaggerSystem.h \
           include/canalib.h

SOURCES += src/main.cpp \
           src/PRadEventViewer.cpp \
           src/HyCalModule.cpp \
           src/HyCalScene.cpp \
           src/HyCalView.cpp \
           src/Spectrum.cpp \
           src/SpectrumSettingPanel.cpp \
           src/HtmlDelegate.cpp \
           src/QRootCanvas.cpp \
           src/ConfigParser.cpp \
           src/ConfigValue.cpp \
           src/ConfigObject.cpp \
           src/PRadHistCanvas.cpp \
           src/PRadLogBox.cpp \
           src/PRadDetector.cpp \
           src/PRadEvioParser.cpp \
           src/PRadDSTParser.cpp \
           src/PRadDataHandler.cpp \
           src/PRadInfoCenter.cpp \
           src/PRadException.cpp \
           src/PRadBenchMark.cpp \
           src/PRadEventFilter.cpp \
           src/PRadCoordSystem.cpp \
           src/PRadDetMatch.cpp \
           src/PRadHyCalSystem.cpp \
           src/PRadHyCalDetector.cpp \
           src/PRadHyCalModule.cpp \
           src/PRadCalibConst.cpp \
           src/PRadDAQChannel.cpp \
           src/PRadADCChannel.cpp \
           src/PRadTDCChannel.cpp \
           src/PRadClusterProfile.cpp \
           src/PRadHyCalCluster.cpp \
           src/PRadSquareCluster.cpp \
           src/PRadIslandCluster.cpp \
           src/PRadGEMSystem.cpp \
           src/PRadGEMDetector.cpp \
           src/PRadGEMPlane.cpp \
           src/PRadGEMFEC.cpp \
           src/PRadGEMAPV.cpp \
           src/PRadGEMCluster.cpp \
           src/PRadEPICSystem.cpp \
           src/PRadTaggerSystem.cpp \
           src/canalib.cpp

LIBS += -lexpat -lgfortran \
        -L$$(ROOTSYS)/lib -lCore -lRint -lRIO -lNet -lHist \
                          -lGraf -lGraf3d -lGpad -lTree \
                          -lPostscript -lMatrix -lPhysics \
                          -lMathCore -lThread -lGui -lSpectrum

######################################################################
# general config end
######################################################################

######################################################################
# other compilers
######################################################################

FORTRAN_SOURCES += src/island.F
fortran.output = $${OBJECTS_DIR}/${QMAKE_FILE_BASE}.o
fortran.commands = gfortran -c ${QMAKE_FILE_NAME} -Iinclude -o ${QMAKE_FILE_OUT}
fortran.input = FORTRAN_SOURCES
QMAKE_EXTRA_COMPILERS += fortran

######################################################################
# other compilers end
######################################################################

######################################################################
# implement self-defined components
######################################################################

contains(COMPONENTS, ONLINE_MODE) {
    DEFINES += USE_ONLINE_MODE
    HEADERS += include/PRadETChannel.h \
               include/PRadETStation.h \
               include/ETSettingPanel.h
    SOURCES += src/PRadETChannel.cpp \
               src/PRadETStation.cpp \
               src/ETSettingPanel.cpp
    INCLUDEPATH += $$(ET_INC)
    LIBS += -L$$(ET_LIB) -let
}

contains(COMPONENTS, HV_CONTROL) {
    DEFINES += USE_CAEN_HV
    HEADERS += include/PRadHVSystem.h \
               include/CAENHVSystem.h
    SOURCES += src/PRadHVSystem.cpp \
               src/CAENHVSystem.cpp
    INCLUDEPATH += thirdparty/include
    LIBS += -L$$(THIRD_LIB) -lcaenhvwrapper
}

contains(COMPONENTS, STANDARD_EVIO) {
    DEFINES += USE_EVIO_LIB
    !contains(INCLUDEPATH, thirdparty/include) {
        INCLUDEPATH += thirdparty/include
    }
    LIBS += -L$$(THIRD_LIB) -levio -levioxx
}

contains(COMPONENTS, RECON_DISPLAY) {
    DEFINES += RECON_DISPLAY
    HEADERS += include/ReconSettingPanel.h \
               include/MarkSettingWidget.h
    SOURCES += src/ReconSettingPanel.cpp \
               src/MarkSettingWidget.cpp
}

######################################################################
# self-defined components end
######################################################################

