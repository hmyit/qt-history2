TEMPLATE 	= app
DESTDIR		= $$QT_BUILD_TREE/bin
TARGET		= designer
CONFIG 		-= moc

SOURCES		+= main.cpp
INCLUDEPATH	+= ../designer
LIBS	+= -ldesignercore -lqui -lqassistantclient -L$$QT_BUILD_TREE/lib
win32 {
   RC_FILE	= designer.rc
}
mac {
   RC_FILE	= designer.icns
   staticlib:CONFIG -= global_init_link_order #yuck
}


target.path=$$bins.path
INSTALLS        += target
