TEMPLATE    =app
CONFIG	    += qt warn_on debug
SOURCES	    += helpdemo.cpp main.cpp
HEADERS	    += helpdemo.h
FORMS	    = helpdemobase.ui

unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}

LIBS   += -lqassistantclient
REQUIRES=full-config
