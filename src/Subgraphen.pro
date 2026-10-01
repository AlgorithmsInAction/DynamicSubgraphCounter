########################################################################
# Copyright (C) 2013 - 2020 : Kathrin Hanauer                          #
#                                                                      #
# This file is part of Algora.                                         #
#                                                                      #
# Algora is free software: you can redistribute it and/or modify       #
# it under the terms of the GNU General Public License as published by #
# the Free Software Foundation, either version 3 of the License, or    #
# (at your option) any later version.                                  #
#                                                                      #
# Algora is distributed in the hope that it will be useful,            #
# but WITHOUT ANY WARRANTY; without even the implied warranty of       #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        #
# GNU General Public License for more details.                         #
#                                                                      #
# You should have received a copy of the GNU General Public License    #
# along with Algora.  If not, see <http://www.gnu.org/licenses/>.      #
#                                                                      #
# Contact information:                                                 #
#   http://algora.xaikal.org                                           #
########################################################################

QT =

CONFIG += c++17
# OB and ESCAPE both provide a Graph.cpp. Preserve each source path in its
# object-file path so qmake does not collapse both translation units to Graph.o.
CONFIG += object_parallel_to_source
isEmpty(REFERENCE_CODE): REFERENCE_CODE = ON
!equals(REFERENCE_CODE, ON):!equals(REFERENCE_CODE, OFF): error("REFERENCE_CODE must be ON or OFF")
equals(REFERENCE_CODE, ON): DEFINES += WITH_REFERENCE_CODE
QMAKE_CXXFLAGS += -pthread
LIBS += -pthread

TARGET = SubgraphCounter
CONFIG -= app_bundle

TEMPLATE = app

QMAKE_CXXFLAGS_APP =
QMAKE_CXXFLAGS_STATIC_LIB = # remove -fPIC

QMAKE_CXXFLAGS_DEBUG += -std=c++17 -O0

QMAKE_CXXFLAGS_RELEASE -= -O3 -O2 -O1
QMAKE_CXXFLAGS_RELEASE += -std=c++17 -DNDEBUG -flto
QMAKE_LFLAGS_RELEASE += -flto -O3

general {
  QMAKE_CXXFLAGS_RELEASE += -O2 -march=x86-64
} else {
  QMAKE_CXXFLAGS_RELEASE += -O3 -march=native -mtune=native
}

debugsymbols {
	QMAKE_CXXFLAGS_RELEASE += -fno-omit-frame-pointer -g
}

profiling {
	QMAKE_CXXFLAGS_DEBUG   += -DCOLLECT_PR_DATA
	QMAKE_CXXFLAGS_RELEASE += -DCOLLECT_PR_DATA
}

SOURCES += main.cpp\
	partition/EpsilonTab.cpp\
	partition/HIndex.cpp\
	partition/MockPartition.cpp\
	e_counts/EStructureCounts.cpp\
	e_counts/ESubgraphCounts.cpp\
	h_counts/HStructureCounts.cpp\
	h_counts/HStructureCountsvLV.cpp\
	h_counts/HStructureCountsuLv.cpp\
	h_counts/HStructureCountst.cpp \
	h_counts/HStructureCountsuLLv.cpp \
	h_counts/HStructureCountscLV.cpp \
	h_counts/HStructureCountspLL.cpp \
	h_counts/HStructureCountsuHv.cpp \
	h_counts/HStructureCountscL.cpp \
	h_counts/HSubgraphCounts.cpp \
	h_counts/HSubgraphCountsExtended.cpp \
	h_vanilla/HVStructureCounts.cpp \
	h_vanilla/HVStructureCountscL.cpp \
	h_vanilla/HVStructureCountspLL.cpp \
	h_vanilla/HVStructureCountsuLLv.cpp \
	h_vanilla/HVStructureCountscLV.cpp \
	h_vanilla/HVStructureCountst.cpp \
	h_vanilla/HVStructureCountsuLv.cpp \
	h_vanilla/HVSubgraphCounts.cpp \
	h_vanilla/HVStructureCountsuHv.cpp \
	h_vanilla/HVStructureCountsvLV.cpp \
	h_vanilla/HVSubgraphCountsExtended.cpp \
	h_counts/HSubgraphCountsS.cpp
    

HEADERS += test.h\
	partition/EpsilonTab.h\
	partition/HIndex.h\
	partition/MockPartition.h\
	partition/VertexPartition.h\
	ObserverAlgorithm.h\
	h_counts/HStructureCounts.h\
	h_counts/HSubgraphCounts.h \
	h_vanilla/HVStructureCounts.h \
	h_vanilla/HVSubgraphCounts.h \
	e_counts/ESubgraphCounts.h \
	e_counts/ESubgraphCounts.h \
	StructureCounts.h \
	SubgraphCounts.h \
	UndirectedFourSubgraphCounts.h \
	util/streaming_stats.h\
	static/StaticAlgorithm.h \
	static/StaticWorkerPool.h

equals(REFERENCE_CODE, ON) {
    SOURCES += oaqc/DynamizedOBASubgraphCounts.cpp \
        ../deps/oaqc/src/Graph.cpp \
        ../deps/oaqc/src/QuadCensus.cpp \
        escape/DynamizedESCAPE.cpp \
        ../deps/escape/Graph.cpp \
        ../deps/escape/GraphIO.cpp \
        ../deps/escape/TriangleProgram.cpp
    HEADERS += oaqc/DynamizedOBASubgraphCounts.h \
        escape/DynamizedESCAPE.h \
        ../deps/oaqc/src/Graph.h \
        ../deps/oaqc/src/QuadCensus.h \
        ../deps/escape/Escape/AlmostFiveClique.h \
        ../deps/escape/Escape/Digraph.h \
        ../deps/escape/Escape/ErrorCode.h \
        ../deps/escape/Escape/FiveFromCycleClique.h \
        ../deps/escape/Escape/FiveTrees.h \
        ../deps/escape/Escape/GetAllCounts.h \
        ../deps/escape/Escape/GraphIO.h \
        ../deps/escape/Escape/Triadic.h \
        ../deps/escape/Escape/Utils.h \
        ../deps/escape/Escape/Conversion.h \
        ../deps/escape/Escape/EdgeHash.h \
        ../deps/escape/Escape/FiveCycle.h \
        ../deps/escape/Escape/FiveFromTriangles.h \
        ../deps/escape/Escape/FourVertex.h \
        ../deps/escape/Escape/Graph.h \
        ../deps/escape/Escape/JointSort.h \
        ../deps/escape/Escape/TriangleProgram.h \
        ../deps/escape/Escape/WedgeCollisions.h
    INCLUDEPATH += $$PWD/../deps/oaqc/src $$PWD/../deps/escape
}

CONFIG(release, debug|release) {
  message("Target: Release")
# uncomment the following line if you are also using AlgoraDyn
  unix:!macx: LIBS += -L$$PWD/../../AlgoraDyn/build/Release/ -lAlgoraDyn
  unix:!macx: LIBS += -L$$PWD/../../AlgoraCore/build/Release/ -lAlgoraCore
}
CONFIG(debug, debug|release) {
  message("Target: Debug")
# uncomment the following line if you are also using AlgoraDyn
  unix:!macx: LIBS += -L$$PWD/../../AlgoraDyn/build/Debug/ -lAlgoraDyn
  unix:!macx: LIBS += -L$$PWD/../../AlgoraCore/build/Debug/ -lAlgoraCore
}

INCLUDEPATH += $$PWD/../../AlgoraCore/src
DEPENDPATH += $$PWD/../../AlgoraCore/src


unix:!macx: PRE_TARGETDEPS += $$PWD/../../AlgoraCore/build/Debug/libAlgoraCore.a

# uncomment the following lines if you are also using AlgoraDyn
INCLUDEPATH += $$PWD/../../AlgoraDyn/src
DEPENDPATH += $$PWD/../../AlgoraDyn/src

INCLUDEPATH += $$PWD/../../boost_1_77_0

unix:!macx: PRE_TARGETDEPS += $$PWD/../../AlgoraDyn/build/Debug/libAlgoraDyn.a
