# Qt graphics 

#mng support
mng {
	unix:LIBS	+= -lmng
	INCLUDEPATH        += 3rdparty/libmng
	HEADERS += $$KERNEL_H/qmngio.h
	SOURCES += $$KERNEL_CPP/qmngio.cpp

	!jpeg {
		message(mng support requires support for jpeg)
		CONFIG += jpeg
	}
}
!mng:DEFINES += QT_NO_IMAGEIO_MNG

#jpeg support..
HEADERS += $$KERNEL_H/qjpegio.h 
SOURCES += $$KERNEL_CPP/qjpegio.cpp
jpeg {
	INCLUDEPATH += 3rdparty/libjpeg
	SOURCES += 3rdparty/libjpeg/jcapimin.c \
		  3rdparty/libjpeg/jcapistd.c \
		  3rdparty/libjpeg/jccoefct.c \
		  3rdparty/libjpeg/jccolor.c \
		  3rdparty/libjpeg/jcdctmgr.c \
		  3rdparty/libjpeg/jchuff.c \
		  3rdparty/libjpeg/jcinit.c \
		  3rdparty/libjpeg/jcmainct.c \
		  3rdparty/libjpeg/jcmarker.c \
		  3rdparty/libjpeg/jcmaster.c \
		  3rdparty/libjpeg/jcomapi.c \
		  3rdparty/libjpeg/jcparam.c \
		  3rdparty/libjpeg/jcphuff.c \
		  3rdparty/libjpeg/jcprepct.c \
		  3rdparty/libjpeg/jcsample.c \
		  3rdparty/libjpeg/jctrans.c \
		  3rdparty/libjpeg/jdapimin.c \
		  3rdparty/libjpeg/jdapistd.c \
		  3rdparty/libjpeg/jdatadst.c \
		  3rdparty/libjpeg/jdatasrc.c \
		  3rdparty/libjpeg/jdcoefct.c \
		  3rdparty/libjpeg/jdcolor.c \
		  3rdparty/libjpeg/jddctmgr.c \
		  3rdparty/libjpeg/jdhuff.c \
		  3rdparty/libjpeg/jdinput.c \
		  3rdparty/libjpeg/jdmainct.c \
		  3rdparty/libjpeg/jdmarker.c \
		  3rdparty/libjpeg/jdmaster.c \
		  3rdparty/libjpeg/jdmerge.c \
		  3rdparty/libjpeg/jdphuff.c \
		  3rdparty/libjpeg/jdpostct.c \
		  3rdparty/libjpeg/jdsample.c \
		  3rdparty/libjpeg/jdtrans.c \
		  3rdparty/libjpeg/jerror.c \
		  3rdparty/libjpeg/jfdctflt.c \
		  3rdparty/libjpeg/jfdctfst.c \
		  3rdparty/libjpeg/jfdctint.c \
		  3rdparty/libjpeg/jidctflt.c \
		  3rdparty/libjpeg/jidctfst.c \
		  3rdparty/libjpeg/jidctint.c \
		  3rdparty/libjpeg/jidctred.c \
		  3rdparty/libjpeg/jmemmgr.c \
		  3rdparty/libjpeg/jquant1.c \
		  3rdparty/libjpeg/jquant2.c \
		  3rdparty/libjpeg/jutils.c \
		  3rdparty/libjpeg/jmemansi.c
}
system-jpeg:LIBS += -ljpeg
!jpeg:DEFINES += QT_NO_IMAGEIO_JPEG

#png support
HEADERS+=$$KERNEL_H/qpngio.h
SOURCES+=$$KERNEL_CPP/qpngio.cpp
png {
	INCLUDEPATH  += 3rdparty/libpng
	SOURCES	+= 3rdparty/libpng/png.c \
		  3rdparty/libpng/pngerror.c \
		  3rdparty/libpng/pngget.c \
		  3rdparty/libpng/pngmem.c \
		  3rdparty/libpng/pngpread.c \
		  3rdparty/libpng/pngread.c \
		  3rdparty/libpng/pngrio.c \
		  3rdparty/libpng/pngrtran.c \
		  3rdparty/libpng/pngrutil.c \
		  3rdparty/libpng/pngset.c \
		  3rdparty/libpng/pngtrans.c \
		  3rdparty/libpng/pngwio.c \
		  3rdparty/libpng/pngwrite.c \
		  3rdparty/libpng/pngwtran.c \
		  3rdparty/libpng/pngwutil.c 
}
!png:LIBS += -lpng

#zlib support
zlib {
	INCLUDEPATH       += 3rdparty/zlib
	SOURCES	+= 3rdparty/zlib/adler32.c \
		  3rdparty/zlib/compress.c \
		  3rdparty/zlib/crc32.c \
		  3rdparty/zlib/deflate.c \
		  3rdparty/zlib/gzio.c \
		  3rdparty/zlib/infblock.c \
		  3rdparty/zlib/infcodes.c \
		  3rdparty/zlib/inffast.c \
		  3rdparty/zlib/inflate.c \
		  3rdparty/zlib/inftrees.c \
		  3rdparty/zlib/infutil.c \
		  3rdparty/zlib/trees.c \
		  3rdparty/zlib/uncompr.c \
		  3rdparty/zlib/zutil.c
}
!zlib:LIBS += -lz

#use Qt gif
gif:DEFINES += QT_BUILTIN_GIF_READER

