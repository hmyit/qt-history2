# ----------------------------------------------------------
# QMSNETCONFIGTYPE lib project generated by QMsNet
# ----------------------------------------------------------

TEMPLATE  = lib
LANGUAGE  = C++
CONFIG	 += qt warn_on debug QMSNETCONFIGADD
CONFIG   -= QMSNETCONFIGREMOVE

TARGET	  = QMSNETPROJECTNAME

unix {
    target.path=$$libs.path
    QMAKE_CFLAGS    += $$QMAKE_CFLAGS_SHLIB
    QMAKE_CXXFLAGS  += $$QMAKE_CXXFLAGS_SHLIB
    INSTALLS        += target
}

HEADERS   = QMSNETPROJECTNAME.h
SOURCES	  = QMSNETPROJECTNAME.cpp
