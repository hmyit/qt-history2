TEMPLATE	= subdirs
CONFIG		+= ordered

!contains( QT_PRODUCT, qt-(enterprise|internal) ) {
    message( "ActiveQt requires a Qt/Enterprise license." )
}
contains( QT_PRODUCT, qt-(enterprise|internal) ) {
    	
	SUBDIRS	= idc \
		  container \
		  control \
		  examples

#mingw dos not suport controls yet
	win32-g++:SUBDIRS -= idc \
		  control
}
