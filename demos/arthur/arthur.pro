######################################################################
# Automatically generated by qmake (1.08a)
######################################################################

TEMPLATE = app

SOURCES = \
	alphashade.cpp \
	clipping.cpp \
	demoviewer.cpp \
	demowidget.cpp \
	introscreen.cpp \
	main.cpp \
	paths.cpp \
	rotatinggradient.cpp \
	warpix.cpp

HEADERS = \
	alphashade.h \
	clipping.h \
	demoviewer.h \
	demowidget.h \
	introscreen.h \
	paths.h \
	rotatinggradient.h \
	warpix.h

contains(QT_CONFIG, opengl) {
	HEADERS += glpainter.h
	SOURCES += glpainter.cpp
	QT += opengl
}


TARGET = arthur