#ifndef QFONTENGINE_P_H
#define QFONTENGINE_P_H

#include "qtextengine_p.h"
#include "qfontdata_p.h"

enum IndicFeatures {
    InitFeature = 0x0001,
    NuktaFeature = 0x0002,
    AkhantFeature = 0x0004,
    RephFeature = 0x0008,
    BelowFormFeature = 0x0010,
    HalfFormFeature = 0x0020,
    PostFormFeature = 0x0040,
    VattuFeature = 0x0080,
    PreSubstFeature = 0x0100,
    BelowSubstFeature = 0x0200,
    AboveSubstFeature = 0x0400,
    PostSubstFeature = 0x0800,
    HalantFeature = 0x1000
};

#ifdef Q_WS_X11
#include <qt_x11.h>

#ifndef QT_NO_XFTFREETYPE
#include <freetype/freetype.h>
#include "ftxopen.h"
#endif

class QFontEngineBox : public QFontEngine
{
public:
    QFontEngineBox( int size );
    ~QFontEngineBox();

    Error stringToCMap( const QChar *str,  int len, glyph_t *glyphs, advance_t *advances, int *nglyphs ) const;

    void draw( QPainter *p, int x, int y, const glyph_t *glyphs,
	       const advance_t *advances, const offset_t *offsets, int numGlyphs, bool reverse );

    virtual QGlyphMetrics boundingBox( const glyph_t *glyphs,
				    const advance_t *advances, const offset_t *offsets, int numGlyphs );
    QGlyphMetrics boundingBox( glyph_t glyph );

    int ascent() const;
    int descent() const;
    int leading() const;
    int maxCharWidth() const;

    int cmap() const;
    const char *name() const;

    bool canRender( const QChar *string,  int len );

    Type type() const;
    inline int size() const { return _size; }

private:
    friend class QFontPrivate;
    int _size;
};


#ifndef QT_NO_XFTFREETYPE
class QTextCodec;

class QFontEngineXft : public QFontEngine
{
public:
    QFontEngineXft( XftFont *font, XftPattern *pattern, int cmap );
    ~QFontEngineXft();

    QOpenType *openType() const;

    Error stringToCMap( const QChar *str,  int len, glyph_t *glyphs, advance_t *advances, int *nglyphs ) const;

    void draw( QPainter *p, int x, int y, const glyph_t *glyphs,
	       const advance_t *advances, const offset_t *offsets, int numGlyphs, bool reverse );

    virtual QGlyphMetrics boundingBox( const glyph_t *glyphs,
				    const advance_t *advances, const offset_t *offsets, int numGlyphs );
    QGlyphMetrics boundingBox( glyph_t glyph );

    int ascent() const;
    int descent() const;
    int leading() const;
    int maxCharWidth() const;

    int cmap() const;
    const char *name() const;

    bool canRender( const QChar *string,  int len );

    Type type() const;

private:
    friend class QFontPrivate;
    XftFont *_font;
    XftPattern *_pattern;
    QOpenType *_openType;
    int _cmap;
};
#endif

class QFontEngineXLFD : public QFontEngine
{
public:
    QFontEngineXLFD( XFontStruct *fs, const char *name, const char *encoding, int cmap );
    ~QFontEngineXLFD();

    Error stringToCMap( const QChar *str,  int len, glyph_t *glyphs, advance_t *advances, int *nglyphs ) const;

    void draw( QPainter *p, int x, int y, const glyph_t *glyphs,
	       const advance_t *advances, const offset_t *offsets, int numGlyphs, bool reverse );

    virtual QGlyphMetrics boundingBox( const glyph_t *glyphs,
				    const advance_t *advances, const offset_t *offsets, int numGlyphs );
    QGlyphMetrics boundingBox( glyph_t glyph );

    int ascent() const;
    int descent() const;
    int leading() const;
    int maxCharWidth() const;

    int cmap() const;
    const char *name() const;

    bool canRender( const QChar *string,  int len );

    void setScale( double scale );
    Type type() const;

private:
    friend class QFontPrivate;
    XFontStruct *_fs;
    QCString _name;
    QTextCodec *_codec;
    float _scale; // needed for printing, to correctly scale font metrics for bitmap fonts
    int _cmap;
};

class QScriptItem;

#ifndef QT_NO_XFTFREETYPE
class QOpenType
{
public:
    QOpenType( FT_Face face );
    ~QOpenType();

    bool supportsScript( unsigned int script );

    void apply( unsigned int script, unsigned short *featuresToApply, QScriptItem *item, int stringLength );

private:
    bool loadTables( FT_ULong script);

    FT_Face face;
    TTO_GDEF gdef;
    TTO_GSUB gsub;
    TTO_GPOS gpos;
    FT_UShort script_index;
    FT_ULong current_script;
    unsigned short found_bits;
    unsigned short always_apply;
    bool hasGDef : 1;
    bool hasGSub : 1;
    bool hasGPos : 1;
};
#endif // QT_NO_XFTFREETYPE

#elif defined( Q_WS_MAC )

#if defined( Q_WS_MACX )
# define QMAC_FONT_ATSUI
#endif
class QMacFontInfo;
#include "qt_mac.h"

class QFontEngineMac : public QFontEngine
{
#if defined( QMAC_FONT_ATSUI ) && 0
    ATSFontMetrics *info;
#else
    FontInfo *info;
#endif
    short fnum;
    int psize;
    QMacFontInfo *internal_fi;
    friend class QFontPrivate; //SDM?
    friend class QMacSetFontInfo; //SDM?

public:
    QFontEngineMac() : QFontEngine(), info(NULL), fnum(-1), internal_fi(NULL) { cache_cost = 1; }

    Error stringToCMap( const QChar *str, int len, glyph_t *glyphs, advance_t *advances, int *nglyphs ) const;

    void draw( QPainter *p, int x, int y, const glyph_t *glyphs,
	       const advance_t *advances, const offset_t *offsets, int numGlyphs, bool reverse );

    QGlyphMetrics boundingBox( const glyph_t *glyphs,
			       const advance_t *advances, const offset_t *offsets, int numGlyphs );
    QGlyphMetrics boundingBox( glyph_t glyph );

    int ascent() const { return (int)info->ascent; }
    int descent() const { return (int)info->descent; }
    int leading() const { return (int)info->leading; }
#if defined( QMAC_FONT_ATSUI ) && 0
    int maxCharWidth() const { return (int)info->maxAdvanceWidth; }
#else
    int maxCharWidth() const { return info->widMax; }
#endif

    const char *name() const { return "null"; }

    bool canRender( const QChar *string,  int len );

    Type type() const { return QFontEngine::Mac; }
};

#elif defined( Q_WS_WIN )

class QOpenType;

class QFontEngineWin : public QFontEngine
{
public:
    QFontEngineWin( const char * name );

    QOpenType *openType() const;

    Error stringToCMap( const QChar *str, int len, glyph_t *glyphs, advance_t *advances, int *nglyphs ) const;

    void draw( QPainter *p, int x, int y, const glyph_t *glyphs,
	       const advance_t *advances, const offset_t *offsets, int numGlyphs, bool reverse );

    QGlyphMetrics boundingBox( const glyph_t *glyphs,
			       const advance_t *advances, const offset_t *offsets, int numGlyphs );
    QGlyphMetrics boundingBox( glyph_t glyph );

    int ascent() const;
    int descent() const;
    int leading() const;
    int maxCharWidth() const;

    const char *name() const;

    bool canRender( const QChar *string,  int len );

    Type type() const;
};

#if 0
class QFontEngineUniscribe : public QFontEngine
{
public:
    QFontEngineUniscribe( const char * name );
    ~QFontEngineUniscribe();

    QOpenType *openType() const;

    void draw( QPainter *p, int x, int y, const glyph_t *glyphs,
	       const advance_t *advances, const offset_t *offsets, int numGlyphs, advance_t *advances, bool reverse );

    virtual QGlyphMetrics boundingBox( const glyph_t *glyphs,
				    const advance_t *advances, const offset_t *offsets, int numGlyphs );
    QGlyphMetrics boundingBox( glyph_t glyph );

    int ascent() const;
    int descent() const;
    int leading() const;
    int maxCharWidth() const;

    const char *name() const;

    bool canRender( const QChar *string,  int len );

    Type type() const;

    void *scriptCache;
};
#endif

#endif // Q_WS_WIN

#endif
