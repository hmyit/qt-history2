/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qpixmap.h#68 $
**
** Definition of QPixmap class
**
** Created : 940501
**
** Copyright (C) 1994-1996 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QPIXMAP_H
#define QPIXMAP_H

#include "qpaintd.h"
#include "qcolor.h"
#include "qshared.h"


class QPixmap : public QPaintDevice		// pixmap class
{
friend class QPaintDevice;
friend class QPainter;
public:
    enum ColorMode { Auto, Color, Mono };

    QPixmap();
    QPixmap( int w, int h,  int depth=-1 );
    QPixmap( const QSize &, int depth=-1 );
    QPixmap( const char *fileName, const char *format=0, ColorMode mode=Auto );
    QPixmap( const QPixmap & );
   ~QPixmap();

    QPixmap    &operator=( const QPixmap & );
    QPixmap    &operator=( const QImage	 & );

    bool	isNull()	const;

    int		width()		const { return data->w; }
    int		height()	const { return data->h; }
    QSize	size()		const { return QSize(data->w,data->h); }
    QRect	rect()		const { return QRect(0,0,data->w,data->h); }
    int		depth()		const { return data->d; }
    static int	defaultDepth();

    void	fill( const QColor &fillColor=white );
    void	fill( const QWidget *, int xofs, int yofs );
    void	fill( const QWidget *, const QPoint &ofs );
    void	resize( int width, int height );
    void	resize( const QSize & );

    const QBitmap *mask() const;
    void	setMask( const QBitmap & );
    QBitmap	reasonableMask() const;

    static  QPixmap  grabWindow( WId, int x=0, int y=0, int w=-1, int h=-1 );

    QPixmap	    xForm( const QWMatrix & ) const;
    static QWMatrix trueMatrix( const QWMatrix &, int w, int h );

    QImage	convertToImage() const;
    bool	convertFromImage( const QImage &, ColorMode mode=Auto );

    static const char *imageFormat( const char *fileName );
    bool	load( const char *fileName, const char *format=0,
		      ColorMode mode=Auto );
    bool	loadFromData( const uchar *buf, uint len,
			      const char *format=0,
			      ColorMode mode=Auto );
    bool	save( const char *fileName, const char *format ) const;

#if defined(_WS_WIN_) || defined(_WS_PM_)
    HANDLE	hbm()		const;
#endif

    int		serialNumber()	const;

    bool	isOptimized()	const;
    void	optimize( bool );
    static bool isGloballyOptimized();
    static void optimizeGlobally( bool );

    virtual void detach();

    bool	isQBitmap()	const;

protected:
    QPixmap( int w, int h, const uchar *data, bool isXbitmap );
    int		metric( int ) const;

#if defined(_WS_WIN_)
    HANDLE	allocMemDC();
    void	freeMemDC();
#endif

    struct QPixmapData : public QShared {	// internal pixmap data
	QCOORD	w, h;
	short	d;
	uint	dirty	 : 1;
	uint	optim	 : 1;
	uint	uninit	 : 1;
	uint	bitmap	 : 1;
	uint	selfmask : 1;
	int	ser_no;
	QBitmap *mask;
#if defined(_WS_WIN_)
	HANDLE	hbm;
	void   *bits;
#elif defined(_WS_PM_)
	HANDLE	hdcmem;
	HANDLE	hbm;
#elif defined(_WS_X11_)
	void   *ximage;
#endif
    } *data;

private:
    void	init( int, int, int );
    QPixmap	copy() const;
    static bool optimAll;
    friend void bitBlt( QPaintDevice *, int, int, const QPaintDevice *,
			int, int, int, int, RasterOp, bool );
};


inline bool QPixmap::isNull() const
{
#if defined(_WS_X11_)
    return hd == 0;
#else
    return data->hbm == 0;
#endif
}

inline void QPixmap::fill( const QWidget *w, const QPoint &ofs )
{
    fill( w, ofs.x(), ofs.y() );
}

inline void QPixmap::resize( const QSize &s )
{
    resize( s.width(), s.height() );
}

inline const QBitmap *QPixmap::mask() const
{
    return data->mask;
}

#if defined(_WS_WIN_) || defined(_WS_PM_)
inline HANDLE QPixmap::hbm() const
{
    return data->hbm;
}
#endif

inline int QPixmap::serialNumber() const
{
    return data->ser_no;
}

inline bool QPixmap::isOptimized() const
{
    return data->optim;
}

inline bool QPixmap::isQBitmap() const
{
    return data->bitmap;
}


/*****************************************************************************
  QPixmap stream functions
 *****************************************************************************/

QDataStream &operator<<( QDataStream &, const QPixmap & );
QDataStream &operator>>( QDataStream &, QPixmap & );


#endif // QPIXMAP_H
