/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qrichtext.cpp#37 $
**
** Implementation of the Qt classes dealing with rich text
**
** Created : 990101
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Troll Tech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees holding valid Qt Professional Edition licenses may use this
** file in accordance with the Qt Professional Edition License Agreement
** provided with the Qt Professional Edition.
**
** See http://www.troll.no/pricing.html or email sales@troll.no for
** information about the Professional Edition licensing, or see
** http://www.troll.no/qpl/ for QPL licensing information.
**
*****************************************************************************/
#include "qstylesheet.h"
#include "qrichtextintern.cpp"

#include <qtextstream.h>
#include <qapplication.h>
#include <qlayout.h>
#include <qpainter.h>

#include <qstack.h>
#include <stdio.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qlayout.h>
#include <qbitmap.h>
#include <qtimer.h>
#include <qimage.h>
#include <qdragobject.h>
#include <qdatetime.h>
#include <qdrawutil.h>
#include <limits.h>

class QTextTableCell : public QLayoutItem
{
public:
    QTextTableCell(QTextTable* table,
      int row, int column, 		
      const QMap<QString, QString> &attr,
      const QStyleSheetItem* style,
      const QTextCharFormat& fmt, const QString& context,
      const QMimeSourceFactory &factory, const QStyleSheet *sheet, const QString& doc, int& pos );
    ~QTextTableCell();
    QSize sizeHint() const ;
    QSize minimumSize() const ;
    QSize maximumSize() const ;
    QSizePolicy::ExpandData expanding() const;
    bool isEmpty() const;
    void setGeometry( const QRect& ) ;
    QRect geometry() const;

    bool hasHeightForWidth() const;
    int heightForWidth( int ) const;

    void realize();

    int row() const { return row_; }
    int column() const { return col_; }
    int rowspan() const { return rowspan_; }
    int colspan() const { return colspan_; }
    int stretch() const { return stretch_; }
    
    void selectAll();
    void clearSelection();


    void draw( int x, int y,
	       int ox, int oy, int cx, int cy, int cw, int ch,
	       QRegion& backgroundRegion, const QColorGroup& cg, const QTextOptions& to );

    QString anchorAt( int x, int y ) const;
    bool toggleSelection( const QPoint& from, const QPoint& to, QRect& reg );
    
private:

    QPainter* painter() const;
    QRect geom;
    QTextTable* parent;
    QRichText* richtext;
    QBrush* background;
    int row_;
    int col_;
    int rowspan_;
    int colspan_;
    int stretch_;
    int maxw;
    int minw;
    bool hasFixedWidth;
};

class QTextTable: public QTextCustomItem
{
public:
    QTextTable(const QMap<QString, QString> &attr);
    ~QTextTable();
    void realize( QPainter* );
    void draw(QPainter* p, int x, int y,
	      int ox, int oy, int cx, int cy, int cw, int ch,
	      QRegion& backgroundRegion, const QColorGroup& cg, const QTextOptions& to );

    bool noErase() const { return TRUE; };
    bool expandsHorizontally() const { return TRUE; }
    bool ownSelection() const { return TRUE; }
    void resize( QPainter*, int nwidth );
    QString anchorAt( QPainter* p, int x, int y );
    bool toggleSelection( QPainter* p, const QPoint& from, const QPoint& to, QRect& reg );

    void selectAll();
    void clearSelection();

private:
    QGridLayout* layout;
    QList<QTextTableCell> cells;

    friend class QTextTableCell;
    void addCell( QTextTableCell* cell );
    QPainter* painter;
    int cachewidth;
    int fixwidth;
    int cellpadding;
    int cellspacing;
    int border;
    int outerborder;
    int stretch;
    QTextTableCell* selection_owner;
};


void QTextOptions::erase( QPainter* p, const QRect& r ) const
{
    if ( !paper )
	return;
    if ( p->device()->devType() == QInternal::Printer )
	return;
    if ( paper->pixmap() )
	p->drawTiledPixmap( r, *paper->pixmap(),
			    QPoint(r.x()+offsetx, r.y()+offsety) );
    else
	p->fillRect(r, *paper );
}



QTextImage::QTextImage(const QMap<QString, QString> &attr, const QString& context,
		       const QMimeSourceFactory &factory)
{
    width = height = 0;
    if ( attr.contains("width") )
	width = attr["width"].toInt();
    if ( attr.contains("height") )
	height = attr["height"].toInt();

    reg = 0;
    QImage img;
    QString imageName = attr["source"];

    if (!imageName)
	imageName = attr["src"];

    if ( !imageName.isEmpty() ) {
	const QMimeSource* m =
			factory.data( imageName, context );
	if ( !m ) {
	    qWarning("QTextImage: no mimesource for %s", imageName.latin1() );
	}
	else {
	    if ( !QImageDrag::decode( m, img ) ) {
		qWarning("QTextImage: cannot decode %s", imageName.latin1() );
	    }
	}
    }

    if ( !img.isNull() ) {
	if ( width == 0 ) {
	    width = img.width();
	    if ( height != 0 ) {
		width = img.width() * height / img.height();
	    }
	}
	if ( height == 0 ) {
	    height = img.height();
	    if ( width != img.width() ) {
		height = img.height() * width / img.width();
	    }
	}

	if ( img.width() != width || img.height() != height ){
	    img = img.smoothScale(width, height);
	    width = img.width();
	    height = img.height();
	}
	pm.convertFromImage( img );
	if ( pm.mask() ) {
	    QRegion mask( *pm.mask() );
	    QRegion all( 0, 0, pm.width(), pm.height() );
	    reg = new QRegion( all.subtract( mask ) );
	}
    }

    if ( pm.isNull() && (width*height)==0 ) {
	width = height = 50;
    }

    place = PlaceInline;
    if ( attr["align"] == "left" )
	place = PlaceLeft;
    else if ( attr["align"] == "right" )
	place = PlaceRight;

}

QTextImage::~QTextImage()
{
}


QTextCustomItem::Placement QTextImage::placement()
{
    return place;
}

void QTextImage::draw(QPainter* p, int x, int y,
		    int ox, int oy, int , int , int , int ,
		       QRegion& backgroundRegion, const QColorGroup& cg, const QTextOptions& /*to*/)
{
    if ( pm.isNull() ) {
	p->fillRect( x-ox , y-oy, width, height,  cg.dark() );
	return;
    }
    QRect r( x-ox, y-oy, width, height );
    backgroundRegion = backgroundRegion.subtract( r );
    if ( reg ){
	QRegion tmp( *reg );
	tmp.translate( x-ox, y-oy );
	backgroundRegion = backgroundRegion.unite( tmp );
    }
    p->drawPixmap( x-ox , y-oy, pm );
}


QTextHorizontalLine::QTextHorizontalLine()
{
    height = 8;
}


QTextHorizontalLine::~QTextHorizontalLine()
{
}


void QTextHorizontalLine::draw(QPainter* p, int x, int y,
				//int ox, int oy, int cx, int cy, int cw, int ch,
				int ox, int oy, int , int , int , int ,
				QRegion&, const QColorGroup& cg, const QTextOptions& to )
{
    QRect rm( x-ox, y-oy, width, height);
    //QRect ra( cx-ox, cy-oy, cw,  ch);
    QRect r = rm; // ####.intersect( ra );
    to.erase( p, r );
    qDrawShadeLine( p, r.left()-1, y-oy+4, r.right()+1, y-oy+4, cg, TRUE );
}

//************************************************************************


QRichText::QRichText( const QString &doc, const QFont& font,
		      const QString& context,
		      int margin,  const QMimeSourceFactory* factory, const QStyleSheet* sheet  )
    :QTextParagraph( 0, new QTextFormatCollection(),
		      QTextCharFormat( font, Qt::black ), (base = new QStyleSheetItem(0,"") ) )
{
    contxt = context;

    // for access during parsing only
    factory_ = factory? factory : QMimeSourceFactory::defaultFactory();
    // for access during parsing only
    sheet_ = sheet? sheet : (QStyleSheet*)QStyleSheet::defaultSheet();

    int pos = 0;

    //set up base style

    base->setDisplayMode(QStyleSheetItem::DisplayInline);
    base->setFontFamily( font.family() );
    base->setFontItalic( font.italic() );
    base->setFontUnderline( font.underline() );
    base->setFontWeight( font.weight() );
    base->setFontSize( font.pointSize() );
    base->setLogicalFontSize( 3 );
    base->setMargin( QStyleSheetItem::MarginAll, margin );

    keep_going = TRUE;
    init( doc, pos );

    // clear references that are no longer needed
    factory_ = 0;
    sheet_ = 0;
}


// constructor for nested text in text (tables, etc.)
QRichText::QRichText( const QMap<QString, QString> &attr, const QString &doc, int& pos,
			const QStyleSheetItem* style, const QTextCharFormat& fmt,
			const QString& context,
			int margin,  const QMimeSourceFactory* factory, const QStyleSheet* sheet  )
    :QTextParagraph( 0, new QTextFormatCollection(),
	    QTextCharFormat( fmt ), ( base = new QStyleSheetItem(*style) ), attr )
{
    contxt = context;

    // for access during parsing only
    factory_ = factory? factory : QMimeSourceFactory::defaultFactory();
    // for access during parsing only
    sheet_ = sheet? sheet : (QStyleSheet*)QStyleSheet::defaultSheet();

    base->setMargin( QStyleSheetItem::MarginAll, margin );

    keep_going = FALSE;
    init( doc, pos );

     // clear references that are no longer needed
    factory_ = 0;
    sheet_ = 0;
}


void QRichText::init( const QString& doc, int& pos )
{
    if ( !flow_ )
	flow_ = new QTextFlow();

    nullstyle = sheet_->item("");

    valid = TRUE;
//     QTime before = QTime::currentTime();
    if ( !keep_going )
	parse(this, style, 0, format, doc, pos);
    else  do {
	parse(this, style, 0, format, doc, pos);
	// missplaced close tags may kick us out of the parser (auto
	// recover failure), simply jump over the tag and continue
	// until the very end
	int oldpos = pos;
	if ( pos < (int) doc.length()-1
	     && (hasPrefix(doc, pos, QChar('<')) ) ) {
	    if ( hasPrefix(doc, pos+1, QChar('/')) )
		(void) parseCloseTag( doc, pos );
	    else {
		QMap<QString, QString> attr;
		bool emptyTag = FALSE;
		(void) parseOpenTag( doc, pos, attr, emptyTag );
	    }
	}
	if ( pos == oldpos )
	    pos++;
    } while ( pos < (int) doc.length()-1 );
    //qDebug("parse time used: %d", ( before.msecsTo( QTime::currentTime() ) ) );
    b_cache = 0;
    selection_owner = 0;
}

QRichText::~QRichText()
{
    delete base;
    delete flow_;
}



void QRichText::dump()
{
}



bool QRichText::isValid() const
{
    return valid;
}



/*!
  Returns the context of the rich text document. If no context has been specified
  in the constructor, a null string is returned.
*/
QString QRichText::context() const
{
    return contxt;
}

#define ENSURE_ENDTOKEN     if ( curstyle->displayMode() == QStyleSheetItem::DisplayBlock \
		     || curstyle->displayMode() == QStyleSheetItem::DisplayListItem ){ \
		    (dummy?dummy:current)->text.append( "\n", fmt ); \
		}

#define CLOSE_TAG (void) eatSpace(doc, pos); \
		int recoverPos = pos; \
		valid = (hasPrefix(doc, pos, QChar('<')) \
			 && hasPrefix(doc, pos+1, QChar('/')) \
			 && eatCloseTag(doc, pos, tagname) ); \
		if (!valid) { \
		    pos = recoverPos; \
		    valid = TRUE; \
		    return TRUE; \
		}

#define PROVIDE_DUMMY	if ( current->child && !dummy ) { \
		    dummy = new QTextParagraph( current, formats, fmt,  nullstyle ); \
		    QTextParagraph* it = current->child; \
		    while ( it->next ) \
			it = it->next; \
		    it->next = dummy; \
		    dummy->prev = it; \
		}

void QRichText::append( const QString& txt, const QMimeSourceFactory* factory, const QStyleSheet* sheet  )
{
    // for access during parsing only
    factory_ = factory? factory : QMimeSourceFactory::defaultFactory();
    // for access during parsing only
    sheet_ = sheet? sheet : (QStyleSheet*)QStyleSheet::defaultSheet();
    int pos = 0;
    lastChild()->invalidateLayout(); // fix bottom border
    parse( this, style, 0, format, txt, pos );
    // clear references that are no longer needed
    factory_ = 0;
    sheet_ = 0;
}


bool QRichText::parse (QTextParagraph* current, const QStyleSheetItem* curstyle, QTextParagraph* dummy,
			QTextCharFormat fmt, const QString &doc, int& pos)
{
    bool pre = current->whiteSpaceMode() == QStyleSheetItem::WhiteSpacePre;
    if ( !pre )
	eatSpace(doc, pos);
    while ( valid && pos < int(doc.length() )) {
	int beforePos = pos;
	if (hasPrefix(doc, pos, QChar('<')) ){
	    if (hasPrefix(doc, pos+1, QChar('/'))) {
		ENSURE_ENDTOKEN;
		if ( curstyle->isAnchor() ) {
		    PROVIDE_DUMMY
		    (dummy?dummy:current)->text.append( '\0', fmt );
		}
		return TRUE;
	    }
	    QMap<QString, QString> attr;
	    bool emptyTag = FALSE;
	    QString tagname = parseOpenTag(doc, pos, attr, emptyTag);

	    const QStyleSheetItem* nstyle = sheet_->item(tagname);
 	    if ( nstyle && !nstyle->selfNesting() && ( tagname == curstyle->name() ) ) {
 		pos = beforePos;
		ENSURE_ENDTOKEN;
 		return FALSE;
 	    }

	    if ( nstyle && !nstyle->allowedInContext( curstyle ) ) {
		QString msg;
		msg.sprintf( "QText Warning: Document not valid ( '%s' not allowed in '%s' #%d)",
			 tagname.ascii(), current->style->name().ascii(), pos);
		sheet_->error( msg );
		pos = beforePos;
		ENSURE_ENDTOKEN;
		return FALSE;
	    }

	    // TODO was wenn wir keinen nstyle haben?
	    if ( !nstyle )
		nstyle = nullstyle;

	    QTextCustomItem* custom = sheet_->tag( tagname, attr, contxt, *factory_ , emptyTag );
	    if ( custom || tagname == "br") {
		PROVIDE_DUMMY
		if ( custom )
		    (dummy?dummy:current)->text.append( "", fmt.makeTextFormat( nstyle, attr, custom ) );
		else {// br
		    (dummy?dummy:current)->text.append( "\n", fmt ) ;
		    if ( !pre )
			eatSpace(doc, pos);
		}
	    }
	    else if ( tagname == "table" ) {
		QTextCharFormat nfmt( fmt.makeTextFormat( nstyle, attr ) );
		custom = parseTable( attr, nfmt, doc, pos );
		if ( custom ) {
		    PROVIDE_DUMMY
		    (dummy?dummy:current)->text.append( "", fmt.makeTextFormat( nstyle, attr, custom )  );
		    (dummy?dummy:current)->text.append( " ", fmt );
		}
		CLOSE_TAG
	    }
	    else if (nstyle->displayMode() == QStyleSheetItem::DisplayBlock
		|| nstyle->displayMode() == QStyleSheetItem::DisplayListItem
		|| nstyle->displayMode() == QStyleSheetItem::DisplayNone
		) {
		QTextParagraph* subparagraph = new QTextParagraph( current, formats, fmt.makeTextFormat(nstyle,attr), nstyle, attr );

		if ( current == this && !child && text.isEmpty() )
		    attributes_ = attr; // propagate attributes

		if ( !current->text.isEmpty() ){
		    dummy = new QTextParagraph( current, formats, fmt, nullstyle );
		    dummy->text = current->text;
		    dummy->text.append( "\n", fmt );
		    current->text.clear();
		    current->child = dummy;
		}

		bool recover = FALSE;
		if (parse( subparagraph, nstyle, 0, subparagraph->format, doc, pos) ) {
		    (void) eatSpace(doc, pos);
		    int recoverPos = pos;
		    valid = (hasPrefix(doc, pos, QChar('<'))
			     && hasPrefix(doc, pos+1, QChar('/'))
			     && eatCloseTag(doc, pos, tagname) );
		    // sloppy mode, warning was done in eatCloseTag
		    if (!valid) {
			pos = recoverPos;
			valid = TRUE;
			recover = TRUE;
		    }
		}
		if ( subparagraph->style->displayMode() == QStyleSheetItem::DisplayNone ) {
		    // delete invisible paragraphs
		    delete subparagraph;
		} else {
		    // connect visible paragraphs
		    if ( !current->child )
			current->child = subparagraph;
		    else {
			QTextParagraph* it = current->child;
			while ( it->next )
			    it = it->next;
			it->next = subparagraph;
			subparagraph->prev = it;
		    }
		    dummy = 0;
		}

		if ( recover )
		    return TRUE; // sloppy, we could return FALSE to abort
		(void) eatSpace(doc, pos);
	    }
	    else { // containers and empty tags
		// TODO: check empty tags and custom tags in stylesheet

		if ( parse( current, nstyle, dummy, fmt.makeTextFormat(nstyle, attr), doc, pos ) ) {
		    CLOSE_TAG
		}
	    }
	}
	else { // plain text
	    if ( current->child && !dummy ) {
		dummy = new QTextParagraph( current, formats, fmt,  nullstyle );
		QTextParagraph* it = current->child;
		while ( it->next )
		    it = it->next;
		it->next = dummy;
		dummy->prev = it;
	    }
 	    QString word = parsePlainText( doc, pos, pre, TRUE );
 	    if (valid){
		(dummy?dummy:current)->text.append( word, fmt );
		if (!pre && (doc.unicode())[pos] == '<')
		    (void) eatSpace(doc, pos);
	    }
	}
    }
    return TRUE;
}

static bool qt_is_cell_in_use( QList<QTextTableCell>& cells, int row, int col )
{
    for ( QTextTableCell* c = cells.first(); c; c = cells.next() ) {
	if ( row >= c->row() && row < c->row() + c->rowspan()
	     && col >= c->column() && col < c->column() + c->colspan() )
	    return TRUE;
    }
    return FALSE;
}

QTextCustomItem* QRichText::parseTable( const QMap<QString, QString> &attr, const QTextCharFormat &fmt, const QString &doc, int& pos )
{

    QTextTable* table = new QTextTable( attr );
    int row = -1;
    int col = -1;

    QString rowbgcolor;
    QString rowalign;
    QString tablebgcolor = attr["bgcolor"];

    QList<QTextTableCell> multicells;

    QString tagname;
    (void) eatSpace(doc, pos);
    while ( valid && pos < int(doc.length() )) {
	int beforePos = pos;
	if (hasPrefix(doc, pos, QChar('<')) ){
	    if (hasPrefix(doc, pos+1, QChar('/'))) {
		tagname = parseCloseTag( doc, pos );
		if ( tagname == "table" ) {
		    pos = beforePos;
		    return table;
		}
	    } else {
		QMap<QString, QString> attr2;
		bool emptyTag = FALSE;
		tagname = parseOpenTag( doc, pos, attr2, emptyTag );
		if ( tagname == "tr" ) {
		    rowbgcolor = attr2["bgcolor"];
		    rowalign = attr2["align"];
		    row++;
		    col = -1;
		}
		else if ( tagname == "td" || tagname == "th" ) {
		    col++;
		    while ( qt_is_cell_in_use( multicells, row, col ) ) {
			col++;
		    }

		    if ( row >= 0 && col >= 0 ) {
			const QStyleSheetItem* style = sheet_->item(tagname);
			if ( !attr2.contains("bgcolor") ) {
			    if (!rowbgcolor.isEmpty() )
				attr2["bgcolor"] = rowbgcolor;
			    else if (!tablebgcolor.isEmpty() )
				attr2["bgcolor"] = tablebgcolor;
			}
			if ( !attr2.contains("align") ) {
			    if (!rowalign.isEmpty() )
				attr2["align"] = rowalign;
			}
			QTextTableCell* cell  = new QTextTableCell( table, row, col,
			      attr2, style,
			      fmt.makeTextFormat( style, attr2, 0 ),
			      contxt, *factory_, sheet_, doc, pos );
			if ( cell->colspan() > 1 || cell->rowspan() > 1 )
			    multicells.append( cell );
// 			qDebug("add cell %d %d (%d %d )", row, col, cell->rowspan(), cell->colspan() );
			col += cell->colspan()-1;
		    }
		}
	    }

	} else {
	    ++pos;
	}
    }
    return table;
}




bool QRichText::eatSpace(const QString& doc, int& pos, bool includeNbsp )
{
    int old_pos = pos;
    while (pos < int(doc.length()) && (doc.unicode())[pos].isSpace() && ( includeNbsp || (doc.unicode())[pos] != QChar(0x00a0U) ) )
	pos++;
    return old_pos < pos;
}

bool QRichText::eat(const QString& doc, int& pos, QChar c)
{
    valid = valid && (bool) ((doc.unicode())[pos] == c);
    if (valid)
	pos++;
    return valid;
}

bool QRichText::lookAhead(const QString& doc, int& pos, QChar c)
{
    return ((doc.unicode())[pos] == c);
}


static QMap<QCString, QChar> *html_map = 0;
static void qt_cleanup_html_map()
{
    delete html_map;
    html_map = 0;
}

QMap<QCString, QChar> *htmlMap()
{
    if ( !html_map ){
	html_map = new QMap<QCString, QChar>;
	qAddPostRoutine( qt_cleanup_html_map );
  	html_map->insert("lt", '<');
  	html_map->insert("gt", '>');
  	html_map->insert("amp", '&');
  	html_map->insert("nbsp", 0x00a0U);
  	html_map->insert("aring", '�');
  	html_map->insert("oslash", '�');
  	html_map->insert("ouml", '�');
  	html_map->insert("auml", '�');
  	html_map->insert("uuml", '�');
  	html_map->insert("Ouml", '�');
  	html_map->insert("Auml", '�');
  	html_map->insert("Uuml", '�');
  	html_map->insert("szlig", '�');
  	html_map->insert("copy", '�');
  	html_map->insert("deg", '�');
  	html_map->insert("micro", '�');
  	html_map->insert("plusmn", '�');
  	html_map->insert("middot", '*');
    }
    return html_map;
}

QChar QRichText::parseHTMLSpecialChar(const QString& doc, int& pos)
{
    QCString s;
    pos++;
    int recoverpos = pos;
    while ( pos < int(doc.length()) && (doc.unicode())[pos] != ';' && !(doc.unicode())[pos].isSpace() && pos < recoverpos + 6) {
	s += (doc.unicode())[pos];
	pos++;
    }
    if ((doc.unicode())[pos] != ';' && !(doc.unicode())[pos].isSpace() ) {
	pos = recoverpos;
	return '&';
    }
    pos++;

    if ( s.length() > 1 && s[0] == '#') {
	return s.mid(1).toInt();
    }

    QMap<QCString, QChar>::Iterator it = htmlMap()->find(s);
    if ( it != htmlMap()->end() ) {
	return *it;
    }

    pos = recoverpos;
    return '&';
}

QString QRichText::parseWord(const QString& doc, int& pos, bool insideTag, bool lower)
{
    QString s;

    if ((doc.unicode())[pos] == '"') {
	pos++;
	while ( pos < int(doc.length()) && (doc.unicode())[pos] != '"' ) {
	    s += (doc.unicode())[pos];
	    pos++;
	}
	eat(doc, pos, '"');
    }
    else {
	static QString term = QString::fromLatin1("/>");
	while( pos < int(doc.length()) &&
	       ( !insideTag || ((doc.unicode())[pos] != '>' && !hasPrefix( doc, pos, term)) )
	       && (doc.unicode())[pos] != '<'
	       && (doc.unicode())[pos] != '='
	       && !(doc.unicode())[pos].isSpace())
	{
	    if ( (doc.unicode())[pos] == '&')
		s += parseHTMLSpecialChar( doc, pos );
	    else {
		s += (doc.unicode())[pos];
		pos++;
	    }
	}
	if (lower)
	    s = s.lower();
    }
    valid = valid && pos <= int(doc.length());

    return s;
}

QString QRichText::parsePlainText(const QString& doc, int& pos, bool pre, bool justOneWord)
{
    QString s;
    while( pos < int(doc.length()) &&
	   (doc.unicode())[pos] != '<' ) {
	if ((doc.unicode())[pos].isSpace() && (doc.unicode())[pos] != QChar(0x00a0U) ){

	    if ( pre ) {
		if ( (doc.unicode())[pos] == ' ' )
		    s += QChar(0x00a0U);
		else
		    s += (doc.unicode())[pos];
	    }
	    else { // non-pre mode: collapse whitespace except nbsp
		while ( pos+1 < int(doc.length() ) &&
			(doc.unicode())[pos+1].isSpace()  && (doc.unicode())[pos+1] != QChar(0x00a0U) )
		    pos++;

		s += ' ';
	    }
	    pos++;
	    if ( justOneWord )
		return s;
	}
	else if ( (doc.unicode())[pos] == '&')
		s += parseHTMLSpecialChar( doc, pos );
	else {
	    s += (doc.unicode())[pos];
	    pos++;
	}
    }
    valid = valid && pos <= int(doc.length());
    return s;
}


bool QRichText::hasPrefix(const QString& doc, int pos, QChar c)
{
    return valid && (doc.unicode())[pos] ==c;
}

bool QRichText::hasPrefix(const QString& doc, int pos, const QString& s)
{
    if ( pos + s.length() >= doc.length() )
	return FALSE;
    for (int i = 0; i < int(s.length()); i++) {
	if ((doc.unicode())[pos+i] != s[i])
	    return FALSE;
    }
    return TRUE;
}

QString QRichText::parseOpenTag(const QString& doc, int& pos,
				  QMap<QString, QString> &attr, bool& emptyTag)
{
    emptyTag = FALSE;
    pos++;
    if ( hasPrefix(doc, pos, '!') ) {
	if ( hasPrefix( doc, pos+1, "--")) {
	    pos += 3;
	    // eat comments
	    QString pref = QString::fromLatin1("-->");
	    while ( valid && !hasPrefix(doc, pos, pref )
			&& pos < int(doc.length()) )
		pos++;
	    if ( valid && hasPrefix(doc, pos, pref ) ) {
		pos += 3;
		eatSpace(doc, pos, TRUE);
	    }
	    else
		valid = FALSE;
	    emptyTag = TRUE;
	    return QString::null;
	}
	else {
	    // eat strange internal tags
	    while ( valid && !hasPrefix(doc, pos, QChar('>')) && pos < int(doc.length()) )
		pos++;
	    if ( valid && hasPrefix(doc, pos, QChar('>')) ) {
		pos++;
		eatSpace(doc, pos, TRUE);
	    }
	    else
		valid = FALSE;
	    return QString::null;
	}
    }

    QString tag = parseWord(doc, pos, TRUE, TRUE);
    eatSpace(doc, pos, TRUE);
    static QString term = QString::fromLatin1("/>");
    static QString s_true = QString::fromLatin1("true");

    while (valid && !lookAhead(doc, pos, '>')
	    && ! (emptyTag = hasPrefix(doc, pos, term) ))
    {
	QString key = parseWord(doc, pos, TRUE, TRUE);
	eatSpace(doc, pos, TRUE);
	if ( key.isEmpty()) {
	    // error recovery
	    while ( pos < int(doc.length()) && !lookAhead(doc, pos, '>'))
		pos++;
	    break;
	}
	QString value;
	if (hasPrefix(doc, pos, QChar('=')) ){
	    pos++;
	    eatSpace(doc, pos);
	    value = parseWord(doc, pos, TRUE, FALSE);
	}
	else
	    value = s_true;
	attr.insert(key, value );
	eatSpace(doc, pos, TRUE);
    }

    if (emptyTag) {
	eat(doc, pos, '/');
	eat(doc, pos, '>');
    }
    else
	eat(doc, pos, '>');

    return tag;
}

QString QRichText::parseCloseTag( const QString& doc, int& pos )
{
    pos++;
    pos++;
    QString tag = parseWord(doc, pos, TRUE, TRUE);
    eatSpace(doc, pos, TRUE);
    eat(doc, pos, '>');
    return tag;
}

bool QRichText::eatCloseTag(const QString& doc, int& pos, const QString& open)
{
    QString tag = parseCloseTag( doc, pos );
    if (!valid) {
	QString msg;
	msg.sprintf( "QText Warning: Document not valid ( '%s' not closing #%d)", open.ascii(), pos);
	sheet_->error( msg );
	valid = TRUE;
    }
    valid = valid && tag == open;
    if (!valid) {
	QString msg;
	msg.sprintf( "QText Warning: Document not valid ( '%s' not closed before '%s' #%d)",
		     open.ascii(), tag.ascii(), pos);
	sheet_->error( msg );
    }
    return valid;
}



// convenience function
void QRichText::draw(QPainter* p, int x, int y,
		      int ox, int oy, int cx, int cy, int cw, int ch,
		      QRegion& backgroundRegion, const QColorGroup& cg, const QTextOptions& to )
{
    QTextCursor tc( *this );
    QTextParagraph* b = this;
    while ( b ) {
	tc.gotoParagraph( p, b );
	do {
	    tc.makeLineLayout( p );
	    QRect geom( tc.lineGeometry() );
	    if ( geom.bottom()+y > cy && geom.top()+y < cy+ch )
		tc.drawLine( p, ox-x, oy-y, cx-x, cy-y, cw, ch, backgroundRegion, cg, to );
	}
	while ( tc.gotoNextLine( p ) );
	b = tc.paragraph->nextInDocument();
    }
    flow()->drawFloatingItems( p, ox-x, oy-y, cx-x, cy-y, cw, ch, backgroundRegion, cg, to );
}


// convenience function
void QRichText::doLayout( QPainter* p, int nwidth ) {
    QTextCursor fc( *this );
    invalidateLayout();
    flow()->initialize( nwidth );
    fc.initParagraph( p, this );
    fc.updateLayout( p );
}

// convenience function
QString QRichText::anchorAt( QPainter* p, int x, int y) const
{
    QTextCursor tc( *((QRichText*)this) );
    tc.gotoParagraph( p, (QRichText*)this );
    QTextParagraph* b = tc.paragraph;
    while ( b && tc.y() < y ) {
	if ( b && b->dirty ){
	    tc.initParagraph( p, b );
	    tc.updateLayout( p, y + 1 );
	}

	tc.gotoParagraph( p, b );

	if ( tc.y() + tc.paragraph->height > y ) {
	    do {
		tc.makeLineLayout( p );
		QRect geom( tc.lineGeometry() );
		if ( geom.contains( QPoint(x,y) ) ) {
		    tc.gotoLineStart( p );
		    while ( !tc.atEndOfLine() && geom.left() + tc.currentx + tc.currentoffsetx < x ) {
			tc.right( p );
		    }
		    if ( geom.left() + tc.currentx + tc.currentoffsetx > x )
			tc.left( p );
		    else
			return QString::null;
		    QTextCharFormat format( *tc.currentFormat() );
		    if ( format.customItem() ) {
			// custom items may have anchors as well
			int h = format.customItem()->height;
			int nx = x - geom.x() - tc.currentx;
			int ny = y - geom.y() - tc.base + h;
			QString anchor = format.customItem()->anchorAt( p, nx, ny );
			if ( !anchor.isNull() )
			    return anchor;
		    }
		    return format.anchorHref();
		}
	    }
	    while ( tc.gotoNextLine( p ) );
	}
	b = b->nextInDocument();
    }

    return QString::null;
}

bool QRichText::clearSelection()
{
    bool result = FALSE;
    QTextParagraph* b = this;
    while ( b->child )
	b = b->child;
    while ( b ) {
	if ( b->text.isSelected() ) {
	    result = TRUE;
	    b->text.clearSelection();
	    flow()->invalidateRect( QRect( 0, b->y, flow()->width, b->height ) );
	}
	b = b->nextInDocument();
    }
    return result;
}

QString QRichText::selectedText()
{
    QString result;
    QTextParagraph* b = this;
    while ( b->child )
	b = b->child;
    while ( b ) {
	int column = 0;
	if ( b->text.isSelected() ) {
	    for (int i = 0; i < b->text.length(); i++ ) {
		if ( b->text.isSelected( i ) ) {
		    QString& s = b->text.getCharAt( i );
		    if ( column + s.length() > 79 ) {
			result += '\n';
			column = 0;
		    }
		    result += s;
		    column += s.length();
		}
	    }
	    result += '\n';
	}
	b = b->nextInDocument();
    }
    return result;
}

void QRichText::selectAll()
{
    QTextParagraph* b = this;
    while ( b->child )
	b = b->child;
    while ( b ) {
	for (int i = 0; i < b->text.length(); i++ )
	    b->text.setSelected( i, TRUE );
	b = b->nextInDocument();
    }
}

QTextParagraph* QRichText::getParBefore( int y ) const
{
    QTextParagraph* b = dirty?0:b_cache;
    if ( !b ) {
	b = (QTextParagraph*)this;
	while ( b->child )
	    b = b->child;
    }
    while ( b->y > y  && b->prevInDocument() ) 
	b = b->prevInDocument();
    while ( b->y + b->height < y && b->nextInDocument() )
	b = b->nextInDocument();
    QRichText* that = (QRichText*) this;
    that->b_cache = b;
    return b;
}


bool QRichText::toggleSelection( QPainter* p, const QPoint& from, const QPoint& to )
{
    QTextCursor cursor( *this );
    cursor.gotoParagraph( p, b_cache );
    cursor.goTo( p, from.x(), from.y() );
    b_cache = cursor.paragraph;

    if ( to == from ) {
	selection_owner = 0;
	QTextCustomItem* c = cursor.currentFormat()?cursor.currentFormat()->customItem():0;
	if ( c && c->ownSelection() ) {
	    selection_owner = c;
	    cursor.paragraph->text.setSelected( -1, TRUE );
	}
    }
    if ( selection_owner ) {
	QRect geom( cursor.lineGeometry() );
	QPoint offset (geom.x() + cursor.currentx, geom.y() + cursor.base - selection_owner->height );
	QRect r;
	bool sel = selection_owner->toggleSelection( p, from - offset, to - offset, r );
	r.moveBy( offset.x(), offset.y() );
	flow()->invalidateRect( r );
	return sel;
    }
    
    cursor.split();
    QTextCursor oldc( cursor );
    cursor.goTo( p, to.x(), to.y() );
    if ( cursor.split() ) {
	if ( (oldc.paragraph == cursor.paragraph) && (oldc.current >= cursor.current) ) {
	    oldc.current++;
	    oldc.update( p );
	}
    }
    int oldy = oldc.y();
    int oldx = oldc.x();
    int oldh = oldc.height;
    int newy = cursor.y();
    int newx = cursor.x();
    int newh = cursor.height;

    bool sel = cursor.isSelected();
    if ( oldx == newx && oldy == newy && oldh == newh )
  	return sel;

    QTextCursor start( *this ), end( *this );
    bool oldIsFirst = (oldy < newy) || (oldy == newy && oldx <= newx);
    if ( oldIsFirst ) {
	start = oldc;
	end = cursor;
    } else {
	start = cursor;
	end = oldc;
    }

    while ( start.paragraph != end.paragraph ) {
	start.setSelected( !start.isSelected() );
	start.rightOneItem( p );
    }
    while ( !start.atEnd() && start.paragraph == end.paragraph && start.current < end.current ) {
	start.setSelected( !start.isSelected() );
	start.rightOneItem( p );
    }
    
    flow()->invalidateRect( QRect( 0, QMIN(oldy, newy),
				   flow()->width,
				   QMAX(oldy+oldh, newy+newh)-QMIN(oldy,newy) ) );
    return sel;
}


void QTextParagraph::invalidateLayout()
{
    dirty = TRUE;
    QTextParagraph* b = child;
    while ( b ) {
	b->dirty = TRUE;
	b->invalidateLayout();
	b = b->next;
    }
    if ( next && !next->dirty ) {
	next->invalidateLayout();
    }


    if ( parent ) {
	QTextParagraph* p = parent;
	while ( p && !p->next && p->parent )
	    p = p->parent;
	if ( p->next && !p->next->dirty)
	    p->next->invalidateLayout();
    }
}


QTextParagraph* QTextParagraph::realParagraph()
{
    // to work around dummy paragraphs
    if ( parent &&style->name().isEmpty() )
	return parent->realParagraph();
    return this;
}

QStyleSheetItem::ListStyle QTextParagraph::listStyle()
{
    QString s =  attributes()["type"];

    if ( !s )
	return style->listStyle();
    else {
	QCString sl = s.latin1();
	if ( sl == "1" )
	    return QStyleSheetItem::ListDecimal;
	else if ( sl == "a" )
	    return QStyleSheetItem::ListLowerAlpha;
	else if ( sl == "A" )
	    return QStyleSheetItem::ListUpperAlpha;
	sl = sl.lower();
	if ( sl == "square" )
	    return QStyleSheetItem::ListSquare;
	else if ( sl == "disc" )
		return QStyleSheetItem::ListDisc;
	else if ( sl == "circle" )
	    return QStyleSheetItem::ListCircle;
    }
    return style->listStyle();
}


int QTextParagraph::numberOfSubParagraph( QTextParagraph* subparagraph, bool onlyListItems)
{
    QTextParagraph* it = child;
    int i = 1;
    while ( it && it != subparagraph ) {
	if ( !onlyListItems || it->style->displayMode() == QStyleSheetItem::DisplayListItem )
	    ++i;
	it = it->next;
    }
    return i;
}

QTextParagraph::QTextParagraph( QTextParagraph* p, QTextFormatCollection* formatCol, const QTextCharFormat& fmt,
	      const QStyleSheetItem *stl, const QMap<QString, QString> &attr )
    : parent( p ), formats( formatCol ), format( fmt ), text( formatCol ), style ( stl ), attributes_( attr )
{
    init();
};


QTextParagraph::QTextParagraph( QTextParagraph* p, QTextFormatCollection* formatCol, const QTextCharFormat& fmt,
	      const QStyleSheetItem *stl )
    : parent( p ), formats( formatCol ), format( fmt ), text( formats ), style ( stl )
{
    init();
};


void QTextParagraph::init()
{
    formats->registerFormat( format );

    child = next = prev = 0;
    height = y = 0;
    dirty = TRUE;
    selected = FALSE;
    flow_ = 0;
    if ( parent )
	flow_ = parent->flow();

    align = QStyleSheetItem::Undefined;

    if ( attributes_.contains("align") ) {
 	QString s = attributes_["align"].lower();
 	if ( s  == "center" )
 	    align = Qt::AlignCenter;
 	else if ( s == "right" )
 	    align = Qt::AlignRight;
	else
	    align = Qt::AlignLeft;
    }
}

QTextParagraph::~QTextParagraph()
{
    formats->unregisterFormat( format );
    QTextParagraph* tmp = child;
    while ( child ) {
	tmp = child;
	child = child->next;
	delete tmp;
    }
}

QTextParagraph* QTextParagraph::nextInDocument()
{
    if ( next  ) {
	QTextParagraph* b = next;
 	while ( b->child )
 	    b = b->child;
	return b;
    }
    if ( parent ) {
	return parent->nextInDocument();
    }
    return 0;
}

QTextParagraph* QTextParagraph::prevInDocument()
{
    if ( prev ){
	QTextParagraph* b = prev;
	while ( b->child ) {
	    b = b->child;
	    while ( b->next )
		b = b->next;
	}
	return b;
    }
    if ( parent ) {
	return parent->prevInDocument();
    }
    return 0;
}


QTextParagraph* QTextParagraph::lastChild()
{
    if ( !child )
	return this;
    QTextParagraph* b = child;
    while ( b->next )
	b = b->next;
    return b->lastChild();
}


QTextFlow* QTextParagraph::flow()
{
    if ( !flow_ ) {
	qDebug("AUTSCH!!!!!!! no flow");
    }
    return flow_;
}



QTextRichString::QTextRichString( QTextFormatCollection* fmt )
    : formats( fmt )
{
    items = 0;
    len = 0;
    store = 0;
    selection = 0;
}

QTextRichString::~QTextRichString()
{
    for (int i = 0; i < len; ++i )
	formats->unregisterFormat( *items[i].format );
}

void QTextRichString::clearSelection()
{
    for (int i = 0; i < len; ++i ) {
	setSelected( i, FALSE );
    }
    selection = FALSE;
}

bool QTextRichString::isSelected() const
{
    return selection;
}


void QTextRichString::clear()
{
    for (int i = 0; i < len; ++i )
	formats->unregisterFormat( *items[i].format );
    if ( items )
	delete [] items;
    items = 0;
    len = 0;
    store = 0;
}


void QTextRichString::remove( int index, int len )
{
    for (int i = 0; i < len; ++i )
	formats->unregisterFormat( *formatAt( index + i ) );

    int olen = length();
    if ( index + len >= olen ) {		// range problems
	if ( index < olen ) {			// index ok
	    setLength( index );
	}
    } else if ( len != 0 ) {
// 	memmove( items+index, items+index+len,
// 		 sizeof(Item)*(olen-index-len) );
	for (int i = index; i < olen-len; ++i)
	    items[i] = items[i+len];
	setLength( olen-len );
    }
}

void QTextRichString::insert( int index, const QString& c, const QTextCharFormat& form )
{
    QTextCharFormat* f = formats->registerFormat( form );

    if ( index >= len ) {			// insert after end
	setLength( index+1 );
    } else {					// normal insert
//	int olen = len;
	int nlen = len + 1;
	setLength( nlen );
// 	memmove( items+index+1, items+index,
// 		 sizeof(Item)*(olen-index) );
	for (int i = len-1; i > index; --i)
	    items[i] = items[i-1];
    }

    items[index].c = c;
    items[index].format = f;
    items[index].width = -1;
}


QString& QTextRichString::getCharAt( int index )
{
    items[index].width = -1;
    return items[index].c;
}

void QTextRichString::setSelected( int index, bool selected )
{
    if ( index >= 0 ) {
	items[index].selected = selected;
	if ( items[index].format->customItem() ) {
	    if ( selected )
		items[index].format->customItem()->selectAll();
	    else
		items[index].format->customItem()->clearSelection();
	    
	}
    }
    if ( selected )
	selection = TRUE;
}

bool QTextRichString::isSelected( int index ) const
{
    return items[index].selected;
}


void QTextRichString::setBold( int index, bool b )
{
     if ( bold( index ) == b )
 	return ;
     QTextCharFormat* oldfmt = items[index].format;
     QFont f = oldfmt->font();
     QColor c = oldfmt->color();
     f.setBold( b );
     QTextCharFormat newfmt( f, c );
     formats->unregisterFormat( *oldfmt );
     items[index].format =formats->registerFormat( newfmt );
     items[index].width = -1;
}

bool QTextRichString::bold( int index ) const
{
    return items[index].format->font().bold();
}

void QTextRichString::setLength( int l )
{
    if ( l <= store ) {
	len = l; // TODO shrinking
	return;
    } else {
	store = QMAX( l*2, 40 );
	Item* newitems = new Item[store]; // TODO speedup using new char(....)
// 	if ( items )
// 	    memcpy( newitems, items, sizeof(Item)*len );
	for (int i = 0; i < len; ++i )
	    newitems[i] = items[i];
	if ( items )
	    delete [] items;
	items = newitems;
	len = l;
    }
}

QTextRichString::QTextRichString( const QTextRichString &other )
{
    formats = other.formats;
    len = other.len;
    items = 0;
    store = 0;
    if ( len ) {
	store = QMAX( len, 40 );
	items = new Item[ store ];
// 	memcpy( items, other.items, sizeof(Item)*len );
	for (int i = 0; i < len; ++i ) {
	    items[i] = other.items[i];
	    items[i].format->addRef();
	}
    }
}

QTextRichString& QTextRichString::operator=( const QTextRichString &other )
{
    clear();
    formats = other.formats;
    len = other.len;
    items = 0;
    store = 0;
    if ( len ) {
	items = new Item[ len ];
	store = len;
// 	memcpy( items, other.items, sizeof(Item)*len );
	for (int i = 0; i < len; ++i ) {
	    items[i] = other.items[i];
	    items[i].format->addRef();
	}
    }
    return *this;
}


QTextCursor::QTextCursor(QRichText& document )
{
    doc = &document;
    paragraph = doc;
    first = y_ = width = widthUsed = height = base = fill = 0;
    last = first - 1;
    current = currentx = currentoffset = currentoffsetx = 0;
    lmargin = rmargin = 0;
    static_lmargin = static_rmargin = 0;
    currentasc  = currentdesc = 0;
    xline_current = 0;
    xline = 0;
    xline_paragraph = 0;
    formatinuse = 0;
}
QTextCursor::~QTextCursor()
{
}


/*!
  Like gotoParagraph() but also initializes the paragraph
  (i.e. setting the cached values in the paragraph to the cursor's
  settings and not vice versa.

 */
void QTextCursor::initParagraph( QPainter* p, QTextParagraph* b )
{
	b->y = y_;
	b->height = 0;
	while ( b->child  ) {
	    b->child->y = b->y;
	    b = b->child;
	}
	gotoParagraph( p, b );
}

void QTextCursor::gotoParagraph( QPainter* p, QTextParagraph* b )
{
    if ( !b )
	return;
    while ( b->child ) {
	b = b->child;
    }

    paragraph = b;
    flow = paragraph->flow();
    if ( paragraph->text.isEmpty() )
	paragraph->text.append( " ", paragraph->format );

    first = y_ = width = widthUsed = height = base = fill = 0;
    last = first - 1;


    y_ =  b->y;
    int m =  b->topMargin();
    flow->adjustFlow( y_, widthUsed, m ) ;

    y_ += m;

    static_lmargin = paragraph->totalMargin( QStyleSheetItem::MarginLeft );
    static_rmargin = paragraph->totalMargin( QStyleSheetItem::MarginRight );
    static_labelmargin = paragraph->totalLabelMargin();

    width = flow->width;
    lmargin = flow->adjustLMargin( y_, static_lmargin );
    lmargin += static_labelmargin;
    rmargin = flow->adjustRMargin( y_, static_rmargin );

    current = 0;

    currentx = lmargin;
    currentoffset = 0;
    currentoffsetx = 0;
    updateCharFormat( p );

}

void QTextCursor::update( QPainter* p )
{
    int i = current;
    int io = currentoffset;
    gotoParagraph( p, paragraph );
    makeLineLayout( p );
    gotoLineStart( p );
    while ( current < i  && rightOneItem( p ) )
	;
    while ( currentoffset < io )
	right( p );
}


bool QTextCursor::gotoNextLine( QPainter* p )
{
    current = last;
    if ( atEnd() ) {
	current++;
	y_ += height + 1; // first pixel below us
	int m = paragraph->bottomMargin();
	QTextParagraph* nid = paragraph->nextInDocument();
	if ( nid ) {
	    m -= nid->topMargin();
	}
	if ( m > 0 ) {
	    flow->adjustFlow( y_, widthUsed, m ) ;
	    y_ += m;
	}
	width = flow->width;
	lmargin = flow->adjustLMargin( y_, static_lmargin );
	lmargin += static_labelmargin;
	rmargin = flow->adjustRMargin( y_, static_rmargin );
	paragraph->height = y() - paragraph->y; //####
	paragraph->dirty = FALSE;
	return FALSE;
    }
    current++;
    currentx = lmargin;
    y_ += height;
    width = flow->width;
    lmargin = flow->adjustLMargin( y_, static_lmargin );
    lmargin += static_labelmargin;
    rmargin = flow->adjustRMargin( y_, static_rmargin );

    height = 0;
    updateCharFormat( p );
    return TRUE;
}

void QTextCursor::gotoLineStart( QPainter* p )
{
    current = first;
    currentoffset = currentoffsetx = 0;
    currentx = lmargin + fill;
    updateCharFormat( p );
}


void QTextCursor::updateCharFormat( QPainter* p )
{
    if ( pastEnd() )
	return;
    QTextCharFormat* fmt = currentFormat();
    p->setFont( fmt->font() );
    p->setPen( fmt->color() );
    QFontMetrics fm( p->fontMetrics() );
    currentasc = fm.ascent();
    currentdesc = fm.descent();
    QTextCustomItem* custom = fmt->customItem();
    if ( custom ) {
	if ( custom->width < 0 ) {
	    custom->realize( p );
	}
	if ( width >= 0 && custom->expandsHorizontally() ) {
	    custom->resize( p, width - lmargin - rmargin - fm.width(' ' ) );
	}
	if ( custom->placeInline() )
	    currentasc = custom->height;
    }
    formatinuse = fmt;
}


void QTextCursor::drawLabel( QPainter* p, QTextParagraph* par, int x, int y, int w, int h, int ox, int oy,
			      QRegion& backgroundRegion,
			      const QColorGroup& cg, const QTextOptions& to )

{
    if ( !par->parent )
	return;
    if ( par->style->displayMode() != QStyleSheetItem::DisplayListItem )
	return;

    QRect r (x - ox, y-oy, w, h ); ///#### label width?
    to.erase( p, r );
    backgroundRegion = backgroundRegion.subtract( r );
    QStyleSheetItem::ListStyle s = par->parent->listStyle();

    QFont font = p->font();
    p->setFont( par->parent->format.font() );
    int size = p->fontMetrics().lineSpacing() / 3;
    if ( size > 12 )
	size = 12;

    switch ( s ) {
    case QStyleSheetItem::ListDecimal:
    case QStyleSheetItem::ListLowerAlpha:
    case QStyleSheetItem::ListUpperAlpha:
	{
	    int n = 1;
	    n = par->parent->numberOfSubParagraph( par, TRUE );
	    QString l;
	    switch ( s ) {
	    case QStyleSheetItem::ListLowerAlpha:
		if ( n < 27 ) {
		    l = QChar( ('a' + (char) (n-1)));
		    break;
		}
	    case QStyleSheetItem::ListUpperAlpha:
		if ( n < 27 ) {
		    l = QChar( ('A' + (char) (n-1)));
		    break;
		}
		break;
	    default:  //QStyleSheetItem::ListDecimal:
		l.setNum( n );
		break;
	    }
	    l += QString::fromLatin1(". ");
	    p->drawText( r, Qt::AlignRight|Qt::AlignVCenter, l);
	}
	break;
    case QStyleSheetItem::ListSquare:
	{
	    QRect er( r.right()-size*2, r.center().y()-1, size, size);
	    p->fillRect( er , cg.brush( QColorGroup::Foreground ) );
	}
	break;
    case QStyleSheetItem::ListCircle:
	{
	    QRect er( r.right()-size*2, r.center().y()-1, size, size);
	    p->drawEllipse( er );
	}
	break;
    case QStyleSheetItem::ListDisc:
    default:
	{
	    p->setBrush( cg.brush( QColorGroup::Foreground ));
	    QRect er( r.right()-size*2, r.center().y()-1, size, size );
	    p->drawEllipse( er );
	    p->setBrush( Qt::NoBrush );
	}
	break;
    }

    p->setFont( font );
}

void QTextCursor::drawLine( QPainter* p, int ox, int oy,
			     int cx, int cy, int cw, int ch,
			     QRegion& backgroundRegion,
			     const QColorGroup& cg, const QTextOptions& to )

{
    gotoLineStart( p );

    if ( pastEndOfLine() ) {
	qDebug("try to draw empty line!!");
	return;
    }

    int gx = 0;
    int gy = y();

    int realWidth = QMAX( width, widthUsed );
    QRect r(gx-ox+lmargin, gy-oy, realWidth-lmargin-rmargin, height);

    bool clipMode = currentFormat()->customItem() && currentFormat()->customItem()->noErase();

    if (!clipMode )
	to.erase( p, r );

    if ( first == 0 ) {
	//#### TODO cache existence of label
	QTextParagraph*it = paragraph;
	int m = 0;
	while ( it &&
		( it->style->displayMode() == QStyleSheetItem::DisplayListItem
		  || it->prev == 0 ) ) {
	    int itm = it->labelMargin();
	    m += itm;
	    drawLabel( p, it, gx+lmargin-m, gy, itm, height, ox, oy, backgroundRegion, cg, to );
	    m += it->margin( QStyleSheetItem::MarginLeft );
	    if ( it->style->displayMode() == QStyleSheetItem::DisplayListItem && it->prev )
		it = 0;
	    else
		it = it->parent;
	}
    }

    QString c;
    while ( !atEndOfLine() ) {
	QTextCharFormat *format  = currentFormat();
	if ( !format->anchorHref().isEmpty() ) {
	    p->setPen( to.linkColor );
	    if ( to.linkUnderline ) {
		QFont f = format->font();
		f.setUnderline( TRUE );
		p->setFont( f );
	    }
	}
	bool selected = paragraph->text.isSelected(current);
	if ( selected ) {
	    int w = paragraph->text.items[current].width;
	    p->fillRect( gx-ox+currentx, gy-oy, w, height, cg.highlight() );
	    p->setPen( cg.highlightedText() );
	}

	QTextCustomItem* custom = format->customItem();
	if ( custom ) {
	    int h = custom->height;
	    if ( custom->placeInline() ) {
		custom->x = gx + currentx;
		custom->y = gy + base - h;
		custom->draw(p, custom->x, custom->y, ox, oy,
			     cx, cy, cw, ch, backgroundRegion, cg, to );
	    }
	}
	else {
	    c = paragraph->text.charAt( current );
	    int l = c.length();
	    while ( l>0 && c[l-1]=='\n' )
		l--;
	    p->drawText(gx+currentx-ox, gy-oy+base, c, l );
	}
	if ( selected )
	    p->setPen( format->color() );
	gotoNextItem( p );
	gy = y();
    }

    if (clipMode && p->device()->devType() != QInternal::Printer ) {
	p->setClipRegion( backgroundRegion );
	to.erase( p, r);
	p->setClipping( FALSE );
    }
    backgroundRegion = backgroundRegion.subtract(r);
}

bool QTextCursor::atEnd() const
{
    return current > paragraph->text.length()-2;
}


bool QTextCursor::pastEnd() const
{
    return current > paragraph->text.length() - 1;
}

bool QTextCursor::pastEndOfLine() const
{
    return current > last;
}

bool QTextCursor::atEndOfLine() const
{
    return current > last || (current == last && currentoffset >= int(paragraph->text.charAt( current ).length())-1);
}

bool QTextCursor::rightOneItem( QPainter* p )
{
    if ( atEnd() ) {
	QTextParagraph* next = paragraph->nextInDocument();
	if ( next ) {
	    if ( next->dirty ) {
		updateLayout( p );
	    }
	    gotoParagraph( p, next );
	    makeLineLayout( p );
	    gotoLineStart( p );
	} else {
	    return FALSE;
	}
    }
    else if ( current >= last ) {
	(void) gotoNextLine( p );
	makeLineLayout( p );
	gotoLineStart( p );
    }
    else {
	gotoNextItem( p );
    }
    return TRUE;
}

void QTextCursor::right( QPainter* p )
{
    if ( !pastEnd() && !pastEndOfLine() ) {
	QString c =  paragraph->text.charAt( current );
	if ( currentoffset  < int(c.length()) - 1 ) {
	    QTextCharFormat* fmt = currentFormat();
	    p->setFont( fmt->font() );
	    QFontMetrics fm( p->fontMetrics() );
	    currentoffset++;
	    currentoffsetx = fm.width( c, currentoffset );
	    return;
	}
    }
    (void) rightOneItem( p );
}

void QTextCursor::left( QPainter* p )
{
    QFontMetrics fm( p->fontMetrics() );
    if ( currentoffset > 0 ) {
	QString c =  paragraph->text.charAt( current );
	QTextCharFormat* fmt = currentFormat();
	p->setFont( fmt->font() );
	currentoffset--;
	currentoffsetx = fm.width( c, currentoffset );
    }
    else if ( current == 0 ) {
	if ( paragraph->prevInDocument() ) {
	    gotoParagraph( p, paragraph->prevInDocument() );
	    makeLineLayout( p );
	    gotoLineStart( p );
	    while ( !atEnd() )
		(void) rightOneItem( p );
	}
    }
    else {
	int i = current;
	if ( current == first ) {
	    gotoParagraph( p, paragraph );
	    makeLineLayout( p );
	}
	gotoLineStart( p );
	while ( current <  i - 1 )
	    (void) rightOneItem( p );
	QString c =  paragraph->text.charAt( current );
	if ( c.length() > 1 ) {
	    currentoffset = c.length() - 1;
	    QTextCharFormat* fmt = currentFormat();
	    p->setFont( fmt->font() );
	    QFontMetrics fm( p->fontMetrics() );
	    currentoffsetx = fm.width( c, currentoffset );
	}
    }
}


void QTextCursor::up( QPainter* p )
{
    if ( xline_paragraph != paragraph || xline_current != current )
	xline = x();
    gotoLineStart( p );
    left( p );
    gotoLineStart( p );
    while ( !atEndOfLine() && x()  < xline ) {
	right( p );
    }
    xline_paragraph = paragraph;
    xline_current = current;
}

void QTextCursor::down( QPainter* p )
{
    if ( xline_paragraph != paragraph || xline_current != current )
	xline = x();
    while ( current < last )
	(void) rightOneItem( p );
    (void) rightOneItem( p );
    while ( !atEndOfLine() && x() < xline ) {
	right( p );
    }

    xline_paragraph = paragraph;
    xline_current = current;
}


void QTextCursor::goTo( QPainter* p, int xpos, int ypos )
{
    while ( y() > ypos && paragraph->prevInDocument() ) 
	gotoParagraph( p, paragraph->prevInDocument() );
//     gotoParagraph( p, doc );
    QTextParagraph* b = paragraph;
    while ( b ) {
	gotoParagraph( p, b );
	b = paragraph;
	b = b->nextInDocument();
	//??? update dirty stuff here?
	if ( !b || y() + paragraph->height  > ypos ) {
	    do {
		makeLineLayout( p );
		QRect geom( lineGeometry() );
		if ( ypos <= geom.bottom()  ) {
		    gotoLineStart( p );
 		    while ( !atEndOfLine() && geom.left() + x() < xpos ) {
 			right( p );
 		    }
  		    if ( geom.left() + x() > xpos )
  			left( p );
		    return;
		}
	    }
	    while ( gotoNextLine( p ) );
	}
    }
}


void QTextCursor::setSelected( bool selected )
{
    if ( current < paragraph->text.length() ) {
	if ( selected )
	    paragraph->selected = TRUE;
	paragraph->text.setSelected( current, selected );
    }
}

bool QTextCursor::isSelected() const
{
    if ( current < paragraph->text.length() )
	return ( paragraph->text.isSelected( current ) );
    return FALSE;
}

void QTextCursor::insert( QPainter* p, const QString& text )
{
    QFontMetrics fm( p->fontMetrics() );
    if ( paragraph->text.isCustomItem( current ) ) {
	paragraph->text.insert( current, text, currentFormat()->formatWithoutCustom() );
	current++;
	currentoffset = 0;
    }
    else {
	QString& sref = paragraph->text.getCharAt( current );
	for ( int i = 0; i < (int)text.length(); i++ ) {
	    sref.insert( currentoffset, text[i] );
	    if ( text[i] == ' ' && currentoffset < int(sref.length())-1 ) { // not isSpace() to ignore nbsp
		paragraph->text.insert( current+1, sref.mid( currentoffset+1 ),
					*currentFormat() );
		sref.truncate( currentoffset+1 );
		current++;
		currentoffset = 0;
	    }
	    else {
		currentoffset++;
	    }
	}
    }

    updateParagraph( p );
}

bool QTextCursor::split()
{
    QString s = paragraph->text.charAt( current );
    if ( currentoffset == 0 || currentoffset >= int(s.length()) ) {
	return FALSE;
    }
    bool sel = isSelected();
    paragraph->text.insert( current+1, s.mid( currentoffset ),
			    *currentFormat() );
    paragraph->text.getCharAt( current ).truncate( currentoffset );
    current++;
    currentoffset = 0;
    last++;
    setSelected( sel );
    return TRUE;
}

void QTextCursor::updateParagraph( QPainter* p )
{
     int ph = paragraph->height;

     int oldy = y_;
     int oldfirst = first;
     int oldlast = last;
     int oldcurrent = current;
     int prevliney = oldy;

     QTextCursor store ( *this );
     gotoParagraph( p, paragraph );
     do {
	 makeLineLayout( p );
	 if ( last < oldcurrent )
	     prevliney = y_;
     } while ( gotoNextLine( p ) );
     *this = store;

     update( p );

     int uy = QMIN( oldy, y_ );
     if ( current == first )
	 uy = QMIN( uy, prevliney );

     if ( ph != paragraph->height ) {
	 if ( paragraph->nextInDocument() )
	     paragraph->nextInDocument()->invalidateLayout();
	 flow->invalidateRect( QRect( QPoint(0, uy), QPoint(width, INT_MAX-1000) ) );
     } else if ( first == oldfirst && last == oldlast && current != first ) {
	 flow->invalidateRect( QRect( 0, uy, width, height ) );
     }
     else {
	 flow->invalidateRect( QRect( 0, uy, width, paragraph->height - (uy - paragraph->y ) ) );
     }
}


void QTextCursor::gotoNextItem( QPainter* p )
{
    if ( pastEnd() )
	return;
    // tabulators belong here
    QTextRichString::Item* item = &paragraph->text.items[current];
    QTextCustomItem* custom = item->format->customItem();
    if ( currentFormat() != formatinuse ){ // somebody may have changed the document
	updateCharFormat( p );
    }
    if ( custom ) {
	if ( custom->placeInline() )
	    currentx += custom->width;
    }
    else {
	QString c = item->c;
	QFontMetrics fm( p->fontMetrics() );
	if ( item->width < 0 ) {
	    item->width = fm.width( c );
	}
	currentx += item->width;
    }
    current++;
    currentoffset = currentoffsetx = 0;

    if ( current < paragraph->text.length() && !paragraph->text.haveSameFormat( current-1, current ) ) {
 	updateCharFormat( p );
    }
}

void QTextCursor::makeLineLayout( QPainter* p )
{
    first = current;

    if ( pastEnd() )
	return;

    last = first;
    int rh = 0;
    int rasc = 0;
    int rdesc = 0;

    // do word wrap
    int lastSpace =current;
    int lastHeight = rh;
    int lastWidth = 0;
    int lastAsc = rasc;
    int lastDesc = rdesc;
    bool noSpaceFound = TRUE;

    QTextCharFormat* fmt = currentFormat();
    int fmt_current = current;
    p->setFont( fmt->font() );
    QFontMetrics fm( p->fontMetrics() );
    int space_width = fm.width(' ');
    int fm_ascent = fm.ascent();
    int fm_height = fm.height();

    widthUsed = 0;

    QList<QTextCustomItem> floatingItems;

    while ( !pastEnd() ) {

	if ( !paragraph->text.haveSameFormat( fmt_current, current ) ) {
	    fmt = currentFormat();
	    fmt_current = current;
	    p->setFont( fmt->font() );
	    space_width = fm.width(' ');
	    fm_ascent = fm.ascent();
	    fm_height = fm.height();
	}

	QTextRichString::Item* item = &paragraph->text.items[current];

	QChar lastc;

	QTextCustomItem* custom = item->format->customItem();
	if ( !custom && !item->c.isEmpty() ) {
	    lastc = item->c[ (int) item->c.length()-1];
	}

	if ( custom && !custom->placeInline() ) {
	    floatingItems.append( custom );
	}

	bool custombreak = custom && custom->ownLine();

	if ( custombreak && current > first ) {
	    // break _before_ a custom expander
 	    noSpaceFound = TRUE;
 	    lastc = '\n'; // fake newline
	} else {
	    if ( currentasc + currentdesc > rh )
		rh = currentasc + currentdesc;
	    if ( currentasc > rasc )
		rasc = currentasc;
	    if ( currentdesc > rdesc )
		rdesc = currentdesc;

	    gotoNextItem( p );
	    if ( custombreak ) {
		// also break _behind_ a custom expander
		++current;
		lastc = '\n';
	    }
	}
	// if a wordbreak is possible and required, do it. Unless we
	// have a newline, of course. In that case we break after the
	// newline to avoid empty lines.
	if ( currentx > width - rmargin - space_width
	     && !noSpaceFound && lastc != '\n' )
	    break;

	// word break is possible (a) after a space, (b) after a
	// newline, (c) before expandable item  or (d) at the end of the paragraph.
	if ( noSpaceFound || lastc == ' ' || lastc == '\n' || current == paragraph->text.length() ){
	    lastSpace = current - 1;
	    lastHeight = rh;
	    lastAsc = rasc;
	    lastDesc = rdesc;
	    lastWidth = currentx;
 	    if ( lastc == ' ' )
 		noSpaceFound = FALSE;
	}
	if ( lastc == '\n' )
	    break;
    }

    last = lastSpace;
    if ( last == paragraph->text.length() ) { // can happen with break behind custom expanders
	last--;
    }

    rh = lastHeight;
    rasc = lastAsc;
    rdesc = lastDesc;

    height = QMAX(rh, rasc+rdesc+1);
    base = rasc;

    fill = 0;
    switch ( paragraph->alignment() ) {
    case Qt::AlignLeft:
	fill = 0;
	break;
    case Qt::AlignCenter:
	fill = (width - rmargin - lastWidth) / 2;
	break;
    case Qt::AlignRight:
	fill = width - rmargin - lastWidth;
	break;
    }

    current = lastSpace;//###

    int min = lastWidth ;
    if ( min + rmargin > widthUsed )
	widthUsed = min + rmargin;
    if ( widthUsed > width )
	fill = 0; // fall back to left alignment if there isn't sufficient space

    flow->adjustFlow( y_, widthUsed, height ) ;


    int fl = lmargin;
    int fr = width - rmargin;
    for ( QTextCustomItem* item = floatingItems.first(); item; item = floatingItems.next() ) {
	item->y = y_ + height;
	flow->adjustFlow( item->y, item->width, item->height );
	if ( item->placement() == QTextCustomItem::PlaceRight ) {
	    fr -= item->width;
	    item->x = fr;
	} else {
	    item->x = fl;
	    fl += item->width;
	}
	flow->registerFloatingItem( item, item->placement() == QTextCustomItem::PlaceRight );
    }
}


/*\internal

  Note: The cursor's paragraph needs to be initialized
 */
bool QTextCursor::updateLayout( QPainter* p, int ymax )
{
    QTextParagraph* b = paragraph;
    gotoParagraph( p, b );
    while ( b && ( ymax < 0 || y_ <= ymax ) ) {

	if ( !b->dirty )
	    y_ = b->y + b->height;
	else do {
		makeLineLayout( p );
	} while ( gotoNextLine( p ) );

	b = b->nextInDocument();
	if ( b ) {
	    if ( b->dirty ) {
		initParagraph( p, b );
	    } else {
		gotoParagraph( p, b );
	    }
	}
    }
    return b == 0;
}



void QTextCursor::draw(QPainter* p,  int ox, int oy, int cx, int cy, int cw, int ch)
{
    QRect r( caretGeometry() );
    if ( QMAX( r.left(), cx ) <= QMIN( r.right(), cx+cw ) &&
	 QMAX( r.top(), cy ) <= QMIN( r.bottom(), cy+ch ) ) {
	p->drawLine(r.left()-ox, r.top()-oy, r.left()-ox, r.bottom()-oy );
    }
}

QRect QTextCursor::caretGeometry() const
{
    return QRect( x(), y()+base-currentasc, 1, currentasc + currentdesc + 1 );
}

QPoint QTextCursor::clickPos() const
{
    return QPoint( x(), y() + height/2);
}

QRect QTextCursor::lineGeometry() const
{
    int realWidth = QMAX( width, widthUsed );
    return QRect( 0, y(), realWidth, height );
}


QTextFlow::QTextFlow()
{
    width = widthUsed = height = pagesize = 0;
}

QTextFlow::~QTextFlow()
{
}


void QTextFlow::initialize( int w)
{
    height = 0;
    width = w;
    widthUsed = 0;

    leftItems.clear();
    rightItems.clear();
}


int QTextFlow::adjustLMargin( int yp, int margin )
{
    QTextCustomItem* item = 0;
    for ( item = leftItems.first(); item; item = leftItems.next() ) {
	if ( yp >= item->y && yp < item->y + item->height )
	    margin = QMAX( margin, item->x + item->width + 4 );
    }
    return margin;
}

int QTextFlow::adjustRMargin( int yp, int margin )
{
    QTextCustomItem* item = 0;
    for ( item = rightItems.first(); item; item = rightItems.next() ) {
	if ( yp >= item->y && yp < item->y + item->height )
	    margin = QMAX( margin, width - item->x - 4 );
    }
    return margin;
}



void QTextFlow::adjustFlow( int  &yp, int w, int h, bool pages )
{
    if ( w > widthUsed )
	widthUsed = w;


    if ( pages && pagesize > 0 ) { // check pages
	int ty = yp;
	int yinpage = ty % pagesize;
 	if ( yinpage < 2 )
 	    yp += 2 - yinpage;
 	else
	    if ( yinpage + h > pagesize - 2 )
	    yp += ( pagesize - yinpage ) + 2;
    }

    if ( yp + h > height )
	height = yp + h;

}

void QTextFlow::registerFloatingItem( QTextCustomItem* item, bool right   )
{
    if ( right ) {
	if ( !rightItems.contains( item ) )
	    rightItems.append( item );
    }
    else if ( !leftItems.contains( item ) )
	leftItems.append( item );
}

void QTextFlow::drawFloatingItems(QPainter* p,
				   int ox, int oy, int cx, int cy, int cw, int ch,
				   QRegion& backgroundRegion, const QColorGroup& cg, const QTextOptions& to )
{
    QTextCustomItem* item = 0;
    for ( item = leftItems.first(); item; item = leftItems.next() ) {
	item->draw( p, item->x, item->y, ox, oy, cx, cy, cw, ch, backgroundRegion, cg, to );
    }

    for ( item = rightItems.first(); item; item = rightItems.next() ) {
	item->draw( p, item->x, item->y, ox, oy, cx, cy, cw, ch, backgroundRegion, cg, to );
    }
}




QTextTable::QTextTable(const QMap<QString, QString> & attr  )
{
    cells.setAutoDelete( TRUE );

    cellspacing = 2;
    if ( attr.contains("cellspacing") )
	cellspacing = attr["cellspacing"].toInt();
    cellpadding = 1;
    if ( attr.contains("cellpadding") )
	cellpadding = attr["cellpadding"].toInt();
    border = 0;
    if ( attr.contains("border" ) ) {
	QString s( attr["border"] );
	if ( s == "true" )
	    border = 1;
	else
	    border = attr["border"].toInt();
    }

    if ( border )
	cellspacing += 2;
    outerborder = cellspacing + border;
    layout = new QGridLayout( 1, 1, cellspacing );

    fixwidth = 0;
    stretch = 0;
    if ( attr.contains("width") ) {
	bool b;
	QString s( attr["width"] );
	int w = s.toInt( &b );
	if ( b ) {
	    fixwidth = w;
	} else {
 	    s = s.stripWhiteSpace();
 	    if ( s.length() > 1 && s[ (int)s.length()-1 ] == '%' )
		stretch = s.left( s.length()-1).toInt();
	}
    }

    cachewidth = 0;
    selection_owner = 0;
}

QTextTable::~QTextTable()
{
    delete layout;
}

void QTextTable::realize( QPainter* p)
{
    painter = p;
    for (QTextTableCell* cell = cells.first(); cell; cell = cells.next() )
	cell->realize();

    width = 0;
}

void QTextTable::draw(QPainter* p, int x, int y,
		       int ox, int oy, int cx, int cy, int cw, int ch,
		       QRegion& backgroundRegion, const QColorGroup& cg, const QTextOptions& to )
{
    painter = p;
    for (QTextTableCell* cell = cells.first(); cell; cell = cells.next() ) {
	if ( y + cell->geometry().top() < cy+ch && y + 2*outerborder + cell->geometry().bottom() > cy ) {
	    cell->draw( x+outerborder, y+outerborder, ox, oy, cx, cy, cw, ch, backgroundRegion, cg, to );
	    if ( border ) {
		const int w = 1;
		QRect r( x+outerborder+cell->geometry().x()-w-ox,
			 y+outerborder+cell->geometry().y()-w-oy,
			 cell->geometry().width()+2*w,
			 cell->geometry().height()+2*w);
		int s = cellspacing;
		p->fillRect( r.left()-s, r.top(), s, r.height(), cg.button() );
		p->fillRect( r.right(), r.top(), s, r.height(), cg.button() );
		p->fillRect( r.left()-s, r.top()-s, r.width()+2*s, s, cg.button() );
		p->fillRect( r.left()-s, r.bottom(), r.width()+2*s, s, cg.button() );
		qDrawShadePanel( p, r, cg, TRUE );
//  		QRect r2(x+outerborder-ox+cell->geometry().x()-w, y+outerborder-oy+cell->geometry().y()-w,
//  			cell->geometry().width()+2*w, cell->geometry().height()+2*w );
//  		backgroundRegion = backgroundRegion.subtract( r2 );

	    }
	}
    }
    if ( border ) {
	QRect r ( x-ox, y-oy, width, height );
	int s = border;
	p->fillRect( r.left(), r.top(), s, r.height(), cg.button() );
	p->fillRect( r.right()-s, r.top(), s, r.height(), cg.button() );
	p->fillRect( r.left(), r.top(), r.width(), s, cg.button() );
	p->fillRect( r.left(), r.bottom()-s, r.width(), s, cg.button() );
	qDrawShadePanel( p, r, cg, FALSE, border );
	backgroundRegion = backgroundRegion.subtract( r );
    }
}

void QTextTable::resize( QPainter* p, int nwidth )
{
    if ( nwidth == cachewidth )
	return;
    cachewidth = nwidth;
    painter = p;

    if ( stretch )
	nwidth = nwidth * stretch / 100;

    width = nwidth + 2*outerborder;
    layout->invalidate();
    int shw = layout->sizeHint().width() + 2*outerborder;
    int mw = layout->minimumSize().width() + 2*outerborder;
    if ( stretch )
	width = QMAX( mw, nwidth );
    else
	width = QMAX( mw, QMIN( nwidth, shw ) );

    if ( fixwidth )
	width = fixwidth;

    // play it again, Sam, to fix the stretches
    layout->invalidate();
    mw = layout->minimumSize().width() + 2*outerborder;
    width = QMAX( width, mw );

    int h = layout->heightForWidth( width-2*outerborder );
    layout->setGeometry( QRect(0, 0, width-2*outerborder, h)  );
    height = layout->geometry().height()+2*outerborder;
};

QString QTextTable::anchorAt( QPainter* p, int x, int y )
{
    painter = p;
    for (QTextTableCell* cell = cells.first(); cell; cell = cells.next() ) {
	if ( cell->geometry().contains( QPoint(x-outerborder,y-outerborder) ) )
	    return cell->anchorAt( x-outerborder, y-outerborder );
    }
    return QString::null;
}


bool QTextTable::toggleSelection( QPainter* p, const QPoint& from, const QPoint& to, QRect& reg )
{
    painter = p;
    if ( from == to )
	selection_owner = 0;
    
    if ( !selection_owner ) {
	for (QTextTableCell* cell = cells.first(); cell; cell = cells.next() ) {
	    if ( cell->geometry().contains( from - QPoint(outerborder, outerborder)) ) {
		selection_owner = cell;
		break;
	    }
	}
    }
    if ( selection_owner ) {
	QPoint offset( outerborder, outerborder );
	reg.moveBy( -outerborder, -outerborder );
	bool sel = selection_owner->toggleSelection( from - offset, to - offset, reg );
	reg.moveBy( outerborder, outerborder );
	return sel;
    }
    return FALSE;
}

void QTextTable::selectAll()
{
    for (QTextTableCell* cell = cells.first(); cell; cell = cells.next() ) {
	cell->selectAll();
    }
}
void QTextTable::clearSelection()
{
    for (QTextTableCell* cell = cells.first(); cell; cell = cells.next() ) {
	cell->clearSelection();
    }
}


void QTextTable::addCell( QTextTableCell* cell )
{
    cells.append( cell );
    layout->addMultiCell( cell, cell->row(), cell->row() + cell->rowspan()-1,
			  cell->column(), cell->column() + cell->colspan()-1 );
//     if ( cell->stretch() ) {
// 	qDebug("colstrech %d to %d", cell->column(), cell->stretch() );
// 	//### other columns when colspan
// 	layout->setColStretch( cell->column(), cell->stretch() );
//     }
}

QTextTableCell::QTextTableCell(QTextTable* table,
       int row, int column,
       const QMap<QString, QString> &attr,
       const QStyleSheetItem* style,
       const QTextCharFormat& fmt, const QString& context,
       const QMimeSourceFactory &factory, const QStyleSheet *sheet, const QString& doc, int& pos )
{
    maxw = QWIDGETSIZE_MAX;
    minw = 0;

    parent = table;
    row_ = row;
    col_ = column;
    stretch_ = 0;
    richtext = new QRichText( attr, doc, pos, style,
			      fmt, context, parent->cellpadding, &factory, sheet );
    rowspan_ = 1;
    colspan_ = 1;
    if ( attr.contains("colspan") )
	colspan_ = attr["colspan"].toInt();
    if ( attr.contains("rowspan") )
	rowspan_ = attr["rowspan"].toInt();

    background = 0;
    if ( attr.contains("bgcolor") ) {
	background = new QBrush(QColor( attr["bgcolor"] ));
    }

    hasFixedWidth = FALSE;
    if ( attr.contains("width") ) {
	bool b;
	QString s( attr["width"] );
	int w = s.toInt( &b );
	if ( b ) {
	    maxw = w;
	    minw = maxw;
	    hasFixedWidth = TRUE;
	} else {
 	    s = s.stripWhiteSpace();
 	    if ( s.length() > 1 && s[ (int)s.length()-1 ] == '%' )
		stretch_ = s.left( s.length()-1).toInt();
	}
    }

    parent->addCell( this );
}

QTextTableCell::~QTextTableCell()
{
    delete richtext;
}

QSize QTextTableCell::sizeHint() const
{
    //### see QLabel::sizeHint()
    return QSize(maxw,0).expandedTo( minimumSize() );
}

QSize QTextTableCell::minimumSize() const
{
    if ( stretch_ )
	return QSize( QMAX( minw, parent->width * stretch_ / 100 - 2*parent->cellspacing), 0);
    return QSize(minw,0);
}

QSize QTextTableCell::maximumSize() const
{
    return QSize( QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
}

QSizePolicy::ExpandData QTextTableCell::expanding() const
{
    return QSizePolicy::BothDirections;
}

bool QTextTableCell::isEmpty() const
{
    return FALSE;
}
void QTextTableCell::setGeometry( const QRect& r)
{
    if ( r.width() != richtext->flow()->width ) {
	richtext->doLayout( painter(), r.width() );
    }
    geom = r;
}
QRect QTextTableCell::geometry() const
{
    return geom;
}

bool QTextTableCell::hasHeightForWidth() const
{
    return TRUE;
}

int QTextTableCell::heightForWidth( int w ) const
{
    w = QMAX( minw, w ); //####PAUL SHOULD DO THAT

    if ( richtext->flow()->width != w ) {
	QTextTableCell* that = (QTextTableCell*) this;
	that->richtext->doLayout(painter(), w );
    }
    return richtext->flow()->height;
}

void QTextTableCell::realize()
{

    if ( hasFixedWidth )
	return;

    richtext->doLayout(painter(), QWIDGETSIZE_MAX );
    maxw = richtext->flow()->widthUsed + 6;
    richtext->doLayout(painter(), 0 );
    minw = richtext->flow()->widthUsed;
}

QPainter* QTextTableCell::painter() const
{
    return parent->painter;
}

void QTextTableCell::draw(int x, int y,
			int ox, int oy, int cx, int cy, int cw, int ch,
			QRegion& backgroundRegion, const QColorGroup& cg, const QTextOptions& to )
{
    if ( richtext->flow()->width != geom.width() )
	richtext->doLayout(painter(), geom.width() );

    QTextOptions o(to);
    if ( background )
	o.paper = background;

    QRect r(x-ox+geom.x(), y-oy+geom.y(), geom.width(), geom.height() );
    richtext->draw(painter(), x+geom.x(), y+geom.y(), ox, oy, cx, cy, cw, ch, backgroundRegion, cg, o );

     if ( painter()->device()->devType() != QInternal::Printer ) {
	painter()->setClipRegion( backgroundRegion );
	o.erase( painter(), r );
    }
    backgroundRegion = backgroundRegion.subtract( r );
}

QString QTextTableCell::anchorAt( int x, int y ) const
{
    return richtext->anchorAt( painter(), x - geometry().x(), y - geometry().y() );
}

bool QTextTableCell::toggleSelection( const QPoint& from, const QPoint& to, QRect& reg )
{
    bool sel = richtext->toggleSelection( painter(), from - geometry().topLeft(), to - geometry().topLeft() );
    QRect r = richtext->flow()->updateRect();
    r.moveBy( geometry().x(), geometry().y() );
    reg = reg.unite( r );
    richtext->flow()->validateRect();
    return sel;
}


void QTextTableCell::selectAll()
{
    richtext->selectAll();
}
void QTextTableCell::clearSelection()
{
    richtext->clearSelection();
}


QTextCharFormat::QTextCharFormat()
    : ref( 1 ), logicalFontSize( 3 ), stdPointSize( 12 ),
      custom( 0 )
{
}

QTextCharFormat::QTextCharFormat( const QTextCharFormat &format )
    : font_( format.font_ ), color_( format.color_ ),
      key( format.key ), ref( 1 ),
      logicalFontSize( format.logicalFontSize ),
      stdPointSize( format.stdPointSize ),
      anchor_href( format.anchor_href ),
      anchor_name( format.anchor_name ),
      parent(0), custom( format.custom )
{
}

QTextCharFormat::QTextCharFormat( const QFont &f, const QColor &c )
    : font_( f ), color_( c ), ref( 1 ), logicalFontSize( 3 ), stdPointSize( f.pointSize() ),
      parent(0), custom( 0 )
{
    createKey();
}



QTextCharFormat::~QTextCharFormat()
{
}



void QTextCharFormat::createKey()
{
    QTextOStream ts( &key );
    ts
	<< font_.pointSize() << "_"
	<< font_.weight() << "_"
	<< (int)font_.underline()
	<< (int) font_.italic()
	<< anchor_href << "_"
	<< anchor_name << "_"
	<< color_.pixel()
	<< font_.family() << "_"
	<<(ulong) custom;
}

QTextCharFormat &QTextCharFormat::operator=( const QTextCharFormat &fmt )
{
    font_ = fmt.font_;
    color_ = fmt.color_;
    key = fmt.key;
    ref = 1;
    logicalFontSize = fmt.logicalFontSize;
    stdPointSize = fmt.stdPointSize;
    anchor_href = fmt.anchor_href;
    anchor_name = fmt.anchor_name;
    custom = fmt.custom;
    return *this;
}

bool QTextCharFormat::operator==( const QTextCharFormat &format )
{
    return format.key == key;
}

int QTextCharFormat::addRef()
{
    return ++ref;
}

int QTextCharFormat::removeRef()
{
    return --ref;
}

QTextCharFormat QTextCharFormat::makeTextFormat( const QStyleSheetItem *style,
						   const QMap<QString,QString>& attr,
						   QTextCustomItem*  item ) const
{
    QTextCharFormat format(*this);
    format.custom = item;
    bool changed = FALSE;
    if ( style ) {
	if ( style->name() == "font") {

	    if ( attr.contains("color") )
		format.color_.setNamedColor( attr["color"] );
	    if ( attr.contains("size") ) {
		QString a = attr["size"];
		int n = a.toInt();
		if ( a[0] == '+' || a[0] == '-' )
		    n += format.logicalFontSize;
		format.logicalFontSize = n;
		format.font_.setPointSize( format.stdPointSize );
		style->styleSheet()->scaleFont( format.font_, format.logicalFontSize );
	    }
	    if ( attr.contains("face") ) {
		QString a = attr["face"];
		if ( a.contains(',') )
		    a = a.left( a.find(',') );
		format.font_.setFamily( a );
	    }
	} else {

	    if ( style->isAnchor() ) {
		format.anchor_href = attr["href"];
		format.anchor_name = attr["name"];
		changed = TRUE;
	    }

	    if ( style->fontWeight() != QStyleSheetItem::Undefined )
		format.font_.setWeight( style->fontWeight() );
	    if ( style->fontSize() != QStyleSheetItem::Undefined )
		format.font_.setPointSize( style->fontSize() );
	    else if ( style->logicalFontSize() != QStyleSheetItem::Undefined ) {
		format.logicalFontSize = style->logicalFontSize();
		format.font_.setPointSize( format.stdPointSize );
		style->styleSheet()->scaleFont( format.font_, format.logicalFontSize );
	    }
	    else if ( style->logicalFontSizeStep() != QStyleSheetItem::Undefined ) {
		format.logicalFontSize += style->logicalFontSizeStep();
		format.font_.setPointSize( format.stdPointSize );
		style->styleSheet()->scaleFont( format.font_, format.logicalFontSize );
	    }
	    if ( !style->fontFamily().isEmpty() )
		format.font_.setFamily( style->fontFamily() );
	    if ( style->color().isValid() )
		format.color_ = style->color();
	    if ( style->definesFontItalic() )
		format.font_.setItalic( style->fontItalic() );
	    if ( style->definesFontUnderline() )
		format.font_.setUnderline( style->fontUnderline() );
	}
    }

    if ( item || font_ != format.font_ || changed || color_ != format.color_) // slight performance improvement
	format.createKey();
    return format;
}


QTextCharFormat QTextCharFormat::formatWithoutCustom()
{
    QTextCharFormat fm( *this );
    fm.custom = 0;
    fm.createKey();
    return fm;
}

QTextFormatCollection::QTextFormatCollection()
    : cKey(199),lastRegisterFormat( 0 )
{
}


static int anchorcount = 0;
QTextCharFormat* QTextFormatCollection::registerFormat( const QTextCharFormat &format )
{
    if ( format.parent == this ) {
	QTextCharFormat* f = ( QTextCharFormat*) &format;
	f->addRef();
	lastRegisterFormat = f;
	return f;
    }

    if ( lastRegisterFormat ) {
        if ( format.key == lastRegisterFormat->key ) {
	    lastRegisterFormat->addRef();
	    return lastRegisterFormat;
        }
    }

    if ( format.isAnchor() ) {
	// fancy speed optimization: do _not_ share any anchors to keep the map smaller
	// see unregisterFormat()
	++anchorcount;
	lastRegisterFormat =  new QTextCharFormat( format );
	return lastRegisterFormat;
    }

    QTextCharFormat *fc = cKey[ format.key ];
    if ( fc ) {
	fc->addRef();
	lastRegisterFormat = fc;
	return fc;
    } else {
	QTextCharFormat *f = new QTextCharFormat( format );
	f->parent = this;
	cKey.insert( f->key, f );
	lastRegisterFormat = f;
	return f;
    }
}

void QTextFormatCollection::unregisterFormat( const QTextCharFormat &format )
{
    QTextCharFormat* f  = 0;

    if ( format.isAnchor() ) {
	// fancy speed optimization: do _not_ share any anchors to keep the map smaller
	// see registerFormat()
	f = (QTextCharFormat*)&format;
	int ref = f->removeRef();
	if ( ref <= 0 ) {
	    if ( f == lastRegisterFormat )
		lastRegisterFormat = 0;
	    --anchorcount;
	    delete f;
	}
	return;
    }

    if ( format.parent == this )
	f = ( QTextCharFormat*)&format;
    else
	f = cKey[ format.key ];

    if ( f ) {
	int ref = f->removeRef();
	if ( ref <= 0 ) {
	    if ( f == lastRegisterFormat )
		lastRegisterFormat = 0;
	    cKey.remove( format.key );
	    delete f;
	}
    }
}

