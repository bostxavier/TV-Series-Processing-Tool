CONFIG += c++11

DEFINES += IL_STD

QT += core
QT += widgets
QT += multimedia
QT += multimediawidgets
QT += opengl

HEADERS += MainWindow.h

HEADERS += NewProjectDialog.h
HEADERS += AddProjectDialog.h
HEADERS += CompareImageDialog.h
HEADERS += GetFileDialog.h
HEADERS += EditSimShotDialog.h
HEADERS += ResultsDialog.h
HEADERS += ProjectModel.h
HEADERS += Segment.h
HEADERS += Series.h
HEADERS += Season.h
HEADERS += Episode.h
HEADERS += Scene.h
HEADERS += Shot.h
HEADERS += VideoFrame.h
HEADERS += SpeechSegment.h
HEADERS += Face.h
HEADERS += ModelView.h
HEADERS += UtteranceTree.h
HEADERS += UttTreeNode.h
HEADERS += UtteranceTreeWidget.h
HEADERS += Dendogram.h
HEADERS += DendogramNode.h
HEADERS += DendogramWidget.h
HEADERS += Vertex.h
HEADERS += Edge.h

HEADERS += MovieMonitor.h
HEADERS += HistoMonitor.h
HEADERS += VHistoWidget.h
HEADERS += HsHistoWidget.h
HEADERS += DiffHistoGraph.h
HEADERS += SegmentMonitor.h
HEADERS += SocialNetWidget.h
HEADERS += SpkDiarizationDialog.h
HEADERS += SpkDiarMonitor.h
HEADERS += SocialNetMonitor.h
HEADERS += NarrChartMonitor.h
HEADERS += NarrChartWidget.h
HEADERS += SceneExtractDialog.h
HEADERS += FaceDetectDialog.h
HEADERS += MusicTrackingDialog.h
HEADERS += SpkInteractDialog.h
HEADERS += SummarizationDialog.h

HEADERS += MovieAnalyzer.h
HEADERS += VideoFrameProcessor.h
HEADERS += TextProcessor.h
HEADERS += AudioProcessor.h
HEADERS += SocialNetProcessor.h
HEADERS += Optimizer.h
HEADERS += Evaluator.h

HEADERS += VideoPlayer.h
HEADERS += VignetteWidget.h
HEADERS += VideoWidget.h
HEADERS += GraphicsView.h
HEADERS += PlayerControls.h

HEADERS += Convert.h

SOURCES += main.cpp
SOURCES += MainWindow.cpp

SOURCES += NewProjectDialog.cpp
SOURCES += AddProjectDialog.cpp
SOURCES += CompareImageDialog.cpp
SOURCES += GetFileDialog.cpp
SOURCES += EditSimShotDialog.cpp
SOURCES += ResultsDialog.cpp
SOURCES += ProjectModel.cpp
SOURCES += Segment.cpp
SOURCES += Series.cpp
SOURCES += Season.cpp
SOURCES += Episode.cpp
SOURCES += Scene.cpp
SOURCES += Shot.cpp
SOURCES += VideoFrame.cpp
SOURCES += SpeechSegment.cpp
SOURCES += Face.cpp
SOURCES += ModelView.cpp
SOURCES += UtteranceTree.cpp
SOURCES += UttTreeNode.cpp
SOURCES += UtteranceTreeWidget.cpp
SOURCES += Dendogram.cpp
SOURCES += DendogramNode.cpp
SOURCES += DendogramWidget.cpp
SOURCES += Vertex.cpp
SOURCES += Edge.cpp

SOURCES += MovieMonitor.cpp
SOURCES += HistoMonitor.cpp
SOURCES += VHistoWidget.cpp
SOURCES += HsHistoWidget.cpp
SOURCES += DiffHistoGraph.cpp
SOURCES += SegmentMonitor.cpp
SOURCES += SocialNetWidget.cpp
SOURCES += SpkDiarizationDialog.cpp
SOURCES += SpkDiarMonitor.cpp
SOURCES += SocialNetMonitor.cpp
SOURCES += NarrChartMonitor.cpp
SOURCES += NarrChartWidget.cpp
SOURCES += SceneExtractDialog.cpp
SOURCES += FaceDetectDialog.cpp
SOURCES += MusicTrackingDialog.cpp
SOURCES += SpkInteractDialog.cpp
SOURCES += SummarizationDialog.cpp

SOURCES += MovieAnalyzer.cpp
SOURCES += VideoFrameProcessor.cpp
SOURCES += TextProcessor.cpp
SOURCES += AudioProcessor.cpp
SOURCES += SocialNetProcessor.cpp
SOURCES += Optimizer.cpp
SOURCES += Evaluator.cpp

SOURCES += VideoPlayer.cpp
SOURCES += VignetteWidget.cpp
SOURCES += VideoWidget.cpp
SOURCES += GraphicsView.cpp
SOURCES += PlayerControls.cpp

SOURCES += Convert.cpp

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
