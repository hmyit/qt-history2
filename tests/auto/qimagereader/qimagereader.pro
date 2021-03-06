load(qttest_p4)
SOURCES += tst_qimagereader.cpp
MOC_DIR=tmp
QT += network
RESOURCES += qimagereader.qrc

!contains(QT_CONFIG, no-gif):DEFINES += QTEST_HAVE_GIF
!contains(QT_CONFIG, no-jpeg):DEFINES += QTEST_HAVE_JPEG
!contains(QT_CONFIG, no-mng):DEFINES += QTEST_HAVE_MNG
!contains(QT_CONFIG, no-tiff):DEFINES += QTEST_HAVE_TIFF

win32-msvc:QMAKE_CXXFLAGS -= -Zm200
win32-msvc:QMAKE_CXXFLAGS += -Zm800

DEFINES += QT_USE_USING_NAMESPACE

