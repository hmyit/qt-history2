# Qt gui library, paint module

HEADERS += \
	painting/qbrush.h \
	painting/qcolor.h \
	painting/qcolor_p.h \
	painting/qdrawutil.h \
	painting/qpaintdevice.h \
	painting/qpaintdevicedefs.h \
	painting/qpaintdevicemetrics.h \
	painting/qpaintengine.h \
	painting/qpainter.h \
	painting/qpainter_p.h \
	painting/qpainterpath.h \
	painting/qpainterpath_p.h \
	painting/qpen.h \
	painting/qpoint.h \
	painting/qpointarray.h \
	painting/qpolygonscanner.h \
	painting/qrect.h \
	painting/qregion.h \
	painting/qsize.h \
	painting/qwmatrix.h


SOURCES += \
	painting/qbrush.cpp \
	painting/qcolor.cpp \
	painting/qcolor_p.cpp \
	painting/qdrawutil.cpp \
	painting/qpaintdevicemetrics.cpp \
	painting/qpaintengine.cpp \
	painting/qpainter.cpp \
	painting/qpainterpath.cpp \
	painting/qpen.cpp \
	painting/qpoint.cpp \
	painting/qpointarray.cpp \
	painting/qpolygonscanner.cpp \
	painting/qrect.cpp \
	painting/qregion.cpp \
	painting/qsize.cpp \
	painting/qwmatrix.cpp

win32 {
	HEADERS += \
		painting/qpaintengine_win.h \
		painting/qpaintengine_win_p.h

 	SOURCES += \
		painting/qcolor_win.cpp \
		painting/qpaintdevice_win.cpp \
		painting/qpaintengine_win.cpp \
		painting/qregion_win.cpp
	LIBS += -lmsimg32
}

wince-* {
	SOURCES -= painting/qregion_win.cpp
	SOURCES += painting/qregion_wce.cpp
}


unix:x11 {
	HEADERS += \
		painting/qpaintengine_x11.h \
		painting/qpaintengine_x11_p.h

	SOURCES += \
		painting/qcolor_x11.cpp \
		painting/qpaintdevice_x11.cpp \
		painting/qpaintengine_x11.cpp
}

!embedded:!x11:mac {
	HEADERS += \
		painting/qpaintengine_mac.h \
		painting/qpaintengine_mac_p.h

	SOURCES += \
		painting/qcolor_mac.cpp \
		painting/qpaintdevice_mac.cpp \
		painting/qpaintengine_mac.cpp \
} else:unix {
	HEADERS	+= \
		painting/qpsprinter_p.h

	SOURCES += \
		painting/qpsprinter.cpp
}

unix:SOURCES += painting/qregion_unix.cpp


embedded {
	HEADERS += \
		painting/qpaintengine_qws.h \
		painting/qpaintengine_qws_p.h

	SOURCES += \
		painting/qpaintengine_qws.cpp \
		painting/qpaintdevice_qws.cpp \
		painting/qcolor_qws.cpp
}
