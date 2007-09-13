load(qttest_p4)

SOURCES += tst_qsslcertificate.cpp
win32:LIBS += -lws2_32
QT += network

exists($$(QTDIR)/src/network/qnetworkproxy.h) {
    DEFINES += TEST_QNETWORK_PROXY
}

TARGET = tst_qsslcertificate

win32 {
  CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
  }
}

DEFINES += QT_USE_USING_NAMESPACE

