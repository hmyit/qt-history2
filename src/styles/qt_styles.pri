# Qt styles module

styles {
	HEADERS +=$$STYLES_H/qstylefactory.h \
		  $$STYLES_H/qstyleinterface.h \
		  $$STYLES_H/qcommonstyle.h \
		  $$STYLES_H/qmotifstyle.h \
		  $$STYLES_H/qwindowsstyle.h \
		  $$STYLES_H/qcdestyle.h \
		  $$STYLES_H/qmotifplusstyle.h \
		  $$STYLES_H/qplatinumstyle.h \
		  $$STYLES_H/qsgistyle.h \
		  $$STYLES_H/qcompactstyle.h

	SOURCES +=$$STYLES_CPP/qstylefactory.cpp \
		  $$STYLES_CPP/qcommonstyle.cpp \
		  $$STYLES_CPP/qmotifstyle.cpp \
		  $$STYLES_CPP/qwindowsstyle.cpp \
		  $$STYLES_CPP/qcdestyle.cpp \
		  $$STYLES_CPP/qmotifplusstyle.cpp \
		  $$STYLES_CPP/qplatinumstyle.cpp \
		  $$STYLES_CPP/qsgistyle.cpp \
		  $$STYLES_CPP/qcompactstyle.cpp
}

!macx:DEFINES += QT_NO_STYLE_AQUA
macx {
	HEADERS +=$$STYLES_H/qaquastyle.h
	SOURCES +=$$STYLES_CPP/qaquastyle.cpp
}

!windows:DEFINES += QT_NO_STYLE_WINDOWSXP
windows {
	HEADERS +=$$STYLES_H/qwindowsxpstyle.h
	SOURCES +=$$STYLES_CPP/qwindowsxpstyle.cpp
}
