#############################################################################
# Generated by tmake at 15:26, 1996/10/10
#     Project: t6.p
#    Template: e:\tmake\template\win\msvc\qtapp.t
#############################################################################

####### Directories

BASEDIR =	$(QTDIR)
INCDIR	=	$(BASEDIR)\include
LIBDIR	=	$(BASEDIR)\lib

####### Compiler

CFLAGS	=	-Zi -nologo
LFLAGS	=	$(LIBDIR)\qt.lib user32.lib gdi32.lib comdlg32.lib libc.lib
CC	=	cl
MOC	=	moc
LINK	=	link /DEBUG /SUBSYSTEM:windows /NODEFAULTLIB:libcd.lib

####### Files

HEADERS =	
SOURCES =	main.cpp
OBJECTS =	main.obj
SRCMETA =	
OBJMETA =	
TARGET	=	t6.exe

####### Implicit rules

.SUFFIXES: .cpp

.cpp.obj:
	$(CC) -c $(CFLAGS) -I$(INCDIR) -Fo$@ $<

####### Build rules

all: $(TARGET)

$(TARGET): $(OBJECTS) $(OBJMETA) $(LIBDIR)\qt.lib
	$(LINK) -OUT:$(TARGET) $(OBJECTS) $(OBJMETA) $(LFLAGS)

mocify: $(SRCMETA)

clean:
	del $(OBJECTS) $(OBJMETA) $(SRCMETA)

####### Compile

main.obj: main.cpp

