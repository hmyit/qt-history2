message("QT_SOURCE_TREE" $$QT_SOURCE_TREE)

win: CONFIG += qaxserver 
else: CONFIG += qtestlib

SOURCES += tst_qaxserver.cpp
RC_FILE  = $$QT_SOURCE_TREE/src/activeqt/control/qaxserver.rc
DEF_FILE = $$QT_SOURCE_TREE/src/activeqt/control/qaxserver.def

TARGET = tst_qaxserver

