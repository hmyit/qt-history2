#include "qtextlayout.h"

#include "scriptengine.h"
#include "scriptenginebasic.h"
#include "scriptenginearabic.h"

#include <stdlib.h>

#include "bidi.cpp"


ScriptItemArray::~ScriptItemArray()
{
    free( d );
}

void ScriptItemArray::resize( int s )
{
    int alloc = (s + 8) >> 3 << 3;
    d = (ScriptItemArrayPrivate *)realloc( d, sizeof( ScriptItemArrayPrivate ) +
		 sizeof( ScriptItem ) * alloc );
    d->alloc = alloc;
}

void ScriptItemArray::split( int pos )
{
    unsigned int itemToSplit;
    for ( itemToSplit = 0; itemToSplit < d->size && d->items[itemToSplit].position <= pos; itemToSplit++ )
	;
    itemToSplit--;
    if ( d->items[itemToSplit].position == pos )
	// already a split at the requested position
	return;

    if ( d->size == d->alloc )
	resize( d->size + 1 );

    int numMove = d->size - itemToSplit-1;
    if ( numMove > 0 )
	memmove( d->items + itemToSplit+2, d->items +itemToSplit+1, numMove*sizeof( ScriptItem ) );
    d->size++;
    d->items[itemToSplit+1] = d->items[itemToSplit];
    d->items[itemToSplit+1].position = pos;
//     qDebug("split at position %d itempos=%d", pos, itemToSplit );
}


CharAttributesArray::~CharAttributesArray()
{
    free( d );
}


ShapedItem::ShapedItem()
{
    d = new ShapedItemPrivate();
}

ShapedItem::~ShapedItem()
{
    if ( d )
	delete d;
}


const GlyphIndex *ShapedItem::glyphs() const
{
    return d->glyphs;

}

int ShapedItem::count() const
{
    return d->num_glyphs;
}

const Offset *ShapedItem::offsets() const
{
    return d->offsets;
}








ScriptEngine **scriptEngines = 0;

class TextLayoutQt : public TextLayout
{
public:

    void itemize( ScriptItemArray &items, const QString & ) const;

    void attributes( CharAttributesArray &attributes, const QString &string,
		     const ScriptItemArray &items, int item ) const;

    void shape( ShapedItem &shaped, const QFont &font, const QString &string,
		const ScriptItemArray &items, int item ) const;

    void position( ShapedItem &shaped ) const;

    int cursorToX( ShapedItem &shaped, int cpos, Edge edge ) const;
    int xToCursor( ShapedItem &shaped, int x ) const;

    int width( ShapedItem &shaped ) const;
    bool split( ScriptItemArray &items, int item, ShapedItem &shaped, CharAttributesArray &attrs, int width ) const;
};



static TextLayout *_instance = 0;

const TextLayout *TextLayout::instance()
{
    if ( !_instance ) {
	_instance = new TextLayoutQt();

        if ( !scriptEngines ) {
	    scriptEngines = (ScriptEngine **) malloc( QFont::NScripts * sizeof( ScriptEngine * ) );
	    scriptEngines[0] = new ScriptEngineBasic;
	    for ( int i = 1; i < QFont::NScripts; i++ )
		scriptEngines[i] = scriptEngines[0];
	    scriptEngines[QFont::Arabic] = new ScriptEngineArabic;
	}
    }
    return _instance;
}


void TextLayout::bidiReorder( int numRuns, const Q_UINT8 *levels, int *visualOrder ) const
{
    ::bidiReorder(numRuns, levels, visualOrder );
}


void TextLayoutQt::itemize( ScriptItemArray &items, const QString &string ) const
{
    if ( !items.d ) {
	int size = 1;
	items.d = (ScriptItemArrayPrivate *)malloc( sizeof( ScriptItemArrayPrivate ) +
						    sizeof( ScriptItem ) * size );
	items.d->alloc = size;
    }
    items.d->size = 0;

    bidiItemize( string, items, QChar::DirON );
}


void TextLayoutQt::attributes( CharAttributesArray &attrs, const QString &string,
			       const ScriptItemArray &items, int item ) const
{
    const ScriptItem &si = items[item];
    int from = si.position;
    item++;
    int len = ( item < items.size() ? items[item].position : string.length() ) - from;


    attrs.d = (CharAttributesArrayPrivate *)realloc( attrs.d, sizeof( CharAttributesArrayPrivate ) +
						     sizeof(CharAttributes)*len );

    scriptEngines[si.analysis.script]->charAttributes( string, from, len, attrs.d->attributes );
}

void TextLayoutQt::shape( ShapedItem &shaped, const QFont &f, const QString &string,
			 const ScriptItemArray &items, int item ) const
{
    const ScriptItem &si = items[item];
    QFont::Script script = (QFont::Script)si.analysis.script;
    int from = si.position;
    item++;
    int len = ( item < items.size() ? items[item].position : string.length() ) - from;

    shaped.d->string = string;
    shaped.d->from = from;
    shaped.d->length = len;
    shaped.d->fontEngine = f.engineForScript( script );
    shaped.d->analysis = si.analysis;

    if ( shaped.d->fontEngine && shaped.d->fontEngine != (FontEngineIface*)-1 )
	scriptEngines[script]->shape( &shaped );
}

void TextLayoutQt::position( ShapedItem &shaped ) const
{
    QFont::Script script = (QFont::Script)shaped.d->analysis.script;
    if ( shaped.d->fontEngine && shaped.d->fontEngine != (FontEngineIface*)-1 )
	scriptEngines[script]->position( &shaped );
}

int TextLayoutQt::cursorToX( ShapedItem &shaped, int cpos, Edge edge ) const
{
    ShapedItemPrivate *d = shaped.d;
    if ( cpos > d->length )
	cpos = d->length;
    if ( cpos < 0 )
	cpos = 0;

    int glyph_pos = cpos == d->length ? d->num_glyphs : d->logClusters[cpos];
    if ( edge == Trailing ) {
	// trailing edge is leading edge of next cluster
	while ( glyph_pos < d->num_glyphs && !d->glyphAttributes[glyph_pos].clusterStart )
	    glyph_pos++;
    }

    int x = 0;
    bool reverse = d->analysis.bidiLevel % 2;

    if ( reverse ) {
	for ( int i = d->num_glyphs-1; i >= glyph_pos; i-- )
	    x += d->advances[i].x;
    } else {
	for ( int i = 0; i < glyph_pos; i++ )
	    x += d->advances[i].x;
    }
//     qDebug("cursorToX: cpos=%d, gpos=%d x=%d", cpos, glyph_pos, x );
    return x;
}

int TextLayoutQt::xToCursor( ShapedItem &shaped, int x ) const
{
    ShapedItemPrivate *d = shaped.d;
    bool reverse = d->analysis.bidiLevel % 2;
    if ( x < 0 )
	return reverse ? d->length : 0;


    if ( reverse ) {
	int width = 0;
	for ( int i = 0; i < d->num_glyphs; i++ ) {
	    width += d->advances[i].x;
	}
	x = -x + width;
    }
    int cp_before = 0;
    int cp_after = 0;
    int x_before = 0;
    int x_after = 0;

    int lastCluster = 0;
    for ( int i = 1; i <= d->length; i++ ) {
	int newCluster = i < d->length ? d->logClusters[i] : d->num_glyphs;
	if ( newCluster != lastCluster ) {
	    // calculate cluster width
	    cp_before = cp_after;
	    x_before = x_after;
	    cp_after = i;
	    for ( int j = lastCluster; j < newCluster; j++ )
		x_after += d->advances[j].x;
	    // 		qDebug("cluster boundary: lastCluster=%d, newCluster=%d, x_before=%d, x_after=%d",
	    // 		       lastCluster, newCluster, x_before, x_after );
	    if ( x_after > x )
		break;
	    lastCluster = newCluster;
	}
    }

    bool before = (x - x_before) < (x_after - x);
//     qDebug("got cursor position for %d: %d/%d, x_ba=%d/%d using %d",
// 	   x, cp_before,cp_after,  x_before, x_after,  before ? cp_before : cp_after );

    return before ? cp_before : cp_after;
}


int TextLayoutQt::width( ShapedItem &shaped ) const
{
    int width = 0;
    for ( int i = 0; i < shaped.d->num_glyphs; i++ )
	width += shaped.d->advances[i].x;
    return width;
}

bool TextLayoutQt::split( ScriptItemArray &items, int item, ShapedItem &shaped, CharAttributesArray &attrs, int width ) const
{
//     qDebug("TextLayoutQt::split: item=%d, width=%d", item, width );
    // line breaks are always done in logical order
    ShapedItemPrivate *d = shaped.d;

    int lastBreak = 0;
    int lastWidth = 0;
    int w = 0;
    int lastCluster = 0;
    int clusterStart = 0;
    for ( int i = 1; i <= d->length; i++ ) {
	int newCluster = i < d->length ? d->logClusters[i] : d->num_glyphs;
	if ( newCluster != lastCluster ) {
	    // calculate cluster width
	    int x = 0;
	    for ( int j = lastCluster; j < newCluster; j++ )
		x += d->advances[j].x;
	    lastWidth += x;
	    if ( w + lastWidth > width )
		break;
	    lastCluster = newCluster;
	    clusterStart = i;
	    if ( attrs[i].softBreak ) {
		lastBreak = i;
		w += lastWidth;
		lastWidth = 0;
	    }
	}
    }

    if ( lastBreak == 0 )
	return FALSE;

    // we have the break position
    items.split( lastBreak + items[item].position );
    return TRUE;
}
