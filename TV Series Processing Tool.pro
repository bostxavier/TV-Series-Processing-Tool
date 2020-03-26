CONFIG += c++11

DEFINES += IL_STD

QT += core
QT += widgets
QT += multimedia
QT += multimediawidgets
QT += opengl

HEADERS += src/MainWindow.h

HEADERS += src/NewProjectDialog.h
HEADERS += src/AddProjectDialog.h
HEADERS += src/CompareImageDialog.h
HEADERS += src/GetFileDialog.h
HEADERS += src/EditSimShotDialog.h
HEADERS += src/ResultsDialog.h
HEADERS += src/ProjectModel.h
HEADERS += src/Segment.h
HEADERS += src/Series.h
HEADERS += src/Season.h
HEADERS += src/Episode.h
HEADERS += src/Scene.h
HEADERS += src/Shot.h
HEADERS += src/VideoFrame.h
HEADERS += src/SpeechSegment.h
HEADERS += src/Face.h
HEADERS += src/ModelView.h
HEADERS += src/UtteranceTree.h
HEADERS += src/UttTreeNode.h
HEADERS += src/UtteranceTreeWidget.h
HEADERS += src/Dendogram.h
HEADERS += src/DendogramNode.h
HEADERS += src/DendogramWidget.h
HEADERS += src/Vertex.h
HEADERS += src/Edge.h

HEADERS += src/MovieMonitor.h
HEADERS += src/HistoMonitor.h
HEADERS += src/VHistoWidget.h
HEADERS += src/HsHistoWidget.h
HEADERS += src/DiffHistoGraph.h
HEADERS += src/SegmentMonitor.h
HEADERS += src/SocialNetWidget.h
HEADERS += src/SpkDiarizationDialog.h
HEADERS += src/SpkDiarMonitor.h
HEADERS += src/SocialNetMonitor.h
HEADERS += src/NarrChartMonitor.h
HEADERS += src/NarrChartWidget.h
HEADERS += src/SceneExtractDialog.h
HEADERS += src/FaceDetectDialog.h
HEADERS += src/MusicTrackingDialog.h
HEADERS += src/SpkInteractDialog.h
HEADERS += src/SummarizationDialog.h

HEADERS += src/MovieAnalyzer.h
HEADERS += src/VideoFrameProcessor.h
HEADERS += src/TextProcessor.h
HEADERS += src/AudioProcessor.h
HEADERS += src/SocialNetProcessor.h
HEADERS += src/Optimizer.h
HEADERS += src/Evaluator.h

HEADERS += src/VideoPlayer.h
HEADERS += src/VignetteWidget.h
HEADERS += src/VideoWidget.h
HEADERS += src/GraphicsView.h
HEADERS += src/PlayerControls.h

HEADERS += src/Convert.h

SOURCES += src/main.cpp
SOURCES += src/MainWindow.cpp

SOURCES += src/NewProjectDialog.cpp
SOURCES += src/AddProjectDialog.cpp
SOURCES += src/CompareImageDialog.cpp
SOURCES += src/GetFileDialog.cpp
SOURCES += src/EditSimShotDialog.cpp
SOURCES += src/ResultsDialog.cpp
SOURCES += src/ProjectModel.cpp
SOURCES += src/Segment.cpp
SOURCES += src/Series.cpp
SOURCES += src/Season.cpp
SOURCES += src/Episode.cpp
SOURCES += src/Scene.cpp
SOURCES += src/Shot.cpp
SOURCES += src/VideoFrame.cpp
SOURCES += src/SpeechSegment.cpp
SOURCES += src/Face.cpp
SOURCES += src/ModelView.cpp
SOURCES += src/UtteranceTree.cpp
SOURCES += src/UttTreeNode.cpp
SOURCES += src/UtteranceTreeWidget.cpp
SOURCES += src/Dendogram.cpp
SOURCES += src/DendogramNode.cpp
SOURCES += src/DendogramWidget.cpp
SOURCES += src/Vertex.cpp
SOURCES += src/Edge.cpp

SOURCES += src/MovieMonitor.cpp
SOURCES += src/HistoMonitor.cpp
SOURCES += src/VHistoWidget.cpp
SOURCES += src/HsHistoWidget.cpp
SOURCES += src/DiffHistoGraph.cpp
SOURCES += src/SegmentMonitor.cpp
SOURCES += src/SocialNetWidget.cpp
SOURCES += src/SpkDiarizationDialog.cpp
SOURCES += src/SpkDiarMonitor.cpp
SOURCES += src/SocialNetMonitor.cpp
SOURCES += src/NarrChartMonitor.cpp
SOURCES += src/NarrChartWidget.cpp
SOURCES += src/SceneExtractDialog.cpp
SOURCES += src/FaceDetectDialog.cpp
SOURCES += src/MusicTrackingDialog.cpp
SOURCES += src/SpkInteractDialog.cpp
SOURCES += src/SummarizationDialog.cpp

SOURCES += src/MovieAnalyzer.cpp
SOURCES += src/VideoFrameProcessor.cpp
SOURCES += src/TextProcessor.cpp
SOURCES += src/AudioProcessor.cpp
SOURCES += src/SocialNetProcessor.cpp
SOURCES += src/Optimizer.cpp
SOURCES += src/Evaluator.cpp

SOURCES += src/VideoPlayer.cpp
SOURCES += src/VignetteWidget.cpp
SOURCES += src/VideoWidget.cpp
SOURCES += src/GraphicsView.cpp
SOURCES += src/PlayerControls.cpp

SOURCES += src/Convert.cpp

INCLUDEPATH += /usr/local/include/igraph
INCLUDEPATH += /opt/ibm/ILOG/CPLEX_Studio1251/cplex/include
INCLUDEPATH += /opt/ibm/ILOG/CPLEX_Studio1251/concert/include

LIBS += -L/usr/lib
LIBS += -L/usr/local/lib
LIBS += -L/opt/ibm/ILOG/CPLEX_Studio1251/cplex/lib/x86-64_sles10_4.1/static_pic
LIBS += -L/opt/ibm/ILOG/CPLEX_Studio1251/concert/lib/x86-64_sles10_4.1/static_pic

LIBS += -lopencv_core
LIBS += -lopencv_highgui
LIBS += -lopencv_imgproc
LIBS += -lopencv_objdetect
LIBS += -lopencv_videoio
LIBS += -lopencv_imgcodecs
LIBS += -larmadillo
LIBS += -lilocplex
LIBS += -lcplex
LIBS += -lconcert
LIBS += -lm
LIBS += -lpthread
LIBS += -ligraph
LIBS += -lGLU

DESTDIR = bin
OBJECTS_DIR = build/.obj
MOC_DIR = build/.moc
RCC_DIR = build/.rcc
UI_DIR = build/.ui
