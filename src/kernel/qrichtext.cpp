/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qrichtext.cpp#37 $
**
** Implementation of the internal Qt classes dealing with rich text
**
** Created : 990101
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "qrichtext_p.h"
#include "qtextedit.h"
#include "qstringlist.h"
#include "qfont.h"
#include "qtextstream.h"
#include "qfile.h"
#include "qregexp.h"
#include "qapplication.h"
#include "qclipboard.h"
#include "qmap.h"
#include "qfileinfo.h"
#include "qstylesheet.h"
#include "qmime.h"
#include "qregexp.h"
#include "qimage.h"
#include "qdragobject.h"
#include "qpaintdevicemetrics.h"
#include "qpainter.h"
#include "qdrawutil.h"
#include "qcursor.h"

#include <stdlib.h>

//#define PARSER_DEBUG
//#define DEBUG_COLLECTION ---> also in qrichtext_p.h
//#define DEBUG_TABLE_RENDERING

static QMap<QChar, QStringList> *eCompletionMap = 0;

#if defined(PARSER_DEBUG)
static QString debug_indent;
#endif

static double scale_factor( double v )
{
    return v/96;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void QTextCommandHistory::addCommand( QTextCommand *cmd )
{
    if ( current < (int)history.count() - 1 ) {
	QList<QTextCommand> commands;
	commands.setAutoDelete( FALSE );

	for( int i = 0; i <= current; ++i ) {
	    commands.insert( i, history.at( 0 ) );
	    history.take( 0 );
	}

	commands.append( cmd );
	history.clear();
	history = commands;
	history.setAutoDelete( TRUE );
    } else {
	history.append( cmd );
    }

    if ( (int)history.count() > steps )
	history.removeFirst();
    else
	++current;
}

QTextCursor *QTextCommandHistory::undo( QTextCursor *c )
{
    if ( current > -1 ) {
	QTextCursor *c2 = history.at( current )->unexecute( c );
	--current;
	return c2;
    }
    return 0;
}

QTextCursor *QTextCommandHistory::redo( QTextCursor *c )
{
    if ( current > -1 ) {
	if ( current < (int)history.count() - 1 ) {
	    ++current;
	    return history.at( current )->execute( c );
	}
    } else {
	if ( history.count() > 0 ) {
	    ++current;
	    return history.at( current )->execute( c );
	}
    }
    return 0;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

QTextCursor *QTextDeleteCommand::execute( QTextCursor *c )
{
    QTextParag *s = doc->paragAt( id );
    if ( !s ) {
	qWarning( "can't locate parag at %d, last parag: %d", id, doc->lastParag()->paragId() );
	return 0;
    }

    cursor.setParag( s );
    cursor.setIndex( index );
    int len = text.length();
    doc->setSelectionStart( QTextDocument::Temp, &cursor );
    for ( int i = 0; i < len; ++i )
	cursor.gotoRight();
    doc->setSelectionEnd( QTextDocument::Temp, &cursor );
    doc->removeSelectedText( QTextDocument::Temp, &cursor );

    if ( c ) {
	c->setParag( s );
	c->setIndex( index );
    }

    return c;
}

QTextCursor *QTextDeleteCommand::unexecute( QTextCursor *c )
{
    QTextParag *s = doc->paragAt( id );
    if ( !s ) {
	qWarning( "can't locate parag at %d, last parag: %d", id, doc->lastParag()->paragId() );
	return 0;
    }

    cursor.setParag( s );
    cursor.setIndex( index );
    cursor.insert( text, TRUE );
    cursor.setParag( s );
    cursor.setIndex( index );
    if ( c ) {
	c->setParag( s );
	c->setIndex( index );
	for ( int i = 0; i < (int)text.length(); ++i )
	    c->gotoRight();
    }

    s = cursor.parag();
    while ( s ) {
	s->format();
	s->setChanged( TRUE );
	if ( s == c->parag() )
	    break;
	s = s->next();
    }

    return &cursor;
}

QTextFormatCommand::QTextFormatCommand( QTextDocument *d, int selId, QTextFormat *f, int flgs )
    : QTextCommand( d ), selection( selId ),  flags( flgs )
{
    format = d->formatCollection()->format( f );
}

QTextFormatCommand::~QTextFormatCommand()
{
    format->removeRef();
}

QTextCursor *QTextFormatCommand::execute( QTextCursor *c )
{
    doc->setFormat( selection, format, flags );
    return c;
}

QTextCursor *QTextFormatCommand::unexecute( QTextCursor *c )
{
    return c;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

QTextCursor::QTextCursor( QTextDocument *d )
    : doc( d ), ox( 0 ), oy( 0 )
{
    nested = FALSE;
    idx = 0;
    string = doc->firstParag();
    tmpIndex = -1;
}

int QTextCursor::totalOffsetX() const
{
    if ( !nested )
	return 0;
    QValueStack<int>::ConstIterator xit = xOffsets.begin();
    int xoff = ox;
    for ( ; xit != xOffsets.end(); ++xit )
	xoff += *xit;
    return xoff;
}

int QTextCursor::totalOffsetY() const
{
    if ( !nested )
	return 0;
    QValueStack<int>::ConstIterator yit = yOffsets.begin();
    int yoff = oy;
    for ( ; yit != yOffsets.end(); ++yit )
	yoff += *yit;
    return yoff;
}

void QTextCursor::gotoIntoNested( const QPoint &globalPos )
{
    push();
    ox = 0;
    int bl, y;
    string->lineHeightOfChar( idx, &bl, &y );
    oy = y + string->rect().y();
    nested = TRUE;
    QPoint p( globalPos.x() - offsetX(), globalPos.y() - offsetY() );
    string->at( idx )->customItem()->enterAt( doc, string, idx, ox, oy, p );
}

void QTextCursor::invalidateNested()
{
    if ( nested ) {
	QValueStack<QTextParag*>::Iterator it = parags.begin();
	QValueStack<int>::Iterator it2 = indices.begin();
	for ( ; it != parags.end(); ++it, ++it2 ) {
	    if ( *it == string )
		continue;
	    (*it)->invalidate( 0 );
	    if ( (*it)->at( *it2 )->isCustom )
		(*it)->at( *it2 )->customItem()->invalidate();
	}		
    }
}

void QTextCursor::insert( const QString &s, bool checkNewLine )
{
    tmpIndex = -1;
    bool justInsert = TRUE;
    if ( checkNewLine )
	justInsert = s.find( '\n' ) == -1;
    if ( justInsert ) {
	string->insert( idx, s );
	idx += s.length();
    } else {
	QStringList lst = QStringList::split( '\n', s, TRUE );
	QStringList::Iterator it = lst.begin();
	int y = string->rect().y() + string->rect().height();
	for ( ; it != lst.end(); ++it ) {
	    if ( it != lst.begin() ) {
		splitAndInsertEmtyParag( FALSE, FALSE );
		string->setEndState( -1 );
		string->prev()->format( -1, FALSE );
	    }
	    QString s = *it;
	    if ( s.isEmpty() )
		continue;
	    string->insert( idx, s );
	    idx += s.length();
	}
	string->format( -1, FALSE );
	int dy = string->rect().y() + string->rect().height() - y;
	QTextParag *p = string->next();
	while ( p ) {
	    p->setParagId( p->prev()->paragId() + 1 );
	    p->move( dy );
	    p->invalidate( 0 );
	    p->setEndState( -1 );
	    p = p->next();
	}
    }

    int h = string->rect().height();
    string->format( -1, TRUE );
    if ( h != string->rect().height() )
	invalidateNested();
    else if ( doc->parent() )
	doc->nextDoubleBuffered = TRUE;
}

void QTextCursor::gotoLeft()
{
    tmpIndex = -1;

    if ( idx > 0 ) {
	idx--;
    } else if ( string->prev() ) {
	string = string->prev();
	idx = string->length() - 1;
    } else {
	if ( nested ) {
	    pop();
	    processNesting( Prev );
	    if ( idx == -1 ) {
		pop();
		if ( idx > 0 ) {
		    idx--;
		} else if ( string->prev() ) {
		    string = string->prev();
		    idx = string->length() - 1;
		}
	    }
	}
    }

    if ( string->at( idx )->isCustom &&
	 string->at( idx )->customItem()->isNested() ) {
	processNesting( EnterEnd );
    }
}

void QTextCursor::push()
{
    indices.push( idx );
    parags.push( string );
    xOffsets.push( ox );
    yOffsets.push( oy );
    nestedStack.push( nested );
}

void QTextCursor::pop()
{
    idx = indices.pop();
    string = parags.pop();
    ox = xOffsets.pop();
    oy = yOffsets.pop();
    if ( doc->parent() )
	doc = doc->parent();
    nested = nestedStack.pop();
}

void QTextCursor::restoreState()
{
    while ( !indices.isEmpty() )
	pop();
}

void QTextCursor::place( const QPoint &pos, QTextParag *s )
{
    QRect r;
    while ( s ) {
	r = s->rect();
	r.setWidth( doc->width() );
	if ( r.contains( pos ) )
	    break;
	s = s->next();
    }

    if ( !s )
	return;

    setParag( s, FALSE );
    int y = s->rect().y();
    int lines = s->lines();
    QTextString::Char *chr = 0, *c2;
    int index;
    int i = 0;
    int cy;
    int ch;
    for ( ; i < lines; ++i ) {
	chr = s->lineStartOfLine( i, &index );
	cy = s->lineY( i );
	ch = s->lineHeight( i );
	if ( !chr )
	    return;
	if ( pos.y() >= y + cy && pos.y() <= y + cy + ch )
	    break;
    }

    c2 = chr;
    i = index;
    int x = s->rect().x(), last = index;
    int lastw = 0;
    int h = ch;
    int bl;
    int cw;
    while ( TRUE ) {
	if ( c2->lineStart )
	    h = s->lineHeightOfChar( i, &bl, &cy );
	last = i;
	cw = c2->width();
	if ( c2->isCustom && c2->customItem()->isNested() )
	    cw *= 2;
	if ( pos.x() >= x + c2->x - lastw && pos.x() <= x + c2->x + cw / 2 &&
	     pos.y() >= y + cy && pos.y() <= y + cy + h )
	    break;
	lastw = cw / 2;
	i++;
	if ( i < s->length() )
	    c2 = s->at( i );
	else
	    break;
    }

    setIndex( last, FALSE );

    if ( parag()->at( last )->isCustom && parag()->at( last )->customItem()->isNested() ) {
	gotoIntoNested( pos );
	QPoint p( pos.x() - offsetX(), pos.y() - offsetY() );
	place( p, document()->firstParag() );
    }
}

void QTextCursor::processNesting( Operation op )
{
    push();
    ox = 0;
    int bl, y;
    string->lineHeightOfChar( idx, &bl, &y );
    oy = y + string->rect().y();
    nested = TRUE;

    switch ( op ) {
    case EnterBegin:
	string->at( idx )->customItem()->enter( doc, string, idx, ox, oy );
	break;
    case EnterEnd:
	string->at( idx )->customItem()->enter( doc, string, idx, ox, oy, TRUE );
	break;
    case Next:
	string->at( idx )->customItem()->next( doc, string, idx, ox, oy );
	break;
    case Prev:
	string->at( idx )->customItem()->prev( doc, string, idx, ox, oy );
	break;
    case Down:
	string->at( idx )->customItem()->down( doc, string, idx, ox, oy );
	break;
    case Up:
	string->at( idx )->customItem()->up( doc, string, idx, ox, oy );
	break;
    }
}

void QTextCursor::gotoRight()
{
    tmpIndex = -1;

    if ( string->at( idx )->isCustom &&
	 string->at( idx )->customItem()->isNested() ) {
	processNesting( EnterBegin );
	return;
    }

    if ( idx < string->length() - 1 ) {
	idx++;
    } else if ( string->next() ) {
	string = string->next();
	idx = 0;
    } else {
	if ( nested ) {
	    pop();
	    processNesting( Next );
	    if ( idx == -1 ) {
		pop();
		if ( idx < string->length() - 1 ) {
		    idx++;
		} else if ( string->next() ) {
		    string = string->next();
		    idx = 0;
		}
	    }
	}
    }
}

void QTextCursor::gotoUp()
{
    int indexOfLineStart;
    int line;
    QTextString::Char *c = string->lineStartOfChar( idx, &indexOfLineStart, &line );
    if ( !c )
	return;

    tmpIndex = QMAX( tmpIndex, idx - indexOfLineStart );
    if ( indexOfLineStart == 0 ) {
	if ( !string->prev() ) {
	    if ( !nested )
		return;
	    pop();
	    processNesting( Up );
	    if ( idx == -1 ) {
		pop();
		if ( !string->prev() )
		    return;
		idx = tmpIndex = 0;
	    } else {
		tmpIndex = -1;
		return;
	    }
	}
	string = string->prev();
	int lastLine = string->lines() - 1;
	if ( !string->lineStartOfLine( lastLine, &indexOfLineStart ) )
	    return;
	if ( indexOfLineStart + tmpIndex < string->length() )
	    idx = indexOfLineStart + tmpIndex;
	else
	    idx = string->length() - 1;
    } else {
	--line;
	int oldIndexOfLineStart = indexOfLineStart;
	if ( !string->lineStartOfLine( line, &indexOfLineStart ) )
	    return;
	if ( indexOfLineStart + tmpIndex < oldIndexOfLineStart )
	    idx = indexOfLineStart + tmpIndex;
	else
	    idx = oldIndexOfLineStart - 1;
    }
}

void QTextCursor::gotoDown()
{
    int indexOfLineStart;
    int line;
    QTextString::Char *c = string->lineStartOfChar( idx, &indexOfLineStart, &line );
    if ( !c )
	return;

    tmpIndex = QMAX( tmpIndex, idx - indexOfLineStart );
    if ( line == string->lines() - 1 ) {
	if ( !string->next() ) {
	    if ( !nested )
		return;
	    pop();
	    processNesting( Down );
	    if ( idx == -1 ) {
		pop();
		if ( !string->next() )
		    return;
		idx = tmpIndex = 0;
	    } else {
		tmpIndex = -1;
		return;
	    }
	}
	string = string->next();
	if ( !string->lineStartOfLine( 0, &indexOfLineStart ) )
	    return;
	int end;
	if ( string->lines() == 1 )
	    end = string->length();
	else
	    string->lineStartOfLine( 1, &end );
	if ( indexOfLineStart + tmpIndex < end )
	    idx = indexOfLineStart + tmpIndex;
	else
	    idx = end - 1;
    } else {
	++line;
	int end;
	if ( line == string->lines() - 1 )
	    end = string->length();
	else
	    string->lineStartOfLine( line + 1, &end );
	if ( !string->lineStartOfLine( line, &indexOfLineStart ) )
	    return;
	if ( indexOfLineStart + tmpIndex < end )
	    idx = indexOfLineStart + tmpIndex;
	else
	    idx = end - 1;
    }
}

void QTextCursor::gotoLineEnd()
{
    int indexOfLineStart;
    int line;
    QTextString::Char *c = string->lineStartOfChar( idx, &indexOfLineStart, &line );
    if ( !c )
	return;

    if ( line == string->lines() - 1 ) {
	idx = string->length() - 1;
    } else {
	c = string->lineStartOfLine( ++line, &indexOfLineStart );
	indexOfLineStart--;
	idx = indexOfLineStart;
    }
}

void QTextCursor::gotoLineStart()
{
    int indexOfLineStart;
    int line;
    QTextString::Char *c = string->lineStartOfChar( idx, &indexOfLineStart, &line );
    if ( !c )
	return;

    idx = indexOfLineStart;
}

void QTextCursor::gotoHome()
{
    tmpIndex = -1;
    string = doc->firstParag();
    idx = 0;
}

void QTextCursor::gotoEnd()
{
    if ( !doc->lastParag()->isValid() )
	return;

    tmpIndex = -1;
    string = doc->lastParag();
    idx = string->length() - 1;
}

void QTextCursor::gotoPageUp( QTextView *view )
{
    tmpIndex = -1;
    QTextParag *s = string;
    int h = view->visibleHeight();
    int y = s->rect().y();
    while ( s ) {
	if ( y - s->rect().y() >= h )
	    break;
	s = s->prev();
    }

    if ( !s )
	s = doc->firstParag();

    string = s;
    idx = 0;
}

void QTextCursor::gotoPageDown( QTextView *view )
{
    tmpIndex = -1;
    QTextParag *s = string;
    int h = view->visibleHeight();
    int y = s->rect().y();
    while ( s ) {
	if ( s->rect().y() - y >= h )
	    break;
	s = s->next();
    }

    if ( !s )
	s = doc->lastParag();

    if ( !s->isValid() )
	return;

    string = s;
    idx = 0;
}

void QTextCursor::gotoWordLeft()
{
    gotoLeft();
    tmpIndex = -1;
    QTextString *s = string->string();
    bool allowSame = FALSE;
    for ( int i = idx - 1; i >= 0; --i ) {
	if ( s->at( i ).c.isSpace() || s->at( i ).c == '\t' ) {
	    if ( !allowSame && s->at( i ).c == s->at( idx ).c )
		continue;
	    idx = i + 1;
	    return;
	}
	if ( !allowSame && s->at( i ).c != s->at( idx ).c )
	    allowSame = TRUE;
    }

    if ( string->prev() ) {
	string = string->prev();
	idx = string->length() - 1;
    } else {
	gotoLineStart();
    }
}

void QTextCursor::gotoWordRight()
{
    tmpIndex = -1;
    QTextString *s = string->string();
    bool allowSame = FALSE;
    for ( int i = idx + 1; i < (int)s->length(); ++i ) {
	if ( s->at( i ).c.isSpace() || s->at( i ).c == '\t' ) {
	    if ( !allowSame &&  s->at( i ).c == s->at( idx ).c )
		continue;
	    idx = i;
	    return;
	}
	if ( !allowSame && s->at( i ).c != s->at( idx ).c )
	    allowSame = TRUE;
    }

    if ( string->next() ) {
	string = string->next();
	idx = 0;
    } else {
	gotoLineEnd();
    }
}

bool QTextCursor::atParagStart()
{
    return idx == 0;
}

bool QTextCursor::atParagEnd()
{
    return idx == string->length() - 1;
}

void QTextCursor::splitAndInsertEmtyParag( bool ind, bool updateIds )
{
    tmpIndex = -1;
    QTextFormat *f = 0;
    if ( doc->useFormatCollection() ) {
	f = string->at( idx )->format();
	if ( idx == string->length() - 1 && idx > 0 )
	    f = string->at( idx - 1 )->format();
	if ( f->isMisspelled() ) {
	    f->removeRef();
	    f = doc->formatCollection()->format( f->font(), f->color() );
	}
    }



    if ( atParagStart() ) {
	QTextParag *p = string->prev();
	QTextParag *s = new QTextParag( doc, p, string, updateIds );
	if ( f )
	    s->setFormat( 0, 1, f, TRUE );
	s->setStyleSheetItems( string->styleSheetItems() );
	s->setListStyle( string->listStyle() );
	s->setAlignment( string->alignment() );
	if ( ind ) {
	    s->indent();
	    s->format();
	    indent();
	    string->format();
	}
    } else if ( atParagEnd() ) {
	QTextParag *n = string->next();
	QTextParag *s = new QTextParag( doc, string, n, updateIds );
	if ( f )
	    s->setFormat( 0, 1, f, TRUE );
	s->setStyleSheetItems( string->styleSheetItems() );
	s->setListStyle( string->listStyle() );
	s->setAlignment( string->alignment() );
	if ( ind ) {
	    int oi, ni;
	    s->indent( &oi, &ni );
	    string = s;
	    idx = ni;
	} else {
	    string = s;
	    idx = 0;
	}
    } else {
	QString str = string->string()->toString().mid( idx, 0xFFFFFF );
	QTextParag *n = string->next();
	QTextParag *s = new QTextParag( doc, string, n, updateIds );
	s->setStyleSheetItems( string->styleSheetItems() );
	s->setListStyle( string->listStyle() );
	s->setAlignment( string->alignment() );
	s->remove( 0, 1 );
	s->append( str, TRUE );
	for ( uint i = 0; i < str.length(); ++i ) {
	    s->setFormat( i, 1, string->at( idx + i )->format(), TRUE );
	    if ( string->at( idx + i )->isCustom )
		s->at( i )->setCustomItem( string->at( idx + i )->customItem() );
	}	
	string->truncate( idx );
	if ( ind ) {
	    int oi, ni;
	    s->indent( &oi, &ni );
	    string = s;
	    idx = ni;
	} else {
	    string = s;
	    idx = 0;
	}
    }

    invalidateNested();
}

bool QTextCursor::remove()
{
    tmpIndex = -1;
    if ( !atParagEnd() ) {
	string->remove( idx, 1 );
	int h = string->rect().height();
	string->format( -1, TRUE );
	if ( h != string->rect().height() )
	    invalidateNested();
	else if ( doc->parent() )
	    doc->nextDoubleBuffered = TRUE;
	return FALSE;
    } else if ( string->next() ) {
	string->join( string->next() );
	invalidateNested();
	return TRUE;
    }
    return FALSE;
}

void QTextCursor::indent()
{
    int oi = 0, ni = 0;
    string->indent( &oi, &ni );
    if ( oi == ni )
	return;

    if ( idx >= oi )
	idx += ni - oi;
    else
	idx = ni;
}

bool QTextCursor::checkOpenParen()
{
    if ( !doc->isParenCheckingEnabled() )
	return FALSE;

    QTextParag::ParenList parenList = string->parenList();

    QTextParag::Paren openParen, closedParen;
    QTextParag *closedParenParag = string;

    int i = 0;
    int ignore = 0;
    bool foundOpen = FALSE;
    QChar c = string->at( idx )->c;
    while ( TRUE ) {
	if ( !foundOpen ) {
	    if ( i >= (int)parenList.count() )
		goto aussi;
	    openParen = parenList[ i ];
	    if ( openParen.pos != idx ) {
		++i;
		continue;
	    } else {
		foundOpen = TRUE;
		++i;
	    }
	}
	
	if ( i >= (int)parenList.count() ) {
	    while ( TRUE ) {
		closedParenParag = closedParenParag->next();
		if ( !closedParenParag )
		    goto aussi;
		if ( closedParenParag->parenList().count() > 0 ) {
		    parenList = closedParenParag->parenList();
		    break;
		}
	    }
	    i = 0;
	}
	
	closedParen = parenList[ i ];
	if ( closedParen.type == QTextParag::Paren::Open ) {
	    ignore++;
	    ++i;
	    continue;
	} else {
	    if ( ignore > 0 ) {
		ignore--;
		++i;
		continue;
	    }
	
	    int id = QTextDocument::ParenMatch;
	    if ( c == '{' && closedParen.chr != '}' ||
		 c == '(' && closedParen.chr != ')' ||
		 c == '[' && closedParen.chr != ']' )
		id = QTextDocument::ParenMismatch;
	    doc->setSelectionStart( id, this );
	    int tidx = idx;
	    QTextParag *tstring = string;
	    idx = closedParen.pos + 1;
	    string = closedParenParag;
	    doc->setSelectionEnd( id, this );
	    string = tstring;
	    idx = tidx;
	    return TRUE;
	}
	
	++i;
    }

 aussi:
    return FALSE;
}

bool QTextCursor::checkClosedParen()
{
    if ( !doc->isParenCheckingEnabled() )
	return FALSE;

    QTextParag::ParenList parenList = string->parenList();

    QTextParag::Paren openParen, closedParen;
    QTextParag *openParenParag = string;

    int i = parenList.count() - 1;
    int ignore = 0;
    bool foundClosed = FALSE;
    QChar c = string->at( idx - 1 )->c;
    while ( TRUE ) {
	if ( !foundClosed ) {
	    if ( i < 0 )
		goto aussi;
	    closedParen = parenList[ i ];
	    if ( closedParen.pos != idx - 1 ) {
		--i;
		continue;
	    } else {
		foundClosed = TRUE;
		--i;
	    }
	}
	
	if ( i < 0 ) {
	    while ( TRUE ) {
		openParenParag = openParenParag->prev();
		if ( !openParenParag )
		    goto aussi;
		if ( openParenParag->parenList().count() > 0 ) {
		    parenList = openParenParag->parenList();
		    break;
		}
	    }
	    i = parenList.count() - 1;
	}
	
	openParen = parenList[ i ];
	if ( openParen.type == QTextParag::Paren::Closed ) {
	    ignore++;
	    --i;
	    continue;
	} else {
	    if ( ignore > 0 ) {
		ignore--;
		--i;
		continue;
	    }
	
	    int id = QTextDocument::ParenMatch;
	    if ( c == '}' && openParen.chr != '{' ||
		 c == ')' && openParen.chr != '(' ||
		 c == ']' && openParen.chr != '[' )
		id = QTextDocument::ParenMismatch;
	    doc->setSelectionStart( id, this );
	    int tidx = idx;
	    QTextParag *tstring = string;
	    idx = openParen.pos;
	    string = openParenParag;
	    doc->setSelectionEnd( id, this );
	    string = tstring;
	    idx = tidx;
	    return TRUE;
	}
	
	--i;
    }

 aussi:
    return FALSE;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

const int QTextDocument::numSelections = 4; // Don't count the Temp one!

QTextDocument::QTextDocument( QTextDocument *p )
    : par( p ), tc( 0 )
{
#if defined(PARSER_DEBUG)
    qDebug( debug_indent + "new QTextDocument (%p)", this );
#endif
    syntaxHighlighte = 0;
    useFC = TRUE;
    pFormatter = 0;
    indenter = 0;
    parenCheck = FALSE;
    completion = FALSE;
    fCollection = new QTextFormatCollection;
    fParag = 0;
    txtFormat = Qt::AutoText;
    preferRichText = FALSE;
    filename = QString::null;
    pages = FALSE;
    focusIndicator.parag = 0;
    minw = 0;
    minwParag = 0;
    align = AlignAuto;

    sheet_ = QStyleSheet::defaultSheet();
    factory_ = QMimeSourceFactory::defaultFactory();
    contxt = QString::null;

    linkC = Qt::blue;
    underlLinks = TRUE;
    backBrush = 0;
    buf_pixmap = 0;
    nextDoubleBuffered = FALSE;

    if ( p )
	withoutDoubleBuffer = p->withoutDoubleBuffer;
    else
	withoutDoubleBuffer = FALSE;

    lParag = fParag = new QTextParag( this, 0, 0 );
    tmpCursor = 0;

    cx = 0;
    cy = 2;
    if ( p )
	cx = cy = 0;
    cw = 600;
    flow_ = new QTextFlow;
    flow_->setWidth( cw );

    selectionColors[ Standard ] = QApplication::palette().color( QPalette::Active, QColorGroup::Highlight );
    selectionColors[ ParenMismatch ] = Qt::magenta;
    selectionColors[ ParenMatch ] = Qt::green;
    selectionColors[ Search ] = Qt::yellow;
    selectionText[ Standard ] = TRUE;
    selectionText[ ParenMismatch ] = FALSE;
    selectionText[ ParenMatch ] = FALSE;
    selectionText[ Search ] = FALSE;
    commandHistory = new QTextCommandHistory( 100 ); // ### max undo/redo steps should be configurable
}

QTextDocument::~QTextDocument()
{
    clear();
    delete commandHistory;
    delete flow_;
    if ( !par )
	delete pFormatter;
    delete fCollection;
    delete syntaxHighlighte;
    delete buf_pixmap;
    delete indenter;
}

void QTextDocument::clear()
{
    while ( fParag ) {
	QTextParag *p = fParag->next();
	delete fParag;
	fParag = p;
    }
    fParag = 0;
}

int QTextDocument::widthUsed() const
{
    QTextParag *p = fParag;
    int w = 0;
    while ( p ) {
	int a = p->alignment();
	p->setAlignment( Qt::AlignLeft );
	p->invalidate( 0 );
	p->format();
	w = QMAX( w, p->rect().width() );
	p->setAlignment( a );
	p->invalidate( 0 );
	p = p->next();
    }
    return w;
}

bool QTextDocument::setMinimumWidth( int w, QTextParag *p )
{
    if ( p == minwParag ) {
	minw = w;
    } else if ( w > minw ) {
	minw = w;
	minwParag = p;
    }
    emit minimumWidthChanged( minw );
    cw = QMAX( minw, cw );
    return TRUE;
}

void QTextDocument::setPlainText( const QString &text, bool tabify )
{
    clear();
    preferRichText = FALSE;

    QString s;
    lParag = 0;
    QStringList lst = QStringList::split( '\n', text, TRUE );
    for ( QStringList::Iterator it = lst.begin(); it != lst.end(); ++it ) {
	lParag = new QTextParag( this, lParag, 0 );
	if ( !fParag )
	    fParag = lParag;
	s = *it;
	if ( !s.isEmpty() ) {
	    QChar c;
	    int i = 0;
	    int spaces = 0;
	    if ( tabify ) {
		for ( ; i < (int)s.length(); ++i ) {
		    c = s[ i ];
		    if ( c != ' ' && c != '\t' )
			break;
		    if ( c == '\t' ) {
			spaces = 0;
			s.replace( i, 1, "\t\t" );
			++i;
		    } else if ( c == ' ' )
			++spaces;
		    if ( spaces == 4 ) {
			s.replace( i  - 3, 4, "\t" );
			i-= 2;
			spaces = 0;
		    }
		}
	    }
	    if ( s.right( 1 ) != " " )
		s += " ";
	    lParag->append( s );
	}
    }

    if ( !lParag )
	lParag = fParag = new QTextParag( this, 0, 0 );
}

struct Tag {
    Tag(){}
    Tag( const QString&n,  const QStyleSheetItem* s ):name(n),style(s) {
	wsm = QStyleSheetItem::WhiteSpaceNormal;
    }
    QString name;
    const QStyleSheetItem* style;
    QStyleSheetItem::WhiteSpaceMode wsm;
    QTextFormat format;
};

#define NEWPAR       do{ if ( !hasNewPar ) curpar = new QTextParag( this, curpar ); \
		    hasNewPar = TRUE;  \
		    QVector<QStyleSheetItem> vec( tags.count() ); \
		    int i = 0; \
		    for ( QValueStack<Tag>::Iterator it = tags.begin(); it != tags.end(); ++it ) \
			vec.insert( i++, (*it).style ); 	\
		    curpar->setStyleSheetItems( vec ); }while(FALSE)
#define NEWPAROPEN(nstyle)       do{ if ( !hasNewPar ) curpar = new QTextParag( this, curpar ); \
		    hasNewPar = TRUE;  \
		    QVector<QStyleSheetItem> vec( tags.count()+1 ); \
		    int i = 0; \
		    for ( QValueStack<Tag>::Iterator it = tags.begin(); it != tags.end(); ++it ) \
			vec.insert( i++, (*it).style ); 	\
		    vec.insert( i, nstyle ); \
		    curpar->setStyleSheetItems( vec ); }while(FALSE)


void QTextDocument::setRichText( const QString &text, const QString &context )
{
    if ( !context.isEmpty() )
	setContext( context );
    clear();
    fParag = new QTextParag( this );
    QTextParag* curpar = fParag;

    int pos = 0;
    QValueStack<Tag> tags;
    Tag curtag( "", sheet_->item("") );
    curtag.format = *formatCollection()->defaultFormat();
    bool space = FALSE;

    QString doc = text;
    int depth = 0;
    bool hasNewPar = TRUE;
    while ( pos < int( doc.length() ) ) {
	if (hasPrefix(doc, pos, '<' ) ){
	    if (!hasPrefix(doc, pos+1, QChar('/'))) {
		// open tag
		QMap<QString, QString> attr;
		bool emptyTag = FALSE;
		QString tagname = parseOpenTag(doc, pos, attr, emptyTag);
		if ( tagname.isEmpty() )
		    continue; // nothing we could do with this, probably parse error
		while ( eat( doc, pos, '\n' ) )
		    ; // eliminate newlines right after openings
		
		const QStyleSheetItem* nstyle = sheet_->item(tagname);
		if ( nstyle ) {
		    // we might have to close some 'forgotten' tags
		    while ( !nstyle->allowedInContext( curtag.style ) ) {
			QString msg;
			msg.sprintf( "QText Warning: Document not valid ( '%s' not allowed in '%s' #%d)",
				     tagname.ascii(), curtag.style->name().ascii(), pos);
			sheet_->error( msg );
			if ( tags.isEmpty() )
			    break;
			curtag = tags.pop();
			depth--;
		    }
		}
		
		QTextCustomItem* custom =  0;
		// some well-known empty tags
		if ( tagname == "br" ) {
		    emptyTag = TRUE;
		    NEWPAR;
		}  else if ( tagname == "hr" ) {
		    emptyTag = TRUE;
		    NEWPAR;
		} else if ( tagname == "table" ) {
		    QTextFormat format = curtag.format.makeTextFormat(  nstyle, attr );
		    custom = parseTable( attr, format, doc, pos );
		    (void ) eatSpace( doc, pos );
		    emptyTag = TRUE;
		} else {
		    custom = sheet_->tag( tagname, attr, contxt, *factory_ , emptyTag, this );
		}
		
		
		if ( custom ) {
		    QTextFormat format = curtag.format.makeTextFormat(  nstyle, attr );
		    int index = curpar->length() - 1;
		    if ( index < 0 )
			index = 0;
		    curpar->append( QChar('b') );
		    curpar->setFormat( index, 1, &format );
		    curpar->at( index )->setCustomItem( custom );
		    curpar->addCustomItem();
		    registerCustomItem( custom, curpar );
		} else if ( !emptyTag ) {
		    tags += curtag;
		    if ( nstyle ) {
			// ignore whitespace for inline elements if there was already one
			if ( nstyle->whiteSpaceMode() == QStyleSheetItem::WhiteSpaceNormal
			     && ( space || nstyle->displayMode() != QStyleSheetItem::DisplayInline ) )
			    eatSpace( doc, pos );
			
			// some styles are not self nesting
			if ( nstyle == curtag.style && !nstyle->selfNesting() )
			    (void) tags.pop();
			
			if ( curtag.style->displayMode() == QStyleSheetItem::DisplayListItem )
			    hasNewPar = FALSE; // we want empty paragraphs in this case
			if ( nstyle->displayMode() != QStyleSheetItem::DisplayInline )
			    NEWPAROPEN(nstyle);
			if ( nstyle->displayMode() == QStyleSheetItem::DisplayListItem )
			    curpar->setListStyle( curtag.style->listStyle() );
			curtag.style = nstyle;
			curtag.wsm = nstyle->whiteSpaceMode();
			curtag.format = curtag.format.makeTextFormat( nstyle, attr );
			if ( nstyle->displayMode() != QStyleSheetItem::DisplayInline )
			    curpar->setFormat( &curtag.format );
		    }
		    curtag.name = tagname;
		    if ( curtag.name == "a" && attr.find( "name" ) != attr.end() && doc[ pos] == '<' ) 	// hack to be sure
			doc.insert( pos, " " );						// <a name=".."></a> formats or inserted
		
		    depth++;
		}
	    } else {	
		// close tag
		QString tagname = parseCloseTag( doc, pos );
		if ( tagname.isEmpty() )
		    continue; // nothing we could do with this, probably parse error
		while ( eat( doc, pos, '\n' ) )
		    ; // eliminate newlines right after closings
		depth--;
		while ( curtag.name != tagname ) {
		    QString msg;
		    msg.sprintf( "QText Warning: Document not valid ( '%s' not closed before '%s' #%d)",
				 curtag.name.ascii(), tagname.ascii(), pos);
		    sheet_->error( msg );
		    if ( !hasNewPar && curtag.style->displayMode() != QStyleSheetItem::DisplayInline
			 && curtag.wsm == QStyleSheetItem::WhiteSpaceNormal ) {
			eatSpace( doc, pos );
			NEWPAR;
		    }
		    if ( tags.isEmpty() )
			break;
		    curtag = tags.pop();
		    depth--;
		}
		
		if ( !hasNewPar && curtag.style->displayMode() != QStyleSheetItem::DisplayInline
		     && curtag.wsm == QStyleSheetItem::WhiteSpaceNormal ) {
		    eatSpace( doc, pos );
		    NEWPAR;
		}
		if ( !tags.isEmpty() )
		    curtag = tags.pop();
	    }
	} else {
	    // normal contents
	    QString s;
	    QChar c;
	    while ( pos < int( doc.length() ) && !hasPrefix(doc, pos, '<' ) ){
		c = parseChar( doc, pos, curtag.wsm );
		space = c.isSpace();
		if ( c == '\n' ) // happens in WhiteSpacePre mode
		    break;
		s += c;
	    }
	    if ( !s.isEmpty() && curtag.style->displayMode() != QStyleSheetItem::DisplayNone ) {
		hasNewPar = FALSE;
		int index = curpar->length() - 1;
		if ( index < 0 )
		    index = 0;
		curpar->append( s );
		curpar->setFormat( index, s.length(), &curtag.format );
	    }
	    if ( c == '\n' )
		NEWPAR;

	}
    }
}

void QTextDocument::load( const QString &fn, bool tabify )
{
    filename = fn;
    QFile file( fn );
    file.open( IO_ReadOnly );
    QTextStream ts( &file );
    QString txt = ts.read();
    file.close();
    setText( txt, fn, tabify );
}

void QTextDocument::setText( const QString &text, const QString &context, bool tabify )
{
    focusIndicator.parag = 0;
    removeSelection( Standard );
    if ( txtFormat == Qt::AutoText && QStyleSheet::mightBeRichText( text ) ||
	 txtFormat == Qt::RichText )
	setRichText( text, context );
    else
	setPlainText( text, tabify );
}

static void do_untabify( QString &s )
{
    int numTabs = 0;
    int i = 0;
    while ( s[ i++ ] == '\t' )
	numTabs++;
    if ( !numTabs )
	return;

    int realTabs = ( numTabs / 2 ) * 2;
    if ( realTabs != numTabs )
	s = s.replace( numTabs - 1, 1, "    " );
    QString tabs;
    tabs.fill( '\t', realTabs / 2 );
    s = s.replace( 0, realTabs, tabs );
}

QString QTextDocument::plainText( QTextParag *p, bool formatted, bool untabify ) const
{
    if ( !p ) {
	QString buffer;
	QString s;
	QTextParag *p = fParag;
	while ( p ) {
	    s = p->string()->toString();
	    if ( untabify )
		do_untabify( s );
	    s += "\n";
	    buffer += s;
	    p = p->next();
	}
	return buffer;
    } else {
	if ( !formatted )
	    return p->string()->toString();

	// ##### TODO: return formatted string
	return p->string()->toString();
    }
}

QString QTextDocument::richText( QTextParag *p, bool formatted ) const
{
    if ( !p ) {
	// #### very poor implementation!
	QString text;
	p = fParag;
	QTextParag *lastParag = 0;
	QTextFormat *lastFormat = 0;
	QTextString::Char *c = 0;
	int listDepth = 0;
	bool inBulletList = FALSE;
	bool inOrderedList = FALSE;
	while ( p ) {
	    QString s;
	    if ( inBulletList && ( !p->style()  || p->style()->displayMode() != QStyleSheetItem::DisplayListItem ) ) {
		text += "</ul>\n";
		inBulletList = FALSE;
		listDepth--;
	    }
	    if ( !inBulletList && p->style() && p->style()->displayMode() == QStyleSheetItem::DisplayListItem &&
		 p->listStyle() == QStyleSheetItem::ListDisc ) {
		text += "<ul>\n";
		listDepth++;
		inBulletList = TRUE;
	    }
	    if ( inOrderedList && ( !p->style() || p->style()->displayMode() != QStyleSheetItem::DisplayListItem ) ) {
		text += "</ol>\n";
		inOrderedList = FALSE;
		listDepth--;
	    }
	    if ( !inOrderedList && p->style() && p->style()->displayMode() == QStyleSheetItem::DisplayListItem &&
		 p->listStyle() == QStyleSheetItem::ListDecimal ) {
		text += "<ol>\n";
		listDepth++;
		inOrderedList = TRUE;
	    }
	    if ( inOrderedList || inBulletList ) {
		s = "<li>";
	    } else if ( !lastParag || lastParag->alignment() != p->alignment() ) {
		s = "<p align=\"";
		if ( p->alignment() & Qt::AlignRight )
		    s += "right";
		else if ( p->alignment() & Qt::AlignCenter )
		    s += "center";
		else
		    s += "left";
		s += "\">";
	    } else {
		s = "<p>";
	    }
	
	    int len = 0;
	    for ( int i = 0; i < p->length(); ++i ) {
		c = &p->string()->at( i );
		if ( !lastFormat || ( lastFormat->key() != c->format()->key() && c->c != ' ' ) ) {
		    s += c->format()->makeFormatChangeTags( lastFormat );
		    lastFormat = c->format();
		}
		if ( c->c == '<' )
		    s += "&lt;";
		else if ( c->c == '>' )
		    s += "&gt;";
		else
		    s += c->c;
		len += c->c != ' ' ? 1 : 0;
	    }
	    if ( !inBulletList && !inOrderedList  )
		text += s + lastFormat->makeFormatEndTags() + "</p>\n";
	    else if ( len > 0 )
		text += s + lastFormat->makeFormatEndTags() + "\n";
	    lastFormat = 0;
	    lastParag = p;
	    p = p->next();
	}
	text += "\n";
	return text;
    } else {
	// #### TODO return really rich text
	return plainText( p, formatted );
    }
}

QString QTextDocument::text( bool untabify ) const
{
    if ( plainText().simplifyWhiteSpace().isEmpty() )
	return QString::null;
    if ( txtFormat == Qt::AutoText && preferRichText || txtFormat == Qt::RichText )
	return richText();
    return plainText( 0, FALSE, untabify );
}

QString QTextDocument::text( int parag, bool formatted ) const
{
    QTextParag *p = paragAt( parag );
    if ( !p )
	return QString::null;

    if ( txtFormat == Qt::AutoText && preferRichText || txtFormat == Qt::RichText )
	return richText( p, formatted );
    else
	return plainText( p, formatted );
}

void QTextDocument::invalidate()
{
    QTextParag *s = fParag;
    while ( s ) {
	s->invalidate( 0 );
	s = s->next();
    }
}

void QTextDocument::save( const QString &fn, bool untabify )
{
    if ( !fn.isEmpty() )
	filename = fn;
    if ( !filename.isEmpty() ) {
	QFile file( filename );
	if ( file.open( IO_WriteOnly ) ) {
	    QTextStream ts( &file );
	    ts << text( untabify );;
	    file.close();
	} else {
	    qWarning( "couldn't open file %s", filename.latin1() );
	}
    } else {
	qWarning( "QTextDocument::save(): couldn't save - no filename specified!" );
    }
}

QString QTextDocument::fileName() const
{
    return filename;
}

void QTextDocument::selectionStart( int id, int &paragId, int &index )
{
    QMap<int, Selection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return;
    Selection &sel = *it;
    paragId = QMIN( sel.startParag->paragId(), sel.endParag->paragId() );
    index = sel.startIndex;
}

void QTextDocument::selectionEnd( int id, int &paragId, int &index )
{
    QMap<int, Selection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return;
    Selection &sel = *it;
    paragId = QMAX( sel.startParag->paragId(), sel.endParag->paragId() );
    if ( paragId == sel.startParag->paragId() )
	index = sel.startParag->selectionEnd( id );
    else
	index = sel.endParag->selectionEnd( id );
}

QTextParag *QTextDocument::selectionStart( int id )
{
    QMap<int, Selection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return 0;
    Selection &sel = *it;
    if ( sel.startParag->paragId() <  sel.endParag->paragId() )
	return sel.startParag;
    return sel.endParag;
}

QTextParag *QTextDocument::selectionEnd( int id )
{
    QMap<int, Selection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return 0;
    Selection &sel = *it;
    if ( sel.startParag->paragId() >  sel.endParag->paragId() )
	return sel.startParag;
    return sel.endParag;
}

bool QTextDocument::setSelectionEnd( int id, QTextCursor *cursor )
{
    QMap<int, Selection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return FALSE;

    Selection &sel = *it;
    QTextParag *oldEndParag = sel.endParag;
    QTextParag *oldStartParag = sel.startParag;
    if ( sel.endParag->paragId() < sel.startParag->paragId() ) {
	oldStartParag = sel.endParag;
	oldEndParag = sel.startParag;
    }
    sel.endParag = cursor->parag();
    int start = sel.startIndex;
    int end = cursor->index();
    bool swapped = FALSE;
    if ( sel.endParag->paragId() < sel.startParag->paragId() ) {
	sel.endParag = sel.startParag;
	sel.startParag = cursor->parag();
	end = sel.startIndex;
	start = cursor->index();
	swapped = TRUE;
    }

    if ( sel.startParag == sel.endParag ) {
	if ( end < start) {
	    end = sel.startIndex;
	    start = cursor->index();
	}
	sel.endParag->setSelection( id, start, end );

	QTextParag *p = 0;
	if ( sel.endParag->paragId() < oldEndParag->paragId() ) {
	    p = sel.endParag;
	    p = p->next();
	    while ( p ) {
		p->removeSelection( id );
		if ( p == oldEndParag )
		    break;
		p = p->next();
	    }
	}
	
	if ( sel.startParag->paragId() > oldStartParag->paragId() ) {
	    p = sel.startParag;
	    p = p->prev();
	    while ( p ) {
		p->removeSelection( id );
		if ( p == oldStartParag )
		    break;
		p = p->prev();
	    }
	}
    } else {
	QTextParag *p = sel.startParag;
	p->setSelection( id, start, p->length() - 1 );
	p->setChanged( TRUE );
	p = p->next();
	if ( p )
	    p->setChanged( TRUE );
	while ( p && p != sel.endParag ) {
	    p->setSelection( id, 0, p->length() - 1 );
	    p->setChanged( TRUE );
	    p = p->next();
	}
	sel.endParag->setSelection( id, 0, end );
	sel.endParag->setChanged( TRUE );

	if ( sel.endParag->paragId() < oldEndParag->paragId() ) {
	    p = sel.endParag;
	    p = p->next();
	    while ( p ) {
		p->removeSelection( id );
		if ( p == oldEndParag )
		    break;
		p = p->next();
	    }
	}
	
	if ( sel.startParag->paragId() > oldStartParag->paragId() ) {
	    p = sel.startParag;
	    p = p->prev();
	    while ( p ) {
		p->removeSelection( id );
		if ( p == oldStartParag )
		    break;
		p = p->prev();
	    }
	}
	
	if ( swapped ) {
	    p = sel.startParag;
	    sel.startParag = sel.endParag;
	    sel.endParag = p;
	}
    }

    return TRUE;
}

bool QTextDocument::removeSelection( int id )
{
    QMap<int, Selection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return FALSE;

    QTextParag *start = ( *it ).startParag;
    QTextParag *end = ( *it ).endParag;
    if ( end->paragId() < start->paragId() ) {
	end = ( *it ).startParag;
	start = ( *it ).endParag;
    }

    QTextParag *p = start;
    while ( p ) {
	p->removeSelection( id );
	if ( p == end )
	    break;
	p = p->next();
    }

    selections.remove( id );
    return TRUE;
}

QString QTextDocument::selectedText( int id ) const
{
    // ######## TODO: look at textFormat() and return rich text or plain text (like the text() method!)
    QMap<int, Selection>::ConstIterator it = selections.find( id );
    if ( it == selections.end() )
	return QString::null;

    Selection sel = *it;

    QTextParag *endParag = sel.endParag;
    QTextParag *startParag = sel.startParag;
    if ( sel.endParag->paragId() < sel.startParag->paragId() ) {
	startParag = sel.endParag;
	endParag = sel.startParag;
    }

    QString buffer;
    QString s;
    QTextParag *p = startParag;
    while ( p ) {
	s = p->string()->toString().mid( p->selectionStart( id ),
					 p->selectionEnd( id ) - p->selectionStart( id ) );
	if ( p->selectionEnd( id ) == p->length() - 1 && p != endParag )
	    s += "\n";
	buffer += s;
	if ( p == endParag )
	    break;
	p = p->next();
    }

    return buffer;
}

void QTextDocument::setFormat( int id, QTextFormat *f, int flags )
{
    QMap<int, Selection>::ConstIterator it = selections.find( id );
    if ( it == selections.end() )
	return;

    Selection sel = *it;

    QTextParag *endParag = sel.endParag;
    QTextParag *startParag = sel.startParag;
    if ( sel.endParag->paragId() < sel.startParag->paragId() ) {
	startParag = sel.endParag;
	endParag = sel.startParag;
    }

    QTextParag *p = startParag;
    while ( p ) {
	int end = p->selectionEnd( id );
	if ( end == p->length() - 1 )
	    end++;
	p->setFormat( p->selectionStart( id ), end - p->selectionStart( id ),
		      f, TRUE, flags );
	if ( p == endParag )
	    break;
	p = p->next();
    }
}

void QTextDocument::copySelectedText( int id )
{
#ifndef QT_NO_CLIPBOARD
    if ( !hasSelection( id ) )
	return;

    QApplication::clipboard()->setText( selectedText( id ) );
#endif
}

void QTextDocument::removeSelectedText( int id, QTextCursor *cursor )
{
    QMap<int, Selection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return;

    Selection sel = *it;
    QTextParag *startParag = sel.startParag;
    QTextParag *endParag = sel.endParag;
    if ( sel.endParag->paragId() < sel.startParag->paragId() ) {
	endParag = sel.startParag;
	startParag = sel.endParag;
    }

    if ( startParag == endParag ) {
	int idx = -1;
	if ( cursor->parag() == startParag &&
	     cursor->index() > startParag->selectionStart( id ) )
	    idx = startParag->selectionStart( id );
	startParag->remove( startParag->selectionStart( id ),
			    startParag->selectionEnd( id ) - startParag->selectionStart( id ) );
	if ( idx != -1 )
	    cursor->setIndex( idx );
    } else {
	int idx = -1;
	QTextParag *cp = 0;
	
	if ( cursor->parag() == startParag &&
	     cursor->index() > startParag->selectionStart( id ) )
	    idx = startParag->selectionStart( id );
	else if ( cursor->parag()->paragId() > startParag->paragId() &&
		  cursor->parag()->paragId() <= endParag->paragId() ) {
	    cp = startParag;
	    idx = startParag->selectionStart( id );
	}
	
	startParag->remove( startParag->selectionStart( id ),
			    startParag->selectionEnd( id ) - startParag->selectionStart( id ) );
	endParag->remove( 0, endParag->selectionEnd( id ) );
	QTextParag *p = startParag, *tmp;
	p = p->next();
	int dy = 0;
	while ( p ) {
	    if ( p == endParag )
		break;
	    tmp = p->next();
	    dy += p->rect().height();
	    delete p;
	    p = tmp;
	}
	
	while ( p ) {
	    p->move( -dy );
	    p->invalidate( 0 );
	    p->setEndState( -1 );
	    p = p->next();
	}
	
	startParag->join( endParag );
	
	if ( cp )
	    cursor->setParag( cp );
	if ( idx != -1 )
	    cursor->setIndex( idx );
    }

    removeSelection( id );
}

void QTextDocument::indentSelection( int id )
{
    QMap<int, Selection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return;

    Selection sel = *it;
    QTextParag *startParag = sel.startParag;
    QTextParag *endParag = sel.endParag;
    if ( sel.endParag->paragId() < sel.startParag->paragId() ) {
	endParag = sel.startParag;
	startParag = sel.endParag;
    }

    QTextParag *p = startParag;
    while ( p && p != endParag ) {
	p->indent();
	p = p->next();
    }
}

void QTextDocument::addCompletionEntry( const QString &s )
{
    if ( !eCompletionMap )
	eCompletionMap = new QMap<QChar, QStringList >();
    QChar key( s[ 0 ] );
    QMap<QChar, QStringList>::Iterator it = eCompletionMap->find( key );
    if ( it == eCompletionMap->end() )
	eCompletionMap->insert( key, QStringList( s ) );
    else
	( *it ).append( s );
}

QStringList QTextDocument::completionList( const QString &s ) const
{
    ( (QTextDocument*)this )->updateCompletionMap();

    QChar key( s[ 0 ] );
    QMap<QChar, QStringList>::ConstIterator it = eCompletionMap->find( key );
    if ( it == eCompletionMap->end() )
	return QStringList();
    QStringList::ConstIterator it2 = ( *it ).begin();
    QStringList lst;
    int len = s.length();
    for ( ; it2 != ( *it ).end(); ++it2 ) {
	if ( (int)( *it2 ).length() > len && ( *it2 ).left( len ) == s &&
	     lst.find( *it2 ) == lst.end() )
	    lst << *it2;
    }

    return lst;
}

void QTextDocument::updateCompletionMap()
{
    QTextParag *s = fParag;
    while ( s ) {
	if ( s->length() == s->lastLengthForCompletion() ) {
	    s = s->next();
	    continue;
	}
	
	QChar c;
	QString buffer;
	for ( int i = 0; i < s->length(); ++i ) {
	    c = s->at( i )->c;
	    if ( c.isLetter() || c.isNumber() || c == '_' || c == '#' ) {
		buffer += c;
	    } else {
		addCompletionEntry( buffer );
		buffer = QString::null;
	    }
	}
	if ( !buffer.isEmpty() )
	    addCompletionEntry( buffer );
	
	s->setLastLengthFotCompletion( s->length() );
	s = s->next();
    }
}

void QTextDocument::addCommand( QTextCommand *cmd )
{
    commandHistory->addCommand( cmd );
}

QTextCursor *QTextDocument::undo( QTextCursor *c )
{
    return commandHistory->undo( c );
}

QTextCursor *QTextDocument::redo( QTextCursor *c )
{
    return commandHistory->redo( c );
}

bool QTextDocument::find( const QString &expr, bool cs, bool /*wo*/, bool /*forward*/,
			      int *parag, int *index, QTextCursor *cursor )
{
    // #### wo and forward is ignored at the moment
    QTextParag *p = fParag;
    if ( parag )
	p = paragAt( *parag );
    else if ( cursor )
	p = cursor->parag();
    bool first = TRUE;

    while ( p ) {
	QString s = p->string()->toString();
	int start = 0;
	if ( first && index )
	    start = *index;
	else if ( first )
	    start = cursor->index();
	first = FALSE;
	int res = s.find( expr, start, cs );
	if ( res != -1 ) {
	    cursor->setParag( p );
	    cursor->setIndex( res );
	    setSelectionStart( Standard, cursor );
	    cursor->setIndex( res + expr.length() );
	    setSelectionEnd( Standard, cursor );
	    if ( parag )
		*parag = p->paragId();
	    if ( index )
		*index = res;
	    return TRUE;
	}
	p = p->next();
    }

    return FALSE;
}

void QTextDocument::setTextFormat( Qt::TextFormat f )
{
    txtFormat = f;
}

Qt::TextFormat QTextDocument::textFormat() const
{
    return txtFormat;
}

bool QTextDocument::inSelection( int selId, const QPoint &pos ) const
{
    QMap<int, Selection>::ConstIterator it = selections.find( selId );
    if ( it == selections.end() )
	return FALSE;

    Selection sel = *it;
    QTextParag *startParag = sel.startParag;
    QTextParag *endParag = sel.endParag;
    if ( sel.startParag == sel.endParag &&
	 sel.startParag->selectionStart( selId ) == sel.endParag->selectionEnd( selId ) )
	return FALSE;
    if ( sel.endParag->paragId() < sel.startParag->paragId() ) {
	endParag = sel.startParag;
	startParag = sel.endParag;
    }

    QRect r = startParag->rect();
    r = r.unite( endParag->rect() );

    return r.contains( pos );
}

void QTextDocument::doLayout( QPainter *p, int w )
{
    withoutDoubleBuffer = (bool)p;
    flow_->setWidth( w );
    cw = w;
    vw = w;
    invalidate();
    fCollection->setPainter( p );
    QTextParag *parag = fParag;
    while ( parag ) {
	parag->format();
	parag = parag->next();
    }
}

QPixmap *QTextDocument::bufferPixmap( const QSize &s )
{
    if ( !buf_pixmap ) {
	buf_pixmap = new QPixmap( s );
    } else {
	if ( buf_pixmap->width() < s.width() ||
	     buf_pixmap->height() < s.height() ) {
	    buf_pixmap->resize( QMAX( s.width(), buf_pixmap->width() ),
				QMAX( s.height(), buf_pixmap->height() ) );
	}
    }

    return buf_pixmap;
}

void QTextDocument::draw( QPainter *p, const QRegion &reg, const QColorGroup &cg, const QBrush *paper )
{
    if ( !firstParag() )
	return;

    if ( paper ) {
	p->setBrushOrigin( -(int)p->translationX(),
			   -(int)p->translationY() );
	p->fillRect( reg.boundingRect(), *paper );
    }

    QTextParag *parag = firstParag();
    QRect cr;
    if ( !reg.isNull() )
	cr = reg.boundingRect();
    while ( parag ) {
	if ( !parag->isValid() )
	    parag->format();
	int y = parag->rect().y();
	if ( !reg.isNull() && !cr.isNull() && !cr.intersects( parag->rect() ) ) {
	    parag = parag->next();
	    continue;
	}
	p->translate( 0, y );
	parag->paint( *p, cg, 0, FALSE );
	p->translate( 0, -y );
	parag = parag->next();
    }
}

void QTextDocument::drawParag( QPainter *p, QTextParag *parag, int cx, int cy, int cw, int ch,
			       QPixmap *&doubleBuffer, const QColorGroup &cg,
			       bool drawCursor, QTextCursor *cursor )
{
    QPainter *painter = new QPainter;
    parag->setChanged( FALSE );
    QRect ir( parag->rect() );
    bool useDoubleBuffer = !parag->document()->parent();
    if ( !useDoubleBuffer && parag->document()->nextDoubleBuffered )
	useDoubleBuffer = TRUE;

    if ( useDoubleBuffer  ) {
	if ( cx >= 0 && cy >= 0 )
	    ir = ir.intersect( QRect( cx, cy, cw, ch ) );
	if ( !doubleBuffer ||
	     ir.width() > doubleBuffer->width() ||
	     ir.height() > doubleBuffer->height() ) {
	    doubleBuffer = bufferPixmap( ir.size() );
	    painter->begin( doubleBuffer );
	} else {
	    painter->begin( doubleBuffer );
	}
    } else {
	painter = p;
	painter->translate( ir.x(), ir.y() );
    }

    painter->setBrushOrigin( -ir.x(), -ir.y() );

    if ( useDoubleBuffer ) {
	painter->fillRect( QRect( 0, 0, ir.width(), ir.height() ),
		       cg.brush( QColorGroup::Base ) );
    } else {
	if ( cursor && cursor->parag() == parag ) {
	    painter->fillRect( QRect( parag->at( cursor->index() )->x, 0, 2, ir.height() ),
			       cg.brush( QColorGroup::Base ) );
	}
    }
	
    painter->translate( -( ir.x() - parag->rect().x() ),
		       -( ir.y() - parag->rect().y() ) );
    parag->paint( *painter, cg, drawCursor ? cursor : 0, TRUE, cx, cy, cw, ch );
    if ( !flow()->isEmpty() ) {
	painter->translate( 0, -parag->rect().y() );
	QRect cr( cx, cy, cw, ch );
	cr = cr.intersect( QRect( 0, parag->rect().y(), parag->rect().width(), parag->rect().height() ) );
	flow()->drawFloatingItems( painter, cr.x(), cr.y(), cr.width(), cr.height(), cg );
	painter->translate( 0, +parag->rect().y() );
    }

	
    if ( useDoubleBuffer ) {
	delete painter;
	p->drawPixmap( ir.topLeft(), *doubleBuffer, QRect( QPoint( 0, 0 ), ir.size() ) );
    } else {
	painter->translate( -ir.x(), -ir.y() );
    }

    if ( parag->rect().x() + parag->rect().width() < parag->document()->x() + parag->document()->width() ) {
	p->fillRect( parag->rect().x() + parag->rect().width(), parag->rect().y(),
		     ( parag->document()->x() + parag->document()->width() ) -
		     ( parag->rect().x() + parag->rect().width() ),
		     parag->rect().height(), cg.brush( QColorGroup::Base ) );
    }

    parag->document()->nextDoubleBuffered = FALSE;
}	

QTextParag *QTextDocument::draw( QPainter *p, int cx, int cy, int cw, int ch, const QColorGroup &cg,
				 bool onlyChanged, bool drawCursor, QTextCursor *cursor )
{
    if ( withoutDoubleBuffer || par && par->withoutDoubleBuffer ) {
	withoutDoubleBuffer = TRUE;
	QRegion rg;
	draw( p, rg, cg );
	return 0;
    }
    withoutDoubleBuffer = FALSE;

    if ( !firstParag() )
	return 0;

    if ( drawCursor && cursor )
	tmpCursor = cursor;
    if ( cx < 0 || cy < 0 ) {
	cx = 0;
	cy = 0;
	cw = width();
	ch = height();
    }

    QTextParag *lastFormatted = 0;
    QTextParag *parag = firstParag();

    QPixmap *doubleBuffer = 0;
    QPainter painter;

    while ( parag ) {
	lastFormatted = parag;
	if ( !parag->isValid() )
	    parag->format();
	
	if ( !parag->rect().intersects( QRect( cx, cy, cw, ch ) ) ) {
	    if ( parag->rect().y() > cy + ch ) {
		tmpCursor = 0;
		if ( buf_pixmap && buf_pixmap->height() > 300 ) {
		    delete buf_pixmap;
		    buf_pixmap = 0;
		}

		return lastFormatted;
	    }
	    parag = parag->next();
	    continue;
	}
	
	if ( !parag->hasChanged() && onlyChanged ) {
	    parag = parag->next();
	    continue;
	}
	
	drawParag( p, parag, cx, cy, cw, ch, doubleBuffer, cg, drawCursor, cursor );
	parag = parag->next();
    }

    parag = lastParag();
    if ( parag->rect().y() + parag->rect().height() < parag->document()->height() ) {
	p->fillRect( 0, parag->rect().y() + parag->rect().height(), parag->document()->width(),
		     parag->document()->height() - ( parag->rect().y() + parag->rect().height() ),
		     cg.brush( QColorGroup::Base ) );
	if ( !flow()->isEmpty() ) {
	    QRect cr( cx, cy, cw, ch );
	    cr = cr.intersect( QRect( 0, parag->rect().y() + parag->rect().height(), parag->document()->width(),
				      parag->document()->height() - ( parag->rect().y() + parag->rect().height() ) ) );
	    flow()->drawFloatingItems( p, cr.x(), cr.y(), cr.width(), cr.height(), cg );
	}
    }

    if ( buf_pixmap && buf_pixmap->height() > 300 ) {
	delete buf_pixmap;
	buf_pixmap = 0;
    }

    tmpCursor = 0;
    return lastFormatted;
}

void QTextDocument::setDefaultFont( const QFont &f )
{
    fCollection->defFormat->setFont( f );
}

void QTextDocument::registerCustomItem( QTextCustomItem *i, QTextParag *p )
{
    if ( i && i->placement() != QTextCustomItem::PlaceInline ) {
	flow_->registerFloatingItem( i, i->placement() == QTextCustomItem::PlaceRight );
	p->registerFloatingItem( i );
    }
    customItems.append( i );
}

void QTextDocument::unregisterCustomItem( QTextCustomItem *i, QTextParag *p )
{
    flow_->unregisterFloatingItem( i );
    p->unregisterFloatingItem( i );
    customItems.removeRef( i );
    delete i;
}

bool QTextDocument::focusNextPrevChild( bool next )
{
    if ( !focusIndicator.parag ) {
	if ( next ) {
	    focusIndicator.parag = fParag;
	    focusIndicator.start = 0;
	    focusIndicator.len = 0;
	} else {
	    focusIndicator.parag = lParag;
	    focusIndicator.start = lParag->length();
	    focusIndicator.len = 0;
	}
    } else {
	focusIndicator.parag->setChanged( TRUE );
    }
    focusIndicator.href = QString::null;

    if ( next ) {
	QTextParag *p = focusIndicator.parag;
	int index = focusIndicator.start + focusIndicator.len;
	while ( p ) {
	    for ( int i = index; i < p->length(); ++i ) {
		if ( p->at( i )->format()->isAnchor() ) {
		    p->setChanged( TRUE );
		    focusIndicator.parag = p;
		    focusIndicator.start = i;
		    focusIndicator.len = 0;
		    focusIndicator.href = p->at( i )->format()->anchorHref();
		    while ( i < p->length() ) {
			if ( !p->at( i )->format()->isAnchor() )
			    return TRUE;
			focusIndicator.len++;
			i++;
		    }
		}
	    }
	    index = 0;
	    p = p->next();
	}
    } else {
	QTextParag *p = focusIndicator.parag;
	int index = focusIndicator.start - 1;
	while ( p ) {
	    for ( int i = index; i >= 0; --i ) {
		if ( p->at( i )->format()->isAnchor() ) {
		    p->setChanged( TRUE );
		    focusIndicator.parag = p;
		    focusIndicator.start = i;
		    focusIndicator.len = 0;
		    focusIndicator.href = p->at( i )->format()->anchorHref();
		    while ( i >= -1 ) {
			if ( i < 0 || !p->at( i )->format()->isAnchor() ) {
			    focusIndicator.start++;
			    return TRUE;
			}
			if ( i < 0 )
			    break;
			focusIndicator.len++;
			focusIndicator.start--;
			i--;
		    }
		}
	    }
	    p = p->prev();
	    if ( p )
		index = p->length() - 1;
	}
    }

    return FALSE;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

QTextString::QTextString()
{
    textChanged = FALSE;
    bidi = FALSE;
    rightToLeft = FALSE;
}

void QTextString::insert( int index, const QString &s, QTextFormat *f )
{
    int os = data.size();
    data.resize( data.size() + s.length() );
    if ( index < os ) {
	memmove( data.data() + index + s.length(), data.data() + index,
		 sizeof( Char ) * ( os - index ) );
    }
    for ( int i = 0; i < (int)s.length(); ++i ) {
	data[ (int)index + i ].x = 0;
	data[ (int)index + i ].lineStart = 0;
	data[ (int)index + i ].d = 0;
	data[ (int)index + i ].isCustom = 0;
	data[ (int)index + i ].rightToLeft = 0;
#if defined(_WS_X11_)
	//### workaround for broken courier fonts on X11
	if ( s[ i ] == QChar( 0x00a0U ) )
	    data[ (int)index + i ].c = ' ';
	else
	    data[ (int)index + i ].c = s[ i ];
#else
	data[ (int)index + i ].c = s[ i ];
#endif
	data[ (int)index + i ].setFormat( f );
    }
    cache.insert( index, s );
    textChanged = TRUE;
}

QTextString::~QTextString()
{
    for ( int i = 0; i < (int)data.count(); ++i ) {
	if ( data[ i ].isCustom )
	    delete data[ i ].customItem();
	else if ( data[ i ].format() )
	    data[ i ].format()->removeRef();
    }
}

void QTextString::insert( int index, Char *c )
{
    int os = data.size();
    data.resize( data.size() + 1 );
    if ( index < os ) {
	memmove( data.data() + index + 1, data.data() + index,
		 sizeof( Char ) * ( os - index ) );
    }
    data[ (int)index ].c = c->c;
    data[ (int)index ].setFormat( c->format() );
    data[ (int)index ].x = 0;
    data[ (int)index ].lineStart = 0;
    data[ (int)index ].rightToLeft = 0;
    data[ (int)index ].d = 0;
    data[ (int)index ].isCustom = 0;
    cache.insert( index, QString::null );
    textChanged = TRUE;
}

void QTextString::truncate( int index )
{
    data.truncate( index );
    cache.truncate( index );
    textChanged = TRUE;
}

void QTextString::remove( int index, int len )
{
    memmove( data.data() + index, data.data() + index + len,
	     sizeof( Char ) * ( data.size() - index - len ) );
    data.resize( data.size() - len );
    cache.remove( index, len );
    textChanged = TRUE;
}

void QTextString::setFormat( int index, QTextFormat *f, bool useCollection )
{
    if ( useCollection && data[ index ].format() )
	data[ index ].format()->removeRef();
    data[ index ].setFormat( f );
}

void QTextString::checkBidi() const
{
    int len = data.size();
    const Char *c = data.data();
    ((QTextString *)this)->bidi = FALSE;
    ((QTextString *)this)->rightToLeft = FALSE;
    while( len ) {
	unsigned char row = c->c.row();
	if( (row > 0x04 && row < 0x09) || row > 0xfa ) {
	    ((QTextString *)this)->bidi = TRUE;
	    basicDirection();
	    return;
	}
	len--;
	++c;
    }
}

void QTextString::basicDirection() const
{
    int pos = 0;
    ((QTextString *)this)->rightToLeft = FALSE;
    while( pos < length() ) {
	switch( at(pos).c.direction() )
	{
	case QChar::DirL:
	case QChar::DirLRO:
	case QChar::DirLRE:
	    return;
	case QChar::DirR:
	case QChar::DirAL:
	case QChar::DirRLO:
	case QChar::DirRLE:
	    ((QTextString *)this)->rightToLeft = TRUE;
	    return;
	default:
	    break;
	}
	++pos;
    }
    return;
}



// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

QTextParag::QTextParag( QTextDocument *d, QTextParag *pr, QTextParag *nx, bool updateIds )
    : invalid( -1 ), p( pr ), n( nx ), doc( d ), align( -1 ), numSubParag( -1 ),
      tm( -1 ), bm( -1 ), lm( -1 ), rm( -1 ), defFormat( d->formatCollection()->defaultFormat() ), tc( 0 ),
      numCustomItems( 0 )
{
#if defined(PARSER_DEBUG)
    qDebug( debug_indent + "new QTextParag" );
#endif
    fullWidth = TRUE;

    if ( p ) {
	p->n = this;
	if ( p->tc )
	    tc = p->tc;
    }
    if ( n ) {
	n->p = this;
	if ( n->tc )
	    tc = n->tc;
    }

    if ( !tc && d->tableCell() )
	tc = d->tableCell();

    if ( !p )
	doc->setFirstParag( this );
    if ( !n )
	doc->setLastParag( this );

    changed = FALSE;
    firstFormat = TRUE;
    state = -1;
    needHighlighte = FALSE;

    if ( p )
	id = p->id + 1;
    else
	id = 0;
    if ( n && updateIds ) {
	QTextParag *s = n;
	while ( s ) {
	    s->id = s->p->id + 1;
	    s->numSubParag = -1;
	    s->lm = s->rm = s->tm = s->bm = -1;
	    s = s->n;
	}
    }
    firstHilite = TRUE;
    lastLenForCompletion = -1;

    str = new QTextString();
    str->insert( 0, " ", d->formatCollection()->defaultFormat() );
}

QTextParag::~QTextParag()
{
    if ( p == doc->minwParag ) {
	doc->minwParag = 0;
	doc->minw = 0;
    }	
}

void QTextParag::setNext( QTextParag *s )
{
    n = s;
    if ( !n )
	doc->setLastParag( this );
}

void QTextParag::setPrev( QTextParag *s )
{
    p = s;
    if ( !p )
	doc->setFirstParag( this );
}

void QTextParag::invalidate( int chr )
{
    if ( invalid < 0 )
	invalid = chr;
    else
	invalid = QMIN( invalid, chr );
    for ( QTextCustomItem *i = floatingItems.first(); i; i = floatingItems.next() )
	i->ypos = -1;
}

void QTextParag::insert( int index, const QString &s )
{
    if ( !doc->useFormatCollection() && doc->syntaxHighlighter() )
	str->insert( index, s,
		     doc->syntaxHighlighter()->format( QTextSyntaxHighlighter::Standard ) );
    else
	str->insert( index, s, doc->formatCollection()->defaultFormat() );
    invalidate( index );
    needHighlighte = TRUE;
}

void QTextParag::truncate( int index )
{
    str->truncate( index );
    insert( length(), " " );
    needHighlighte = TRUE;
}

void QTextParag::remove( int index, int len )
{
    for ( int i = index; i < len; ++i ) {
	QTextString::Char *c = at( i );
	if ( c->isCustom ) {
	    doc->unregisterCustomItem( c->customItem(), this );
	    removeCustomItem();
	}
    }
    str->remove( index, len );
    invalidate( 0 );
    needHighlighte = TRUE;
}

void QTextParag::join( QTextParag *s )
{
    int oh = r.height() + s->r.height();
    n = s->n;
    if ( n )
	n->p = this;
    else
	doc->setLastParag( this );

    int start = str->length() - 1;
    if ( at( length() - 1 )->c == ' ' )
	remove( length() - 1, 1 );
    append( s->str->toString(), TRUE );
    if ( doc->useFormatCollection() ) {
	for ( int i = 0; i < s->length(); ++i ) {
	    s->str->at( i ).format()->addRef();
	    str->setFormat( i + start, s->str->at( i ).format(), TRUE );
	}
    }
    delete s;
    invalidate( 0 );
    r.setHeight( oh );
    format();
    needHighlighte = TRUE;
    if ( n ) {
	QTextParag *s = n;
	while ( s ) {
	    s->id = s->p->id + 1;
	    s->state = -1;
	    s->needHighlighte = TRUE;
	    s->changed = TRUE;
	    s = s->n;
	}
    }
    state = -1;
}

void QTextParag::move( int dy )
{
    if ( dy == 0 )
	return;
    changed = TRUE;
    r.moveBy( 0, dy );
    for ( QTextCustomItem *i = floatingItems.first(); i; i = floatingItems.next() )
	i->ypos += dy;
    if ( doc->verticalBreak() ) {
	const int oy = r.y();
	int y = oy;
	doc->flow()->adjustFlow( y, r.width(), r.height(), TRUE );
	if ( oy != y )
	    r.setY( y );
    }
}

void QTextParag::format( int start, bool doMove )
{
    if ( str->length() == 0 || !doc->formatter() )
	return;

    if ( doc->syntaxHighlighter() &&
	 ( needHighlighte || state == -1 ) )
	doc->syntaxHighlighter()->highlighte( doc, this, invalid <= 0 ? 0 : invalid );
    needHighlighte = FALSE;

    if ( invalid == -1 )
	return;

    r.moveTopLeft( QPoint( doc->x(), p ? p->r.y() + p->r.height() : doc->y() ) );
    r.setWidth( doc->width() );
    for ( QTextCustomItem *i = floatingItems.first(); i; i = floatingItems.next() ) {
	i->ypos = r.y();
	if ( i->placement() == QTextCustomItem::PlaceRight )
	    i->xpos = r.x() + r.width() - i->width;
	doc->flow()->updateHeight( i );
    }
    QMap<int, LineStart*> oldLineStarts = lineStarts;
    lineStarts.clear();
    int y = doc->formatter()->format( doc, this, start, oldLineStarts );
    r.setWidth( QMAX( r.width(), doc->minimumWidth() ) );
    QMap<int, LineStart*>::Iterator it = oldLineStarts.begin();
    for ( ; it != oldLineStarts.end(); ++it )
	delete *it;

    QTextString::Char *c = 0;
    if ( lineStarts.count() == 1 && doc->flow()->isEmpty() && !string()->isBidi() ) {
	c = &str->at( str->length() - 1 );
	r.setWidth( c->x + c->format()->width( c->c ) );
    }

    if ( y != r.height() )
	r.setHeight( y );

    if ( doc->verticalBreak() ) {
	const int oy = r.y();
	int y = oy;
	doc->flow()->adjustFlow( y, r.width(), r.height(), TRUE );
	if ( oy != y ) {
	    r.setY( y );
	}
    }

    if ( n && doMove && n->invalid == -1 && r.y() + r.height() != n->r.y() ) {
	int dy = ( r.y() + r.height() ) - n->r.y();
	QTextParag *s = n;
	bool makeInvalid = FALSE;
	while ( s ) {
 	    if ( !s->isFullWidth() ) // ######### check this, might be a bit inefficient!
		makeInvalid = TRUE;
	    if ( makeInvalid )
		s->invalidate( 0 );
	    s->move( dy );
	    s = s->n;
	}
    }

    firstFormat = FALSE;
    changed = TRUE;
    invalid = -1;
    string()->setTextChanged( FALSE );
}

int QTextParag::lineHeightOfChar( int i, int *bl, int *y ) const
{
    if ( !isValid() )
	( (QTextParag*)this )->format();

    QMap<int, LineStart*>::ConstIterator it = lineStarts.end();
    --it;
    for ( ;; ) {
	if ( i >= it.key() ) {
	    if ( bl )
		*bl = ( *it )->baseLine;
	    if ( y )
		*y = ( *it )->y;
	    return ( *it )->h;
	}
	if ( it == lineStarts.begin() )
	    break;
	--it;
    }
	
    qWarning( "QTextParag::lineHeightOfChar: couldn't find lh for %d", i );
    return 15;
}

QTextString::Char *QTextParag::lineStartOfChar( int i, int *index, int *line ) const
{
    if ( !isValid() )
	( (QTextParag*)this )->format();

    int l = lineStarts.count() - 1;
    QMap<int, LineStart*>::ConstIterator it = lineStarts.end();
    --it;
    for ( ;; ) {
	if ( i >= it.key() ) {
	    if ( index )
		*index = it.key();
	    if ( line )
		*line = l;
	    return &str->at( it.key() );
	}
	if ( it == lineStarts.begin() )
	    break;
	--it;
	--l;
    }

    qWarning( "QTextParag::lineStartOfChar: couldn't find %d", i );
    return 0;
}

int QTextParag::lines() const
{
    if ( !isValid() )
	( (QTextParag*)this )->format();

    return lineStarts.count();
}

QTextString::Char *QTextParag::lineStartOfLine( int line, int *index ) const
{
    if ( !isValid() )
	( (QTextParag*)this )->format();

    if ( line >= 0 && line < (int)lineStarts.count() ) {
	QMap<int, LineStart*>::ConstIterator it = lineStarts.begin();
	while ( line-- > 0 )
	    ++it;
	int i = it.key();
	if ( index )
	    *index = i;
	return &str->at( i );
    }

    qWarning( "QTextParag::lineStartOfLine: couldn't find %d", line );
    return 0;
}

void QTextParag::setFormat( int index, int len, QTextFormat *f, bool useCollection, int flags )
{
    if ( index < 0 )
	index = 0;
    if ( index > str->length() - 1 )
	index = str->length() - 1;
    if ( index + len > str->length() )
	len = str->length() - 1 - index;

    QTextFormatCollection *fc = 0;
    if ( useCollection )
	fc = doc->formatCollection();
    QTextFormat *of;
    for ( int i = 0; i < len; ++i ) {
	of = str->at( i + index ).format();
	if ( !changed && f->key() != of->key() )
	    changed = TRUE;
	if ( invalid == -1 &&
	     ( f->font().family() != of->font().family() ||
	       f->font().pointSize() != of->font().pointSize() ||
	       f->font().weight() != of->font().weight() ||
	       f->font().italic() != of->font().italic() ) ) {
	    invalidate( 0 );
	}
	if ( flags == -1 || flags == QTextFormat::Format || !fc ) {
	    if ( fc )
		f = fc->format( f );
	    str->setFormat( i + index, f, useCollection );
	} else {
	    QTextFormat *fm = fc->format( of, f, flags );
	    str->setFormat( i + index, fm, useCollection );
	}
    }
}

void QTextParag::indent( int *oldIndent, int *newIndent )
{
    if ( !doc->indent() || style() && style()->displayMode() != QStyleSheetItem::DisplayBlock ) {
	if ( oldIndent )
	    *oldIndent = 0;
	if ( newIndent )
	    *newIndent = 0;
	if ( oldIndent && newIndent )
	    *newIndent = *oldIndent;
	return;
    }
    doc->indent()->indent( doc, this, oldIndent, newIndent );
}

void QTextParag::paint( QPainter &painter, const QColorGroup &cg, QTextCursor *cursor, bool drawSelections,
			int clipx, int clipy, int clipw, int cliph )
{
    QTextString::Char *chr = at( 0 );
    int i = 0;
    int h = 0;
    int baseLine = 0, lastBaseLine = 0;
    QTextFormat *lastFormat = 0;
    int lastY = -1;
    QString buffer;
    int startX = 0;
    int bw = 0;
    int cy = 0;
    int curx = -1, cury, curh;
    bool lastDirection = 0;

    int selectionStarts[ doc->numSelections ];
    int selectionEnds[ doc->numSelections ];
    if ( drawSelections ) {
	bool hasASelection = FALSE;
	for ( i = 0; i < doc->numSelections; ++i ) {
	    if ( !hasSelection( i ) ) {
		selectionStarts[ i ] = -1;
		selectionEnds[ i ] = -1;
	    } else {
		hasASelection = TRUE;
		selectionStarts[ i ] = selectionStart( i );
		int end = selectionEnd( i );
		if ( end == length() - 1 && n && n->hasSelection( i ) )
		    end++;
		selectionEnds[ i ] = end;
	    }
	}
	if ( !hasASelection )
	    drawSelections = FALSE;
    }
	
    int line = -1;
    int cw;
    bool didListLabel = FALSE;
    for ( i = 0; i < length(); i++ ) {
	chr = at( i );
	cw = chr->width();

	// init a new line
	if ( chr->lineStart ) {
	    ++line;
	    lineInfo( line, cy, h, baseLine );
	    if ( lastBaseLine == 0 )
		lastBaseLine = baseLine;
	}
	
	// draw bullet list items
	if ( !didListLabel && line == 0 && style() && style()->displayMode() == QStyleSheetItem::DisplayListItem ) {
	    didListLabel = TRUE;
	    drawLabel( &painter, leftMargin(), cy, 0, 0, baseLine, cg );
	}

	// check for cursor mark
	if ( cursor && this == cursor->parag() && i == cursor->index() ) {
	    curx = chr->x;
	    if ( chr->rightToLeft )
		curx += cw;
	    curh = h;
	    cury = cy;
	}
	
	// first time - start again...
	if ( !lastFormat || lastY == -1 ) {
	    lastFormat = chr->format();
	    lastY = cy;
	    startX = chr->x;
	    if ( !chr->isCustom )
		buffer += chr->c;
	    bw = cw;
	    if ( !chr->isCustom )
		continue;
	}
	
	// check if selection state changed
	bool selectionChange = FALSE;
	if ( drawSelections ) {
	    for ( int j = 0; j < doc->numSelections; ++j ) {
		selectionChange = selectionStarts[ j ] == i || selectionEnds[ j ] == i;
		if ( selectionChange )
		    break;
	    }
	}

	//if something (format, etc.) changed, draw what we have so far
	if ( ( ( alignment() & Qt::AlignJustify ) == Qt::AlignJustify && buffer.length() > 0 && buffer[ (int)buffer.length() - 1 ].isSpace() ) ||
	     lastDirection != chr->rightToLeft || chr->rightToLeft ||
	     lastY != cy || chr->format() != lastFormat || buffer == "\t" || chr->c == '\t' ||
	     selectionChange || chr->isCustom ) {
	    drawParagBuffer( painter, buffer, startX, lastY, lastBaseLine, bw, h, drawSelections,
			     lastFormat, i, selectionStarts, selectionEnds, cg );
	    if ( !chr->isCustom ) {
		buffer = chr->c;
		lastFormat = chr->format();
		lastY = cy;
		startX = chr->x;
		bw = cw;
	    } else {
		if ( chr->customItem()->placement() == QTextCustomItem::PlaceInline ) {
		    chr->customItem()->draw( &painter, chr->x, cy, clipx - r.x(), clipy - r.y(), clipw, cliph, cg );
		    buffer = QString::null;
		    lastFormat = chr->format();
		    lastY = cy;
		    startX = chr->x + chr->width();
		    bw = 0;
		} else {
		    chr->customItem()->resize( 0, chr->customItem()->width );
		    buffer = QString::null;
		    lastFormat = chr->format();
		    lastY = cy;
		    startX = chr->x + chr->width();
		    bw = 0;
		}
	    }
	} else {
	    buffer += chr->c;
	    bw += cw;
	}
	lastBaseLine = baseLine;
	lastDirection = chr->rightToLeft;
    }
	
    // if we are through thg parag, but still have some stuff left to draw, draw it now
    if ( !buffer.isEmpty() ) {
	bool selectionChange = FALSE;
	if ( drawSelections ) {
	    for ( int j = 0; j < doc->numSelections; ++j ) {
		selectionChange = selectionStarts[ j ] == i || selectionEnds[ j ] == i;
		if ( selectionChange )
		    break;
	    }
	}
	drawParagBuffer( painter, buffer, startX, lastY, lastBaseLine, bw, h, drawSelections,
			 lastFormat, i, selectionStarts, selectionEnds, cg );
    }
	
    // if we should draw a cursor, draw it now
    if ( curx != -1 && cursor ) {
	painter.fillRect( QRect( curx, cury, 1, curh ), Qt::black );
	painter.save();
	if ( string()->isBidi() ) {
	    int d = curh / 3;
	    if ( at( cursor->index() )->rightToLeft ) {
		painter.setPen( Qt::black );
		painter.drawLine( curx, cury, curx - d / 2, cury + d / 2 );
		painter.drawLine( curx, cury + d, curx - d / 2, cury + d / 2 );
	    } else {
		painter.setPen( Qt::black );
		painter.drawLine( curx, cury, curx + d / 2, cury + d / 2 );
		painter.drawLine( curx, cury + d, curx + d / 2, cury + d / 2 );
	    }
	}
	painter.restore();
    }
	
}

void QTextParag::drawParagBuffer( QPainter &painter, const QString &buffer, int startX,
				      int lastY, int baseLine, int bw, int h, bool drawSelections,
				      QTextFormat *lastFormat, int i, int *selectionStarts,
				      int *selectionEnds, const QColorGroup &cg )
{
    painter.setPen( QPen( lastFormat->color() ) );
    painter.setFont( lastFormat->font() );

    if ( lastFormat->isAnchor() && !lastFormat->anchorHref().isEmpty() ) {
	painter.setPen( QPen( doc->linkColor() ) );
	if ( doc->underlineLinks() ) {
	    QFont fn = lastFormat->font();
	    fn.setUnderline( TRUE );
	    painter.setFont( fn );	
	}
    }

    if ( drawSelections ) {
	for ( int j = 0; j < doc->numSelections; ++j ) {
	    if ( i > selectionStarts[ j ] && i <= selectionEnds[ j ] ) {
		if ( doc->invertSelectionText( j ) )
		    painter.setPen( QPen( cg.color( QColorGroup::HighlightedText ) ) );
		painter.fillRect( startX, lastY, bw, h, doc->selectionColor( j ) );
	    }
	}
    }
    if ( buffer != "\t" )
	painter.drawText( startX, lastY + baseLine, buffer );
    if ( lastFormat->isMisspelled() ) {
	painter.save();
	painter.setPen( QPen( Qt::red, 1, Qt::DotLine ) );
	painter.drawLine( startX, lastY + baseLine + 1, startX + bw, lastY + baseLine + 1 );
	painter.restore();
    }

    i -= buffer.length();
    if ( lastFormat->isAnchor() && !lastFormat->anchorHref().isEmpty() &&
	 doc->focusIndicator.parag == this &&
 	 doc->focusIndicator.start >= i &&
  	 doc->focusIndicator.start + doc->focusIndicator.len <= i + (int)buffer.length() ) {
	painter.drawWinFocusRect( QRect( startX, lastY, bw, h ) );
    }

}

void QTextParag::drawLabel( QPainter* p, int x, int y, int w, int h, int base, const QColorGroup& cg )
{
    if ( !style() )
	return;
    QRect r ( x, y, w, h );
    QStyleSheetItem::ListStyle s = listStyle();

    QFont font = p->font();
    p->setFont( defFormat->font() );
    QFontMetrics fm( p->fontMetrics() );
    int size = fm.lineSpacing() / 3;

    switch ( s ) {
    case QStyleSheetItem::ListDecimal:
    case QStyleSheetItem::ListLowerAlpha:
    case QStyleSheetItem::ListUpperAlpha:
	{
	    int n = numberOfSubParagraph();
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
	    p->drawText( r.right() - fm.width( l ), r.top() + base, l );
	}
	break;
    case QStyleSheetItem::ListSquare:
	{
 	    QRect er( r.right() - size * 2, r.top() + base - fm.boundingRect( 'A' ).height() / 2 - size / 2 - 1, size, size );
	    p->fillRect( er , cg.brush( QColorGroup::Foreground ) );
	}
	break;
    case QStyleSheetItem::ListCircle:
	{
	    QRect er( r.right()-size*2, r.top() + base - fm.boundingRect('A').height()/2 - size/2 - 1, size, size);
	    p->drawEllipse( er );
	}
	break;
    case QStyleSheetItem::ListDisc:
    default:
	{
	    p->setBrush( cg.brush( QColorGroup::Foreground ));
	    QRect er( r.right()-size*2, r.top() + base - fm.boundingRect('A').height()/2 - size/2 - 1, size, size);
	    p->drawEllipse( er );
	    p->setBrush( Qt::NoBrush );
	}
	break;
    }

    p->setFont( font );
}

void QTextParag::setStyleSheetItems( const QVector<QStyleSheetItem> &vec )
{
    styleSheetItemsVec = vec;
}

void QTextParag::setList( bool b, int listStyle )
{
    if ( !style() ) {
	styleSheetItemsVec.resize( 2 );
	styleSheetItemsVec.insert( 0, doc->styleSheet()->item( "html" ) );
	styleSheetItemsVec.insert( 0, doc->styleSheet()->item( "html" ) );
	styleSheetItemsVec.insert( 1, doc->styleSheet()->item( "p" ) );
    }

    if ( b ) {
	if ( style()->displayMode() != QStyleSheetItem::DisplayListItem || this->listStyle() != listStyle ) {
	    styleSheetItemsVec.remove( styleSheetItemsVec.size() - 1 );
	    QStyleSheetItem *item = styleSheetItemsVec[ styleSheetItemsVec.size() - 2 ];
	    if ( item )
		styleSheetItemsVec.remove( styleSheetItemsVec.size() - 2 );
	    styleSheetItemsVec.insert( styleSheetItemsVec.size() - 2,
				       listStyle == QStyleSheetItem::ListDisc || listStyle == QStyleSheetItem::ListCircle
				       || listStyle == QStyleSheetItem::ListSquare ?
				       doc->styleSheet()->item( "ul" ) : doc->styleSheet()->item( "ol" ) );
	    styleSheetItemsVec.insert( styleSheetItemsVec.size() - 1, doc->styleSheet()->item( "li" ) );
	    setListStyle( (QStyleSheetItem::ListStyle)listStyle );
	} else {
	    return;
	}
    } else {
	if ( style()->displayMode() != QStyleSheetItem::DisplayBlock ) {
	    styleSheetItemsVec.remove( styleSheetItemsVec.size() - 1 );
	    if ( styleSheetItemsVec.size() >= 2 ) {
		styleSheetItemsVec.remove( styleSheetItemsVec.size() - 2 );
		styleSheetItemsVec.resize( styleSheetItemsVec.size() - 2 );
	    } else {
		styleSheetItemsVec.resize( styleSheetItemsVec.size() - 1 );
	    }
	} else {
	    return;
	}
    }
    invalidate( 0 );
    lm = rm = tm = bm = -1;
    numSubParag = -1;
    if ( next() ) {
	QTextParag *s = next();
	while ( s ) {
	    s->numSubParag = -1;
	    s->lm = s->rm = s->tm = s->bm = -1;
	    s->numSubParag = -1;
	    s->invalidate( 0 );
	    s = s->next();
	}
    }
}

void QTextParag::incDepth()
{
    if ( !style() )
	return;
    if ( style()->displayMode() != QStyleSheetItem::DisplayListItem )
	return;
    styleSheetItemsVec.resize( styleSheetItemsVec.size() + 1 );
    styleSheetItemsVec.insert( styleSheetItemsVec.size() - 1, styleSheetItemsVec[ styleSheetItemsVec.size() - 2 ] );
    styleSheetItemsVec.insert( styleSheetItemsVec.size() - 2,
			       listStyle() == QStyleSheetItem::ListDisc || listStyle() == QStyleSheetItem::ListCircle ||
			       listStyle() == QStyleSheetItem::ListSquare ?
			       doc->styleSheet()->item( "ul" ) : doc->styleSheet()->item( "ol" ) );
    invalidate( 0 );
    lm = -1;
}

void QTextParag::decDepth()
{
    if ( !style() )
	return;
    if ( style()->displayMode() != QStyleSheetItem::DisplayListItem )
	return;
    int numLists = 0;
    QStyleSheetItem *lastList = 0;
    int lastIndex, i;
    for ( i = 0; i < (int)styleSheetItemsVec.size(); ++i ) {
	QStyleSheetItem *item = styleSheetItemsVec[ i ];
	if ( item->name() == "ol" || item->name() == "ul" ) {
	    lastList = item;
	    lastIndex = i;
	    numLists++;
	}
    }

    if ( !lastList )
	return;
    styleSheetItemsVec.remove( lastIndex );
    for ( i = lastIndex; i < (int)styleSheetItemsVec.size() - 1; ++i )
	styleSheetItemsVec.insert( i, styleSheetItemsVec[ i + 1 ] );
    styleSheetItemsVec.resize( styleSheetItemsVec.size() - 1 );
    if ( numLists == 1 )
	setList( FALSE, -1 );
    invalidate( 0 );
    lm = -1;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


QTextSyntaxHighlighter::QTextSyntaxHighlighter()
{
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

QTextFormatter::QTextFormatter()
{
}

/* only used for bidi or complex text reordering
 */
QTextParag::LineStart *QTextFormatter::formatLine( QTextString *string, QTextParag::LineStart *line,
				 QTextString::Char *startChar, QTextString::Char *lastChar, int align, int space )
{
    if( string->isBidi() )
	return bidiReorderLine( string, line, startChar, lastChar, align, space );

    space = QMAX( space, 0 ); // #### with nested tables this gets negative because of a bug I didn't find yet, so workaround for now. This also means non-left aligned nested tables do not work at the moment
    int start = (startChar - &string->at(0));
    int last = (lastChar - &string->at(0) );
    // do alignment Auto == Left in this case
    if ( align & Qt::AlignHCenter || align & Qt::AlignRight ) {
	if ( align & Qt::AlignHCenter )
	    space /= 2;
	for ( int j = start; j <= last; ++j )
	    string->at( j ).x += space;
    } else if ( align & Qt::AlignJustify ) {
	int numSpaces = 0;
	for ( int j = start; j < last; ++j ) {
	    if( isBreakable( string, j ) ) {
		numSpaces++;
	    }
	}
	int toAdd = 0;
	for ( int k = start + 1; k <= last; ++k ) {
	    if( isBreakable( string, k ) && numSpaces ) {
		int s = space / numSpaces;
		toAdd += s;
		space -= s;
		numSpaces--;
	    }
	    string->at( k ).x += toAdd;
	}
    }	

    return new QTextParag::LineStart();
}

struct QTextBidiRun {
    QTextBidiRun(int _start, int _stop, QTextBidiContext *context, QChar::Direction dir) {
	start = _start;
	stop = _stop;
	if(dir == QChar::DirON) dir = context->dir;

	level = context->level;

	// add level of run (cases I1 & I2)
	if( level % 2 ) {
	    if(dir == QChar::DirL || dir == QChar::DirAN)
		level++;
	} else {
	    if( dir == QChar::DirR )
		level++;
	    else if( dir == QChar::DirAN )
		level += 2;
	}
	//printf("new run: level = %d\n", level);
    }

    int start;
    int stop;
    // explicit + implicit levels here
    uchar level;
};

//#define BIDI_DEBUG 1
#ifdef BIDI_DEBUG
#include <iostream>
#endif

// collects one line of the paragraph and transforms it to visual order
QTextParag::LineStart *QTextFormatter::bidiReorderLine( QTextString *text, QTextParag::LineStart *line,
				 QTextString::Char *startChar, QTextString::Char *lastChar, int align, int space )
{
    int start = (startChar - &text->at(0));
    int last = (lastChar - &text->at(0) );
    //printf("doing BiDi reordering from %d to %d!\n", start, last);

    QList<QTextBidiRun> runs;
    runs.setAutoDelete(TRUE);

    QTextBidiContext *context = line->context();
    if ( !context ) {
	// first line
	if( start != 0 )
	    qDebug( "bidiReorderLine::internal error");
	if( text->isRightToLeft() )
	    context = new QTextBidiContext( 1, QChar::DirR );
	else
	    context = new QTextBidiContext( 0, QChar::DirL );
    }
    context->ref();

    QTextBidiStatus status = line->status;
    QChar::Direction dir = QChar::DirON;

    int sor = start;
    int eor = start;

    int current = start;
    while(current < last) {
	QChar::Direction dirCurrent;
	if(current == text->length()) {
	    QTextBidiContext *c = context;
	    while ( c->parent )
		c = c->parent;
	    dirCurrent = c->dir;
	} else
	    dirCurrent = text->at(current).c.direction();

	
#if BIDI_DEBUG > 1
	cout << "directions: dir=" << dir << " current=" << dirCurrent << " last=" << status.last << " eor=" << status.eor << " lastStrong=" << status.lastStrong << " embedding=" << context->dir << " level =" << (int)context->level << endl;
#endif
	
	switch(dirCurrent) {

	    // embedding and overrides (X1-X9 in the BiDi specs)
	case QChar::DirRLE:
	    {
		unsigned char level = context->level;
		if(level%2) // we have an odd level
		    level += 2;
		else
		    level++;
		if(level < 61) {
		    runs.append( new QTextBidiRun(sor, eor, context, dir) );
		    ++eor; sor = eor; dir = QChar::DirON; status.eor = QChar::DirON;
		    context = new QTextBidiContext(level, QChar::DirR, context);
		    context->ref();
		    status.last = QChar::DirR;
		    status.lastStrong = QChar::DirR;
		}
		break;
	    }
	case QChar::DirLRE:
	    {
		unsigned char level = context->level;
		if(level%2) // we have an odd level
		    level++;
		else
		    level += 2;
		if(level < 61) {
		    runs.append( new QTextBidiRun(sor, eor, context, dir) );	
		    ++eor; sor = eor; dir = QChar::DirON; status.eor = QChar::DirON;
		    context = new QTextBidiContext(level, QChar::DirL, context);
		    context->ref();
		    status.last = QChar::DirL;
		    status.lastStrong = QChar::DirL;
		}
		break;
	    }
	case QChar::DirRLO:
	    {
		unsigned char level = context->level;
		if(level%2) // we have an odd level
		    level += 2;
		else
		    level++;
		if(level < 61) {
		    runs.append( new QTextBidiRun(sor, eor, context, dir) );
		    ++eor; sor = eor; dir = QChar::DirON; status.eor = QChar::DirON;
		    context = new QTextBidiContext(level, QChar::DirR, context, TRUE);
		    context->ref();
		    dir = QChar::DirR;
		    status.last = QChar::DirR;
		    status.lastStrong = QChar::DirR;
		}
		break;
	    }
	case QChar::DirLRO:
	    {
		unsigned char level = context->level;
		if(level%2) // we have an odd level
		    level++;
		else
		    level += 2;
		if(level < 61) {
		    runs.append( new QTextBidiRun(sor, eor, context, dir) );
		    ++eor; sor = eor; dir = QChar::DirON; status.eor = QChar::DirON;
		    context = new QTextBidiContext(level, QChar::DirL, context, TRUE);
		    context->ref();
		    dir = QChar::DirL;
		    status.last = QChar::DirL;
		    status.lastStrong = QChar::DirL;
		}
		break;
	    }
	case QChar::DirPDF:
	    {
		QTextBidiContext *c = context->parent;
		if(c) {
		    runs.append( new QTextBidiRun(sor, eor, context, dir) );
		    ++eor; sor = eor; dir = QChar::DirON; status.eor = QChar::DirON;
		    status.last = context->dir;
		    context->deref();
		    context = c;
		    if(context->override)
			dir = context->dir;
		    else
			dir = QChar::DirON;
		    status.lastStrong = context->dir;
		}		
		break;
	    }
	
	    // strong types
	case QChar::DirL:
	    if(dir == QChar::DirON)
		dir = QChar::DirL;
	    switch(status.last)
		{
		case QChar::DirL:
		    eor = current; status.eor = QChar::DirL; break;
		case QChar::DirR:
		case QChar::DirAL:
		case QChar::DirEN:
		case QChar::DirAN:
		    runs.append( new QTextBidiRun(sor, eor, context, dir) );
		    ++eor; sor = eor; dir = QChar::DirON; status.eor = QChar::DirON;
		    break;
		case QChar::DirES:
		case QChar::DirET:
		case QChar::DirCS:
		case QChar::DirBN:
		case QChar::DirB:
		case QChar::DirS:
		case QChar::DirWS:
		case QChar::DirON:
		    if(dir != QChar::DirL) {
			//last stuff takes embedding dir
			if( context->dir == QChar::DirR ) {
			    if(status.eor != QChar::DirR) {
				// AN or EN
				runs.append( new QTextBidiRun(sor, eor, context, dir) );
				++eor; sor = eor; dir = QChar::DirON; status.eor = QChar::DirON;
				dir = QChar::DirR;
			    }
			    else
				eor = current - 1;
			    runs.append( new QTextBidiRun(sor, eor, context, dir) );
			    ++eor; sor = eor; dir = QChar::DirON; status.eor = QChar::DirON;
			} else {
			    if(status.eor == QChar::DirR) {
				runs.append( new QTextBidiRun(sor, eor, context, dir) );
				++eor; sor = eor; dir = QChar::DirON; status.eor = QChar::DirON;
				dir = QChar::DirL;
			    } else {
				eor = current; status.eor = QChar::DirL; break;
			    }
			}
		    } else {
			eor = current; status.eor = QChar::DirL;
		    }
		default:
		    break;
		}
	    status.lastStrong = QChar::DirL;
	    break;
	case QChar::DirAL:
	case QChar::DirR:
	    if(dir == QChar::DirON) dir = QChar::DirR;
	    switch(status.last)
		{
		case QChar::DirR:
		case QChar::DirAL:
		    eor = current; status.eor = QChar::DirR; break;
		case QChar::DirL:
		case QChar::DirEN:
		case QChar::DirAN:
		    runs.append( new QTextBidiRun(sor, eor, context, dir) );
		    ++eor; sor = eor; dir = QChar::DirON; status.eor = QChar::DirON;
		    break;
		case QChar::DirES:
		case QChar::DirET:
		case QChar::DirCS:
		case QChar::DirBN:
		case QChar::DirB:
		case QChar::DirS:
		case QChar::DirWS:
		case QChar::DirON:
		    if( status.eor != QChar::DirR && status.eor != QChar::DirAL ) {
			//last stuff takes embedding dir
			if(context->dir == QChar::DirR || status.lastStrong == QChar::DirR) {
			    runs.append( new QTextBidiRun(sor, eor, context, dir) );
			    ++eor; sor = eor; dir = QChar::DirON; status.eor = QChar::DirON;
			    dir = QChar::DirR;
			    eor = current;
			} else {
			    eor = current - 1;
			    runs.append( new QTextBidiRun(sor, eor, context, dir) );
			    ++eor; sor = eor; dir = QChar::DirON; status.eor = QChar::DirON;
			    dir = QChar::DirR;
			}
		    } else {
			eor = current; status.eor = QChar::DirR;
		    }
		default:
		    break;
		}
	    status.lastStrong = dirCurrent;
	    break;

	    // weak types:

	case QChar::DirNSM:
	    // ### if @sor, set dir to dirSor
	    break;
	case QChar::DirEN:
	    if(status.lastStrong != QChar::DirAL) {
		// if last strong was AL change EN to AL
		if(dir == QChar::DirON) {
		    if(status.lastStrong == QChar::DirL)
			dir = QChar::DirL;
		    else
			dir = QChar::DirAN;
		}
		switch(status.last)
		    {
		    case QChar::DirEN:
		    case QChar::DirL:
		    case QChar::DirET:
			eor = current;
			status.eor = dirCurrent;
			break;
		    case QChar::DirR:
		    case QChar::DirAL:
		    case QChar::DirAN:
			runs.append( new QTextBidiRun(sor, eor, context, dir) );
			++eor; sor = eor; dir = QChar::DirON; status.eor = QChar::DirON;
			dir = QChar::DirAN; break;
		    case QChar::DirES:
		    case QChar::DirCS:
			if(status.eor == QChar::DirEN) {
			    eor = current; status.eor = QChar::DirEN; break;
			}
		    case QChar::DirBN:
		    case QChar::DirB:
		    case QChar::DirS:
		    case QChar::DirWS:
		    case QChar::DirON:		
			if(status.eor == QChar::DirR) {
			    // neutrals go to R
			    eor = current - 1;
			    runs.append( new QTextBidiRun(sor, eor, context, dir) );
			    ++eor; sor = eor; dir = QChar::DirON; status.eor = QChar::DirON;
			    dir = QChar::DirAN;
			}
			else if( status.eor == QChar::DirL ||
				 (status.eor == QChar::DirEN && status.lastStrong == QChar::DirL)) {
			    eor = current; status.eor = dirCurrent;
			} else {
			    // numbers on both sides, neutrals get right to left direction
			    if(dir != QChar::DirL) {
				runs.append( new QTextBidiRun(sor, eor, context, dir) );
				++eor; sor = eor; dir = QChar::DirON; status.eor = QChar::DirON;
				eor = current - 1;
				dir = QChar::DirR;
				runs.append( new QTextBidiRun(sor, eor, context, dir) );
				++eor; sor = eor; dir = QChar::DirON; status.eor = QChar::DirON;
				dir = QChar::DirAN;
			    } else {
				eor = current; status.eor = dirCurrent;
			    }
			}
		    default:
			break;
		    }
		break;
	    }
	case QChar::DirAN:
	    dirCurrent = QChar::DirAN;
	    if(dir == QChar::DirON) dir = QChar::DirAN;
	    switch(status.last)
		{
		case QChar::DirL:
		case QChar::DirAN:
		    eor = current; status.eor = QChar::DirAN; break;
		case QChar::DirR:
		case QChar::DirAL:
		case QChar::DirEN:
		    runs.append( new QTextBidiRun(sor, eor, context, dir) );
		    ++eor; sor = eor; dir = QChar::DirON; status.eor = QChar::DirON;
		    break;
		case QChar::DirCS:
		    if(status.eor == QChar::DirAN) {
			eor = current; status.eor = QChar::DirR; break;
		    }
		case QChar::DirES:
		case QChar::DirET:
		case QChar::DirBN:
		case QChar::DirB:
		case QChar::DirS:
		case QChar::DirWS:
		case QChar::DirON:		
		    if(status.eor == QChar::DirR) {
			// neutrals go to R
			eor = current - 1;
			runs.append( new QTextBidiRun(sor, eor, context, dir) );
			++eor; sor = eor; dir = QChar::DirON; status.eor = QChar::DirON;
			dir = QChar::DirAN;
		    } else if( status.eor == QChar::DirL ||
			       (status.eor == QChar::DirEN && status.lastStrong == QChar::DirL)) {
			eor = current; status.eor = dirCurrent;
		    } else {
			// numbers on both sides, neutrals get right to left direction
			if(dir != QChar::DirL) {
			    runs.append( new QTextBidiRun(sor, eor, context, dir) );
			    ++eor; sor = eor; dir = QChar::DirON; status.eor = QChar::DirON;
			    eor = current - 1;
			    dir = QChar::DirR;
			    runs.append( new QTextBidiRun(sor, eor, context, dir) );
			    ++eor; sor = eor; dir = QChar::DirON; status.eor = QChar::DirON;
			    dir = QChar::DirAN;
			} else {
			    eor = current; status.eor = dirCurrent;
			}
		    }
		default:
		    break;
		}
	    break;
	case QChar::DirES:
	case QChar::DirCS:
	    break;
	case QChar::DirET:
	    if(status.last == QChar::DirEN) {
		dirCurrent = QChar::DirEN;
		eor = current; status.eor = dirCurrent;
		break;
	    }
	    break;

	    // boundary neutrals should be ignored
	case QChar::DirBN:
	    break;
	    // neutrals
	case QChar::DirB:
	    // ### what do we do with newline and paragraph separators that come to here?
	    break;
	case QChar::DirS:
	    // ### implement rule L1
	    break;
	case QChar::DirWS:
	case QChar::DirON:
	    break;
	default:
	    break;
	}

	//cout << "     after: dir=" << //        dir << " current=" << dirCurrent << " last=" << status.last << " eor=" << status.eor << " lastStrong=" << status.lastStrong << " embedding=" << context->dir << endl;

	if(current >= text->length()) break;
	
	// set status.last as needed.
	switch(dirCurrent)
	    {
	    case QChar::DirET:
	    case QChar::DirES:
	    case QChar::DirCS:
	    case QChar::DirS:
	    case QChar::DirWS:
	    case QChar::DirON:
		switch(status.last)
		    {
		    case QChar::DirL:
		    case QChar::DirR:
		    case QChar::DirAL:
		    case QChar::DirEN:
		    case QChar::DirAN:
			status.last = dirCurrent;
			break;
		    default:
			status.last = QChar::DirON;
		    }
		break;
	    case QChar::DirNSM:
	    case QChar::DirBN:
		// ignore these
		break;
	    default:
		status.last = dirCurrent;
	    }

	++current;
    }

#ifdef BIDI_DEBUG
    cout << "reached end of line current=" << current << ", eor=" << eor << endl;
#endif
    eor = current;

    runs.append( new QTextBidiRun(sor, eor, context, dir) );

    // reorder line according to run structure...

    // first find highest and lowest levels
    uchar levelLow = 128;
    uchar levelHigh = 0;
    QTextBidiRun *r = runs.first();
    while ( r ) {
	//printf("level = %d\n", r->level);
	if ( r->level > levelHigh )
	    levelHigh = r->level;
	if ( r->level < levelLow )
	    levelLow = r->level;
	r = runs.next();
    }

    // implements reordering of the line (L2 according to BiDi spec):
    // L2. From the highest level found in the text to the lowest odd level on each line,
    // reverse any contiguous sequence of characters that are at that level or higher.

    // reversing is only done up to the lowest odd level
    if(!levelLow%2) levelLow++;

#ifdef BIDI_DEBUG
    cout << "reorderLine: lineLow = " << (uint)levelLow << ", lineHigh = " << (uint)levelHigh << endl;
    cout << "logical order is:" << endl;
    QListIterator<QTextBidiRun> it2(runs);
    QTextBidiRun *r2;
    for ( ; (r2 = it2.current()); ++it2 )
	cout << "    " << r2 << "  start=" << r2->start << "  stop=" << r2->stop << "  level=" << (uint)r2->level << endl;
#endif

    int count = runs.count() - 1;

    while(levelHigh >= levelLow)
    {
	int i = 0;
	while ( i < count )
	{
	    while(i < count && runs.at(i)->level < levelHigh) i++;
	    int start = i;
	    while(i <= count && runs.at(i)->level >= levelHigh) i++;
	    int end = i-1;

	    if(start != end)
	    {
		//cout << "reversing from " << start << " to " << end << endl;
		for(int j = 0; j < (end-start+1)/2; j++)
		{
		    QTextBidiRun *first = runs.take(start+j);
		    QTextBidiRun *last = runs.take(end-j-1);
		    runs.insert(start+j, last);
		    runs.insert(end-j, first);
		}
	    }
	    i++;
	    if(i >= count) break;
	}
	levelHigh--;
    }

#ifdef BIDI_DEBUG
    cout << "visual order is:" << endl;
    QListIterator<QTextBidiRun> it3(runs);
    QTextBidiRun *r3;
    for ( ; (r3 = it3.current()); ++it3 )
    {
	cout << "    " << r3 << endl;
    }
#endif

    // now construct the reordered string out of the runs...

    int x = 4;
    int numSpaces = 0;
    // set the correct alignment. This is a bit messy....
    if( align == Qt::AlignAuto ) {
	// align according to directionality of the paragraph...
	if ( text->isRightToLeft() )
	    align = Qt::AlignRight;
    }

    if ( align & Qt::AlignHCenter )
	x += space/2;
    else if ( align & Qt::AlignRight )
	x += space;
    else if ( align & Qt::AlignJustify ) {
	for ( int j = start; j < last; ++j ) {
	    if( isBreakable( text, j ) ) {
		numSpaces++;
	    }
	}
    }
    int toAdd = 0;

    // in rtl text the leftmost character is usually a space
    // this space should not take up visible space on the left side, to get alignment right.
    // the following bool is used for that purpose
    bool first = TRUE;
    r = runs.first();
    while ( r ) {
	if(r->level %2) {
	    // odd level, need to reverse the string
	    int pos = r->stop;
	    while(pos >= r->start) {
		QTextString::Char *c = &text->at(pos);
		if( numSpaces && !first && isBreakable( text, pos ) ) {
		    int s = space / numSpaces;
		    toAdd += s;
		    space -= s;
		    numSpaces--;
		}
		if ( first ) {
		    first = FALSE;
		    if ( c->c == ' ' )
			x -= c->width();
		}
		c->x = x + toAdd;
		c->rightToLeft = TRUE;
		int ww = 0;
		if ( c->c.unicode() >= 32 || c->c == '\t' || c->isCustom ) {
		    ww = c->width();
		} else {
		    ww = c->format()->width( ' ' );
		}
		//qDebug("setting char %d at pos %d width=%d", pos, x, ww);
		x += ww;
		pos--;
	    }
	} else {
	    int pos = r->start;
	    while(pos <= r->stop) {
		QTextString::Char* c = &text->at(pos);
		if( numSpaces && !first && isBreakable( text, pos ) ) {
		    int s = space / numSpaces;
		    toAdd += s;
		    space -= s;
		    numSpaces--;
		}
		if ( first ) {
		    first = FALSE;
		    if ( c->c == ' ' )
			x -= c->width();
		}
		c->x = x + toAdd;
		int ww = 0;
		if ( c->c.unicode() >= 32 || c->c == '\t' || c->isCustom ) {
		    ww = c->width();
		} else {
		    ww = c->format()->width( ' ' );
		}
		//qDebug("setting char %d at pos %d", pos, x);
		x += ww;
		pos++;
	    }
	}
	r = runs.next();
    }
    QTextParag::LineStart *ls = new QTextParag::LineStart( context, status );
    context->deref();
    return ls;
}

bool QTextFormatter::isBreakable( QTextString *string, int pos ) const
{
    // ### add line breaking rules for Kanji, thai and other languages
    return string->at( pos ).c.isSpace();
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

QTextFormatterBreakInWords::QTextFormatterBreakInWords()
{
}

int QTextFormatterBreakInWords::format( QTextDocument *doc,QTextParag *parag,
					int start, const QMap<int, QTextParag::LineStart*> & )
{
    QTextString::Char *c = 0;
    QTextString::Char *firstChar = 0;
    int left = parag->leftMargin() + 4;
    int x = left;
    int dw = doc->visibleWidth() - 8;
    int y = 0;
    int h = 0;
    int len = parag->length();
    x = parag->document()->flow()->adjustLMargin( y + parag->rect().y(), left, 4 );
    int rm = parag->rightMargin();
    int w = dw - parag->document()->flow()->adjustRMargin( y + parag->rect().y(), rm, 4 );
    bool fullWidth = TRUE;

    start = 0;
    if ( start == 0 )
	c = &parag->string()->at( 0 );

    int i = start;
    QTextParag::LineStart *lineStart = new QTextParag::LineStart( 0, 0, 0 );
    parag->lineStartList().insert( 0, lineStart );

    for ( ; i < len; ++i ) {
	c = &parag->string()->at( i );
	if ( i > 0 ) {
	    c->lineStart = 0;
	} else {
	    c->lineStart = 1;
	    firstChar = c;
	}
	int ww = 0;
	if ( c->c.unicode() >= 32 || c->c == '\t' || c->isCustom ) {
	    ww = c->width();
	} else {
	    ww = c->format()->width( ' ' );
	}
	
	if ( c->isCustom && c->customItem()->ownLine() ) {
	    x = parag->document()->flow()->adjustLMargin( y + parag->rect().y(), left, 4 );
	    w = dw - parag->document()->flow()->adjustRMargin( y + parag->rect().y(), rm, 4 );
	    c->customItem()->resize( 0, dw );
	    if ( x != left || w != dw )
		fullWidth = FALSE;
	    w = dw;
	    y += h;
	    h = c->height();
	    lineStart = new QTextParag::LineStart( y, h, h );
	    parag->lineStartList().insert( i, lineStart );
	    c->lineStart = 1;
	    firstChar = c;
	    x = 0xffffff;
	    continue;
	}

	if ( x + ww > w ) {
	    x = parag->document()->flow()->adjustLMargin( y + parag->rect().y(), left, 4 );
	    if ( x != left )
		fullWidth = FALSE;
	    w = dw;
	    y += h;
	    h = c->height();
	    lineStart = formatLine( parag->string(), lineStart, firstChar, c-1 );
	    lineStart->y = y;
	    parag->lineStartList().insert( i, lineStart );
	    lineStart->baseLine = c->ascent();
	    lineStart->h = c->height();
	    c->lineStart = 1;
	    firstChar = c;
	} else if ( lineStart ) {
	    lineStart->baseLine = QMAX( lineStart->baseLine, c->ascent() );
	    h = QMAX( h, c->height() );
	    lineStart->h = h;
	}
	
	c->x = x;
	x += ww;
    }

    int m = parag->bottomMargin();
    if ( parag->next() )
	m = QMAX( m, parag->next()->topMargin() );
    parag->setFullWidth( fullWidth );
    y += h + m;
    return y;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

QTextFormatterBreakWords::QTextFormatterBreakWords()
{
}

int QTextFormatterBreakWords::format( QTextDocument *doc, QTextParag *parag,
				      int start, const QMap<int, QTextParag::LineStart*> & )
{
    QTextString::Char *c = 0;
    QTextString::Char *firstChar = 0;
    QTextString *string = parag->string();
    int left = parag->leftMargin() + 4;
    int x = left;
    int curLeft = left;
    int dw = doc->visibleWidth() - 8;
    int y = 0;
    int h = 0;
    int len = parag->length();
    x = parag->document()->flow()->adjustLMargin( y + parag->rect().y(), left, 4 );
    curLeft = x;
    int rm = parag->rightMargin();
    int w = dw - parag->document()->flow()->adjustRMargin( y + parag->rect().y(), rm, 4 );
    bool fullWidth = TRUE;
    int minw = 0;

    start = 0;
    if ( start == 0 )
	c = &parag->string()->at( 0 );

    int i = start;
    QTextParag::LineStart *lineStart = new QTextParag::LineStart( 0, 0, 0 );
    parag->lineStartList().insert( 0, lineStart );
    int lastBreak = -1;
    int tmpBaseLine = 0, tmph = 0;
    bool lastWasNonInlineCustom = FALSE;

    int align = parag->alignment();
    if ( align == Qt::AlignAuto && doc->alignment() != Qt::AlignAuto )
	align = doc->alignment();

    for ( ; i < len; ++i ) {
	c = &string->at( i );
	if ( i > 0 && x > curLeft || lastWasNonInlineCustom ) {
	    c->lineStart = 0;
	} else {
	    c->lineStart = 1;
	    firstChar = c;
	}

	if ( c->isCustom && c->customItem()->placement() != QTextCustomItem::PlaceInline )
	    lastWasNonInlineCustom = TRUE;
	else
	    lastWasNonInlineCustom = FALSE;
	
	int ww = 0;
	if ( c->c.unicode() >= 32 || c->c == '\t' || c->isCustom ) {
	    ww = c->width();
	} else {
	    ww = c->format()->width( ' ' );
	}
	
	if ( c->isCustom && c->customItem()->ownLine() ) {
	    x = parag->document()->flow()->adjustLMargin( y + parag->rect().y(), left, 4 );
	    w = dw - parag->document()->flow()->adjustRMargin( y + parag->rect().y(), rm, 4 );
	    lineStart = formatLine( string, lineStart, firstChar, c-1, align, w - x );
	    c->customItem()->resize( 0, dw );
	    if ( x != left || w != dw )
		fullWidth = FALSE;
	    curLeft = x;
	    y += h;
	    tmph = c->height();
	    h = tmph;
	    lineStart->y = y;
	    lineStart->h = h;
	    lineStart->baseLine = h;
	    parag->lineStartList().insert( i, lineStart );
	    c->lineStart = 1;
	    firstChar = c;
	    tmpBaseLine = lineStart->baseLine;
	    lastBreak = -1;
	    x = 0xffffff;
	    minw = QMAX( minw, QMIN( c->customItem()->widthHint(), c->customItem()->width ) );
	    continue;
	}
	minw = QMAX( ww, minw );
	
	if ( x + ww > w ) {
	    if ( lastBreak == -1 ) {
		if ( lineStart ) {
		    lineStart->baseLine = QMAX( lineStart->baseLine, tmpBaseLine );
		    h = QMAX( h, tmph );
		    lineStart->h = h;
		}
		lineStart = formatLine( string, lineStart, firstChar, c-1, align, w - x );
		
		x = parag->document()->flow()->adjustLMargin( y + parag->rect().y(), left, 4 );
		w = dw - parag->document()->flow()->adjustRMargin( y + parag->rect().y(), rm, 4 );
		if ( x != left || w != dw )
		    fullWidth = FALSE;
		curLeft = x;
		y += h;
		tmph = c->height();
		h = 0;
		lineStart->y = y;
		parag->lineStartList().insert( i, lineStart );
		lineStart->baseLine = c->ascent();
		lineStart->h = c->height();
		c->lineStart = 1;
		firstChar = c;
		tmpBaseLine = lineStart->baseLine;
		lastBreak = -1;
	    } else {
		i = lastBreak;
		lineStart = formatLine( string, lineStart, firstChar, parag->at( lastBreak ), align, w - string->at( i ).x );
		x = parag->document()->flow()->adjustLMargin( y + parag->rect().y(), left, 4 );
		w = dw - parag->document()->flow()->adjustRMargin( y + parag->rect().y(), rm, 4 );
		if ( x != left || w != dw )
		    fullWidth = FALSE;
		curLeft = x;
		y += h;
		tmph = c->height();
		h = tmph;
		lineStart->y = y;
		parag->lineStartList().insert( i + 1, lineStart );
		lineStart->baseLine = c->ascent();
		lineStart->h = c->height();
		c->lineStart = 1;
		firstChar = c;
		tmpBaseLine = lineStart->baseLine;
		lastBreak = -1;
		continue;
	    }
	} else if ( lineStart && isBreakable( string, i ) ) {
	    if ( len < 2 || i < len - 1 ) {
		tmpBaseLine = QMAX( tmpBaseLine, c->ascent() );
		tmph = QMAX( tmph, c->height() );
	    }
	    lineStart->baseLine = QMAX( lineStart->baseLine, tmpBaseLine );
	    h = QMAX( h, tmph );
	    lineStart->h = h;
	    lastBreak = i;
	} else {
	    tmpBaseLine = QMAX( tmpBaseLine, c->ascent() );
	    tmph = QMAX( tmph, c->height() );
	}
	
	c->x = x;
	x += ww;
    }

    if ( lineStart ) {
	lineStart->baseLine = QMAX( lineStart->baseLine, tmpBaseLine );
	h = QMAX( h, tmph );
	lineStart->h = h;
	// last line in a paragraph is not justified
	if ( align == Qt::AlignJustify )
	    align = Qt::AlignAuto;
	lineStart = formatLine( string, lineStart, firstChar, c, align, w - x );
	delete lineStart;
    }

    int m = parag->bottomMargin();
    if ( parag->next() )
	m = QMAX( m, parag->next()->topMargin() );
    parag->setFullWidth( fullWidth );
    y += h + m;

    doc->setMinimumWidth( minw, parag );

    return y;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

QTextIndent::QTextIndent()
{
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

QTextFormatCollection::QTextFormatCollection()
{
    defFormat = new QTextFormat( QApplication::font(),
				     QApplication::palette().color( QPalette::Active, QColorGroup::Text ) );
    lastFormat = cres = 0;
    cflags = -1;
    cKey.setAutoDelete( TRUE );
    cachedFormat = 0;
}

QTextFormat *QTextFormatCollection::format( QTextFormat *f )
{
    if ( f->parent() == this ) {
#ifdef DEBUG_COLLECTION
	qDebug( "need '%s', best case!", f->key().latin1() );
#endif
	lastFormat = f;
	lastFormat->addRef();
	return lastFormat;
    }

    if ( f == lastFormat || ( lastFormat && f->key() == lastFormat->key() ) ) {
#ifdef DEBUG_COLLECTION
	qDebug( "need '%s', good case!", f->key().latin1() );
#endif
	lastFormat->addRef();
	return lastFormat;
    }

    if ( f->isAnchor() ) {
	lastFormat = new QTextFormat( *f );
	lastFormat->collection = 0;
	return lastFormat;
    }

    QTextFormat *fm = cKey.find( f->key() );
    if ( fm ) {
#ifdef DEBUG_COLLECTION
	qDebug( "need '%s', normal case!", f->key().latin1() );
#endif
	lastFormat = fm;
	lastFormat->addRef();
	return lastFormat;
    }

#ifdef DEBUG_COLLECTION
    qDebug( "need '%s', worst case!", f->key().latin1() );
#endif
    lastFormat = new QTextFormat( *f );
    lastFormat->collection = this;
    cKey.insert( lastFormat->key(), lastFormat );
    return lastFormat;
}

QTextFormat *QTextFormatCollection::format( QTextFormat *of, QTextFormat *nf, int flags )
{
    if ( cres && kof == of->key() && knf == nf->key() && cflags == flags ) {
#ifdef DEBUG_COLLECTION
	qDebug( "mix of '%s' and '%s, best case!", of->key().latin1(), nf->key().latin1() );
#endif
	cres->addRef();
	return cres;
    }

    cres = new QTextFormat( *of );
    kof = of->key();
    knf = nf->key();
    cflags = flags;
    if ( flags & QTextFormat::Bold )
	cres->fn.setBold( nf->fn.bold() );
    if ( flags & QTextFormat::Italic )
	cres->fn.setItalic( nf->fn.italic() );
    if ( flags & QTextFormat::Underline )
	cres->fn.setUnderline( nf->fn.underline() );
    if ( flags & QTextFormat::Family )
	cres->fn.setFamily( nf->fn.family() );
    if ( flags & QTextFormat::Size )
	cres->fn.setPointSize( nf->fn.pointSize() );
    if ( flags & QTextFormat::Color )
	cres->col = nf->col;
    if ( flags & QTextFormat::Misspelled )
	cres->missp = nf->missp;
    cres->update();

    QTextFormat *fm = cKey.find( cres->key() );
    if ( !fm ) {
#ifdef DEBUG_COLLECTION
	qDebug( "mix of '%s' and '%s, worst case!", of->key().latin1(), nf->key().latin1() );
#endif
	cres->collection = this;
	cKey.insert( cres->key(), cres );
    } else {
#ifdef DEBUG_COLLECTION
	qDebug( "mix of '%s' and '%s, good case!", of->key().latin1(), nf->key().latin1() );
#endif
	delete cres;
	cres = fm;
	cres->addRef();
    }
					
    return cres;
}

QTextFormat *QTextFormatCollection::format( const QFont &f, const QColor &c )
{
    if ( cachedFormat && cfont == f && ccol == c ) {
#ifdef DEBUG_COLLECTION
	qDebug( "format of font and col '%s' - best case", cachedFormat->key().latin1() );
#endif
	cachedFormat->addRef();
	return cachedFormat;
    }

    QString key = QTextFormat::getKey( f, c, FALSE, QString::null, QString::null );
    cachedFormat = cKey.find( key );
    cfont = f;
    ccol = c;

    if ( cachedFormat ) {
#ifdef DEBUG_COLLECTION
	qDebug( "format of font and col '%s' - good case", cachedFormat->key().latin1() );
#endif
	cachedFormat->addRef();
	return cachedFormat;
    }

    cachedFormat = new QTextFormat( f, c );
    cachedFormat->collection = this;
    cKey.insert( cachedFormat->key(), cachedFormat );
#ifdef DEBUG_COLLECTION
    qDebug( "format of font and col '%s' - worst case", cachedFormat->key().latin1() );
#endif
    return cachedFormat;
}

void QTextFormatCollection::remove( QTextFormat *f )
{
    if ( lastFormat == f )
	lastFormat = 0;
    if ( cres == f )
	cres = 0;
    if ( cachedFormat == f )
	cachedFormat = 0;
    cKey.remove( f->key() );
}

void QTextFormatCollection::setPainter( QPainter *p )
{
    QDictIterator<QTextFormat> it( cKey );
    QTextFormat *f;
    while ( ( f = it.current() ) ) {
	++it;
	f->setPainter( p );
    }
}

void QTextFormatCollection::debug()
{
#ifdef DEBUG_COLLECTION
    qDebug( "------------ QTextFormatCollection: debug --------------- BEGIN" );
    QDictIterator<QTextFormat> it( cKey );
    for ( ; it.current(); ++it ) {
	qDebug( "format '%s' (%p): refcount: %d", it.current()->key().latin1(),
		it.current(), it.current()->ref );
    }
    qDebug( "------------ QTextFormatCollection: debug --------------- END" );
#endif
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void QTextFormat::setBold( bool b )
{
    if ( b == fn.bold() )
	return;
    fn.setBold( b );
    update();
}

void QTextFormat::setMisspelled( bool b )
{
    if ( b == missp )
	return;
    missp = b;
    update();
}

void QTextFormat::setItalic( bool b )
{
    if ( b == fn.italic() )
	return;
    fn.setItalic( b );
    update();
}

void QTextFormat::setUnderline( bool b )
{
    if ( b == fn.underline() )
	return;
    fn.setUnderline( b );
    update();
}

void QTextFormat::setFamily( const QString &f )
{
    if ( f == fn.family() )
	return;
    fn.setFamily( f );
    update();
}

void QTextFormat::setPointSize( int s )
{
    if ( s == fn.pointSize() )
	return;
    fn.setPointSize( s );
    update();
}

void QTextFormat::setFont( const QFont &f )
{
    if ( f == fn )
	return;
    fn = f;
    update();
}

void QTextFormat::setColor( const QColor &c )
{
    if ( c == col )
	return;
    col = c;
    update();
}

void QTextFormat::setPainter( QPainter *p )
{
    painter = p;
}

static int makeLogicFontSize( int s )
{
    int defSize = QApplication::font().pointSize();
    if ( s < defSize - 4 )
	return 1;
    if ( s < defSize )
	return 2;
    if ( s < defSize + 4 )
	return 3;
    if ( s < defSize + 8 )
	return 4;
    if ( s < defSize + 12 )
	return 5;
    if (s < defSize + 16 )
	return 6;
    return 7;
}

static QTextFormat *defaultFormat = 0;

QString QTextFormat::makeFormatChangeTags( QTextFormat *f ) const
{
    if ( !defaultFormat )
	defaultFormat = new QTextFormat( QApplication::font(),
					     QApplication::palette().color( QPalette::Active, QColorGroup::Text ) );

    QString tag;
    if ( f ) {
	if ( f->font() != defaultFormat->font() ||
	     f->color().rgb() != defaultFormat->color().rgb() )
	    tag += "</font>";
	if ( f->font() != defaultFormat->font() ) {
	    if ( f->font().underline() && f->font().underline() != defaultFormat->font().underline() )
		tag += "</u>";
	    if ( f->font().italic() && f->font().italic() != defaultFormat->font().italic() )
		tag += "</i>";
	    if ( f->font().bold() && f->font().bold() != defaultFormat->font().bold() )
		tag += "</b>";
	}
    }

    if ( font() != defaultFormat->font() ) {
	if ( font().bold() && font().bold() != defaultFormat->font().bold() )
	    tag += "<b>";
	if ( font().italic() && font().italic() != defaultFormat->font().italic() )
	    tag += "<i>";
	if ( font().underline() && font().underline() != defaultFormat->font().underline() )
	    tag += "<u>";
    }
    if ( font() != defaultFormat->font() ||
	 color().rgb() != defaultFormat->color().rgb() ) {
	tag += "<font ";
	if ( font().family() != defaultFormat->font().family() )
	    tag +="face=\"" + fn.family() + "\" ";
	if ( font().pointSize() != defaultFormat->font().pointSize() )
	    tag +="size=\"" + QString::number( makeLogicFontSize( fn.pointSize() ) ) + "\" ";
	if ( color().rgb() != defaultFormat->color().rgb() )
	    tag +="color=\"" + col.name() + "\" ";
	tag += ">";
    }

    return tag;
}

QString QTextFormat::makeFormatEndTags() const
{
    if ( !defaultFormat )
	defaultFormat = new QTextFormat( QApplication::font(),
					     QApplication::palette().color( QPalette::Active, QColorGroup::Text ) );

    QString tag;
    if ( font() != defaultFormat->font() ||
	 color().rgb() != defaultFormat->color().rgb() )
	tag += "</font>";
    if ( font() != defaultFormat->font() ) {
	if ( font().underline() && font().underline() != defaultFormat->font().underline() )
	    tag += "</u>";
	if ( font().italic() && font().italic() != defaultFormat->font().italic() )
	    tag += "</i>";
	if ( font().bold() && font().bold() != defaultFormat->font().bold() )
	    tag += "</b>";
    }
    return tag;
}

QTextFormat QTextFormat::makeTextFormat( const QStyleSheetItem *style, const QMap<QString,QString>& attr ) const
{
    QTextFormat format(*this);
    bool changed = FALSE;
    if ( style ) {
	if ( style->name() == "font") {
	    if ( attr.contains("color") )
		format.col.setNamedColor( attr["color"] );
	    if ( attr.contains("size") ) {
		QString a = attr["size"];
		int n = a.toInt();
		if ( a[0] == '+' || a[0] == '-' )
		    n += format.logicalFontSize;
		format.logicalFontSize = n;
		format.fn.setPointSize( format.stdPointSize );
		style->styleSheet()->scaleFont( format.fn, format.logicalFontSize );
	    }
	    if ( attr.contains("face") ) {
		QString a = attr["face"];
		if ( a.contains(',') )
		    a = a.left( a.find(',') );
		format.fn.setFamily( a );
	    }
	} else {

	    if ( style->isAnchor() ) {
		format.anchor_href = attr["href"];
		format.anchor_name = attr["name"];
		changed = TRUE;
	    }

	    if ( style->fontWeight() != QStyleSheetItem::Undefined )
		format.fn.setWeight( style->fontWeight() );
	    if ( style->fontSize() != QStyleSheetItem::Undefined )
		format.fn.setPointSize( style->fontSize() );
	    else if ( style->logicalFontSize() != QStyleSheetItem::Undefined ) {
		format.logicalFontSize = style->logicalFontSize();
		format.fn.setPointSize( format.stdPointSize );
		style->styleSheet()->scaleFont( format.fn, format.logicalFontSize );
	    }
	    else if ( style->logicalFontSizeStep() ) {
		format.logicalFontSize += style->logicalFontSizeStep();
		format.fn.setPointSize( format.stdPointSize );
		style->styleSheet()->scaleFont( format.fn, format.logicalFontSize );
	    }
	    if ( !style->fontFamily().isEmpty() )
		format.fn.setFamily( style->fontFamily() );
	    if ( style->color().isValid() )
		format.col = style->color();
	    if ( style->definesFontItalic() )
		format.fn.setItalic( style->fontItalic() );
	    if ( style->definesFontUnderline() )
		format.fn.setUnderline( style->fontUnderline() );
	}
    }

    if ( fn != format.fn || changed || col != format.col ) // slight performance improvement
	format.generateKey();
    format.update();
    return format;
}

QTextImage::QTextImage( QTextDocument *p, const QMap<QString, QString> &attr, const QString& context,
			const QMimeSourceFactory &factory )
    : QTextCustomItem( p )
{
#if defined(PARSER_DEBUG)
    qDebug( debug_indent + "new QTextImage (pappi: %p)", p );
#endif
    width = height = 0;
    if ( attr.contains("width") )
	width = attr["width"].toInt();
    if ( attr.contains("height") )
	height = attr["height"].toInt();

    reg = 0;
    QImage img;
    QString imageName = attr["src"];

    if (!imageName)
	imageName = attr["source"];

#if defined(PARSER_DEBUG)
    qDebug( debug_indent + "    .." + imageName );
#endif

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

    if ( pm.isNull() && (width*height)==0 )
	width = height = 50;

    place = PlaceInline;
    if ( attr["align"] == "left" )
	place = PlaceLeft;
    else if ( attr["align"] == "right" )
	place = PlaceRight;

    tmpwidth = width;
    tmpheight = height;
}

QTextImage::~QTextImage()
{
}


void QTextImage::adjustToPainter( QPainter* p )
{
    width = tmpwidth;
    height = tmpheight;
    if ( !p || p->device()->devType() != QInternal::Printer )
	return;
    QPaintDeviceMetrics metrics(p->device());
    width = int( width * scale_factor( metrics.logicalDpiX() ) );
    height = int( height * scale_factor( metrics.logicalDpiY() ) );
}

void QTextImage::draw( QPainter* p, int x, int y, int cx, int cy, int cw, int ch, const QColorGroup& cg )
{
    if ( placement() != PlaceInline ) {
	x = xpos;
	y = ypos;
    }
	
    if ( pm.isNull() ) {
	p->fillRect( x , y, width, height,  cg.dark() );
	return;
    }

    if ( p->device()->devType() == QInternal::Printer ) {
#ifndef QT_NO_TRANSFORMATIONS
	p->saveWorldMatrix();
	QPaintDeviceMetrics metrics( p->device() );
	p->translate( x, y );
	p->scale( scale_factor( metrics.logicalDpiY() ),
		  scale_factor( metrics.logicalDpiY() ) );
	p->drawPixmap( 0, 0, pm );
	p->restoreWorldMatrix();
#else
	p->drawPixmap( x, y, pm );
#endif
	return;
    }

    if ( placement() != PlaceInline && !QRect( xpos, ypos, width, height ).intersects( QRect( cx, cy, cw, ch ) ) )
	return;
    if ( placement() == PlaceInline )
	p->drawPixmap( x , y, pm );
    else
	p->drawPixmap( cx , cy, pm, cx - x, cy - y, cw, ch );
}

void QTextHorizontalLine::realize( QPainter* p )
{
    if ( !p || p->device()->devType() != QInternal::Printer )
	return;
    QPaintDeviceMetrics metrics(p->device());
    height = int( height * scale_factor( metrics.logicalDpiY() ) );
}


QTextHorizontalLine::QTextHorizontalLine( QTextDocument *p )
    : QTextCustomItem( p )
{
    height = 8;
}


QTextHorizontalLine::~QTextHorizontalLine()
{
}


void QTextHorizontalLine::draw( QPainter* p, int x, int y, int , int , int , int , const QColorGroup& cg )
{
    QRect r( x, y, width, height);
    if ( p->device()->devType() == QInternal::Printer ) {
	QPen oldPen = p->pen();
	p->setPen( QPen( cg.text(), height/8 ) );
	p->drawLine( r.left()-1, y + height / 2, r.right() + 1, y + height / 2 );
	p->setPen( oldPen );
    } else {
	qDrawShadeLine( p, r.left() - 1, y + height / 2, r.right() + 1, y + height / 2, cg, TRUE, height / 8 );
    }
}


/*****************************************************************/
// Small set of utility functions to make the parser a bit simpler
//

bool QTextDocument::hasPrefix(const QString& doc, int pos, QChar c)
{
    if ( pos >= (int)doc.length() )
	return FALSE;
    return (doc.unicode())[pos] ==c;
}

bool QTextDocument::hasPrefix(const QString& doc, int pos, const QString& s)
{
    if ( pos + s.length() >= doc.length() )
	return FALSE;
    for (int i = 0; i < int(s.length()); i++) {
	if ((doc.unicode())[pos+i] != s[i])
	    return FALSE;
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

QTextCustomItem* QTextDocument::parseTable( const QMap<QString, QString> &attr, const QTextFormat &fmt, const QString &doc, int& pos )
{

    QTextTable* table = new QTextTable( this, attr );
    int row = -1;
    int col = -1;

    QString rowbgcolor;
    QString rowalign;
    QString tablebgcolor = attr["bgcolor"];

    QList<QTextTableCell> multicells;

    QString tagname;
    (void) eatSpace(doc, pos);
    while ( pos < int(doc.length() )) {
	int beforePos = pos;
	if (hasPrefix(doc, pos, QChar('<')) ){
	    if (hasPrefix(doc, pos+1, QChar('/'))) {
		tagname = parseCloseTag( doc, pos );
		if ( tagname == "table" ) {
		    pos = beforePos;
#if defined(PARSER_DEBUG)
		    debug_indent.remove( debug_indent.length() - 3, 2 );
#endif
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
			const QStyleSheetItem* s = sheet_->item(tagname);
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
			
			// extract the cell contents
			int end = pos;
			while ( end < (int) doc.length()
				&& !hasPrefix( doc, end, "</td")
				&& !hasPrefix( doc, end, "<td")
				&& !hasPrefix( doc, end, "</th")
				&& !hasPrefix( doc, end, "<th")
				&& !hasPrefix( doc, end, "<td")
				&& !hasPrefix( doc, end, "</tr")
				&& !hasPrefix( doc, end, "<tr")
				&& !hasPrefix( doc, end, "</table") ) {
			    if ( hasPrefix( doc, end, "<table" ) ) { // nested table
				while ( end < (int)doc.length() &&
					!hasPrefix( doc, end, "</table" ) )
				    end++;
			    }
			    end++;
			}
			QTextTableCell* cell  = new QTextTableCell( table, row, col,
			      attr2, s, fmt.makeTextFormat( s, attr2 ),
			      contxt, *factory_, sheet_, doc.mid( pos, end - pos ) );
			if ( cell->colspan() > 1 || cell->rowspan() > 1 )
			    multicells.append( cell );
			col += cell->colspan()-1;
			pos = end;
		    }
		}
	    }

	} else {
	    ++pos;
	}
    }
#if defined(PARSER_DEBUG)
    debug_indent.remove( debug_indent.length() - 3, 2 );
#endif
    return table;
}

bool QTextDocument::eatSpace(const QString& doc, int& pos, bool includeNbsp )
{
    int old_pos = pos;
    while (pos < int(doc.length()) && (doc.unicode())[pos].isSpace() && ( includeNbsp || (doc.unicode())[pos] != QChar::nbsp ) )
	pos++;
    return old_pos < pos;
}

bool QTextDocument::eat(const QString& doc, int& pos, QChar c)
{
    bool ok = pos < int(doc.length()) && ((doc.unicode())[pos] == c);
    if ( ok )
	pos++;
    return ok;
}
/*****************************************************************/



static QMap<QCString, QChar> *html_map = 0;
static void qt_cleanup_html_map()
{
    delete html_map;
    html_map = 0;
}

static QMap<QCString, QChar> *htmlMap()
{
    if ( !html_map ) {
	html_map = new QMap<QCString, QChar>;
	qAddPostRoutine( qt_cleanup_html_map );
  	html_map->insert( "lt", '<');
  	html_map->insert( "gt", '>');
  	html_map->insert( "amp", '&');
  	html_map->insert( "nbsp", 0x00a0U);
  	html_map->insert( "bull", 0x2022U);
  	html_map->insert( "aring", '\xe5');
  	html_map->insert( "oslash", '\xf8');
  	html_map->insert( "ouml", '\xf6');
  	html_map->insert( "auml", '\xe4');
  	html_map->insert( "uuml", '\xfc');
  	html_map->insert( "Ouml", '\xd6');
  	html_map->insert( "Auml", '\xc4');
  	html_map->insert( "Uuml", '\xdc');
  	html_map->insert( "szlig", '\xdf');
  	html_map->insert( "copy", '\xa9');
  	html_map->insert( "deg", '\xb0');
  	html_map->insert( "micro", '\xb5');
  	html_map->insert( "plusmn", '\xb1');
  	html_map->insert( "middot", '*');
  	html_map->insert( "quot", '\"');
  	html_map->insert( "commat", '@');
  	html_map->insert( "num", '#');
  	html_map->insert( "dollar", '$');
  	html_map->insert( "ldquo", '`');
  	html_map->insert( "rdquo", '\'');
  	html_map->insert( "sol", '/' );
  	html_map->insert( "bsol", '\\');
  	html_map->insert( "lowbar", '_');
    }
    return html_map;
}

QChar QTextDocument::parseHTMLSpecialChar(const QString& doc, int& pos)
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

QString QTextDocument::parseWord(const QString& doc, int& pos, bool lower)
{
    QString s;

    if ((doc.unicode())[pos] == '"') {
	pos++;
	while ( pos < int(doc.length()) && (doc.unicode())[pos] != '"' ) {
	    s += (doc.unicode())[pos];
	    pos++;
	}
	eat(doc, pos, '"');
    } else {
	static QString term = QString::fromLatin1("/>");
	while( pos < int(doc.length()) &&
	       ((doc.unicode())[pos] != '>' && !hasPrefix( doc, pos, term))
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
    return s;
}

QChar QTextDocument::parseChar(const QString& doc, int& pos, QStyleSheetItem::WhiteSpaceMode wsm )
{
    if ( pos >=  int(doc.length() ) )
	return QChar::null;

    QChar c = (doc.unicode())[pos++];

    if (c == '<' )
	return QChar::null;
	
    if ( c.isSpace() && c != QChar::nbsp ) {
	if ( wsm == QStyleSheetItem::WhiteSpacePre ) {
	    if ( c == ' ' )
		return QChar::nbsp;
	    else
		return c;
	} else { // non-pre mode: collapse whitespace except nbsp
	    while ( pos< int(doc.length() ) &&
		    (doc.unicode())[pos].isSpace()  && (doc.unicode())[pos] != QChar::nbsp )
		pos++;
	    if ( wsm == QStyleSheetItem::WhiteSpaceNoWrap )
		return QChar::nbsp;
	    else
		return ' ';
	}
    }
    else if ( c == '&' )
	return parseHTMLSpecialChar( doc, --pos );
    else
	return c;
}

QString QTextDocument::parseOpenTag(const QString& doc, int& pos,
				  QMap<QString, QString> &attr, bool& emptyTag)
{
    emptyTag = FALSE;
    pos++;
    if ( hasPrefix(doc, pos, '!') ) {
	if ( hasPrefix( doc, pos+1, "--")) {
	    pos += 3;
	    // eat comments
	    QString pref = QString::fromLatin1("-->");
	    while ( !hasPrefix(doc, pos, pref ) && pos < int(doc.length()) )
		pos++;
	    if ( hasPrefix(doc, pos, pref ) ) {
		pos += 3;
		eatSpace(doc, pos, TRUE);
	    }
	    emptyTag = TRUE;
	    return QString::null;
	}
	else {
	    // eat strange internal tags
	    while ( !hasPrefix(doc, pos, '>') && pos < int(doc.length()) )
		pos++;
	    if ( hasPrefix(doc, pos, '>') ) {
		pos++;
		eatSpace(doc, pos, TRUE);
	    }
	    return QString::null;
	}
    }

    QString tag = parseWord(doc, pos );
    eatSpace(doc, pos, TRUE);
    static QString term = QString::fromLatin1("/>");
    static QString s_TRUE = QString::fromLatin1("TRUE");

    while ((doc.unicode())[pos] != '>' && ! (emptyTag = hasPrefix(doc, pos, term) )) {
	QString key = parseWord(doc, pos );
	eatSpace(doc, pos, TRUE);
	if ( key.isEmpty()) {
	    // error recovery
	    while ( pos < int(doc.length()) && (doc.unicode())[pos] != '>' )
		pos++;
	    break;
	}
	QString value;
	if (hasPrefix(doc, pos, '=') ){
	    pos++;
	    eatSpace(doc, pos);
	    value = parseWord(doc, pos, FALSE);
	}
	else
	    value = s_TRUE;
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

QString QTextDocument::parseCloseTag( const QString& doc, int& pos )
{
    pos++;
    pos++;
    QString tag = parseWord(doc, pos );
    eatSpace(doc, pos, TRUE);
    eat(doc, pos, '>');
    return tag;
}

QTextFlow::QTextFlow()
{
    width = height = pagesize = 0;
    leftItems.setAutoDelete( FALSE );
    rightItems.setAutoDelete( FALSE );
}

QTextFlow::~QTextFlow()
{
}

void QTextFlow::setWidth( int w )
{
    height = 0;
    width = w;
}

int QTextFlow::adjustLMargin( int yp, int margin, int space )
{
    for ( QTextCustomItem* item = leftItems.first(); item; item = leftItems.next() ) {
	if ( item->ypos == -1 )
	    continue;
	if ( yp >= item->ypos && yp < item->ypos + item->height )
	    margin = QMAX( margin, item->xpos + item->width + space );
    }
    return margin;
}

int QTextFlow::adjustRMargin( int yp, int margin, int space )
{
    for ( QTextCustomItem* item = rightItems.first(); item; item = rightItems.next() ) {
	if ( item->ypos == -1 )
	    continue;
	if ( yp >= item->ypos && yp < item->ypos + item->height )
	    margin = QMAX( margin, width - item->xpos - space );
    }
    return margin;
}

void QTextFlow::adjustFlow( int &yp, int , int h, bool pages )
{
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

void QTextFlow::unregisterFloatingItem( QTextCustomItem* item )
{
    leftItems.removeRef( item );
    rightItems.removeRef( item );
}

void QTextFlow::registerFloatingItem( QTextCustomItem* item, bool right )
{
    if ( right ) {
	if ( !rightItems.contains( item ) )
	    rightItems.append( item );
    } else if ( !leftItems.contains( item ) ) {
	leftItems.append( item );
    }
}

void QTextFlow::drawFloatingItems( QPainter* p, int cx, int cy, int cw, int ch, const QColorGroup& cg )
{
    QTextCustomItem *item;
    for ( item = leftItems.first(); item; item = leftItems.next() ) {
	if ( item->xpos == -1 || item->ypos == -1 )
	    continue;
	item->draw( p, item->xpos, item->ypos, cx, cy, cw, ch, cg );
    }

    for ( item = rightItems.first(); item; item = rightItems.next() ) {
	if ( item->xpos == -1 || item->ypos == -1 )
	    continue;
	item->draw( p, item->xpos, item->ypos, cx, cy, cw, ch, cg );
    }
}

void QTextFlow::updateHeight( QTextCustomItem *i )
{
    height = QMAX( height, i->ypos + i->height );
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

QTextTable::QTextTable( QTextDocument *p, const QMap<QString, QString> & attr  )
    : QTextCustomItem( p ), currCell( -1 )
{
    cells.setAutoDelete( TRUE );
#if defined(PARSER_DEBUG)
    debug_indent += "\t";
    qDebug( debug_indent + "new QTextTable (%p)", this );
    debug_indent += "\t";
#endif
    cellspacing = 2;
    if ( attr.contains("cellspacing") )
	cellspacing = attr["cellspacing"].toInt();
    cellpadding = 1;
    if ( attr.contains("cellpadding") )
	cellpadding = attr["cellpadding"].toInt();
    border = 0;
    innerborder = 1;
    if ( attr.contains("border" ) ) {
	QString s( attr["border"] );
	if ( s == "TRUE" )
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

    place = PlaceInline;
    if ( attr["align"] == "left" )
	place = PlaceLeft;
    else if ( attr["align"] == "right" )
	place = PlaceRight;
    cachewidth = 0;
}

QTextTable::~QTextTable()
{
    delete layout;
}

void QTextTable::adjustToPainter( QPainter* p)
{
    painter = p;
    if ( p && p->device()->devType() != QInternal::Printer ) {
	QPaintDeviceMetrics metrics(p->device());
	double xscale = scale_factor( metrics.logicalDpiX() );
	cellspacing = int(cellspacing * xscale);
	border = int(border * xscale);
	innerborder = int(innerborder * xscale);
	outerborder = int(outerborder * xscale);
    }
    for (QTextTableCell* cell = cells.first(); cell; cell = cells.next() )
	cell->adjustToPainter();

    width = 0;
}

void QTextTable::verticalBreak( int  yt, QTextFlow* flow )
{
    if ( flow->pageSize() <= 0 )
	return;
    int shift = 0;
    for (QTextTableCell* cell = cells.first(); cell; cell = cells.next() ) {
	QRect r = cell->geometry();
 	r.moveBy(0, shift );
	cell->setGeometry( r );
	if ( cell->column() == 0 ) {
	    int y = yt + outerborder + cell->geometry().y();
	    int oldy = y;
	    flow->adjustFlow( y, width, cell->geometry().height() + 2*cellspacing );
	    shift += y - oldy;
	    r = cell->geometry();
 	    r.moveBy(0, y - oldy );
	    cell->setGeometry( r );
	}
    }
    height += shift;
}

void QTextTable::draw(QPainter* p, int x, int y, int cx, int cy, int cw, int ch, const QColorGroup& cg )
{
    if ( placement() != PlaceInline ) {
	x = xpos;
	y = ypos;
    }

    lastX = x;
    lastY = y;

    painter = p;
    for (QTextTableCell* cell = cells.first(); cell; cell = cells.next() ) {
	if ( cx < 0 && cy < 0 ||
	     QRect( cx, cy, cw, ch ).intersects( QRect( x + outerborder + cell->geometry().x(),
							y + outerborder + cell->geometry().y(),
							cell->geometry().width(), cell->geometry().height() ) ) ) {
	    cell->draw( x+outerborder, y+outerborder, cx, cy, cw, ch, cg );
	    if ( border ) {
		QRect r( x+outerborder+cell->geometry().x()-innerborder,
			 y+outerborder+cell->geometry().y()-innerborder,
			 cell->geometry().width()+2*innerborder,
			 cell->geometry().height()+2*innerborder);
		int s = cellspacing;
		if ( p->device()->devType() == QInternal::Printer ) {
		    qDrawPlainRect( p, r, cg.text(), innerborder );
		} else {
		    p->fillRect( r.left()-s, r.top(), s, r.height(), cg.button() );
		    p->fillRect( r.right(), r.top(), s, r.height(), cg.button() );
		    p->fillRect( r.left()-s, r.top()-s, r.width()+2*s, s, cg.button() );
		    p->fillRect( r.left()-s, r.bottom(), r.width()+2*s, s, cg.button() );
		    qDrawShadePanel( p, r, cg, TRUE, innerborder );
		}
	    }
 	}
    }
    if ( border ) {
	QRect r ( x, y, width, height );
	if ( p->device()->devType() == QInternal::Printer ) {
	    qDrawPlainRect( p, QRect(r.x()+1, r.y()+1, r.width()-2, r.height()-2), cg.text(), border );
	} else {
	    int s = border;
	    p->fillRect( r.left(), r.top(), s, r.height(), cg.button() );
	    p->fillRect( r.right()-s, r.top(), s, r.height(), cg.button() );
	    p->fillRect( r.left(), r.top(), r.width(), s, cg.button() );
	    p->fillRect( r.left(), r.bottom()-s, r.width(), s, cg.button() );
	    qDrawShadePanel( p, r, cg, FALSE, border );
	}
    }

#if defined(DEBUG_TABLE_RENDERING)
    p->save();
    p->setPen( Qt::red );
    p->drawRect( x, y, width, height );
    p->restore();
#endif
}

void QTextTable::resize( QPainter* p, int nwidth )
{
    if ( nwidth == cachewidth )
	return;
    int w = nwidth;
    format( w );
    if ( nwidth >= 32000 )
	nwidth = w;
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

    layout->invalidate();
    mw = layout->minimumSize().width() + 2*outerborder;
    width = QMAX( width, mw );

    int h = layout->heightForWidth( width-2*outerborder );
    layout->setGeometry( QRect(0, 0, width-2*outerborder, h)  );
    height = layout->geometry().height()+2*outerborder;
};

void QTextTable::format( int &w )
{
    for ( int i = 0; i < (int)cells.count(); ++i ) {
	QTextTableCell *cell = cells.at( i );
	cell->richText()->doLayout( 0, QWIDGETSIZE_MAX );
	cell->cached_sizehint = cell->richText()->widthUsed() + 2 * ( innerborder + 4 );
	if ( cell->cached_sizehint > 32000 ) // nested table in paragraph
	    cell->cached_sizehint = cell->minimumSize().width();
	cell->richText()->doLayout( 0, cell->cached_sizehint );
	cell->cached_width = -1;
    }
    w = widthHint();
    layout->invalidate();
    layout->activate();
    width = minimumWidth();
}

void QTextTable::addCell( QTextTableCell* cell )
{
    cells.append( cell );
    layout->addMultiCell( cell, cell->row(), cell->row() + cell->rowspan()-1,
			  cell->column(), cell->column() + cell->colspan()-1 );
}

void QTextTable::enter( QTextDocument *&doc, QTextParag *&parag, int &idx, int &ox, int &oy, bool atEnd )
{
    if ( !atEnd ) {
	currCell = -1;
	next( doc, parag, idx, ox, oy );
    } else {
	currCell = cells.count();
	prev( doc, parag, idx, ox, oy );
    }
}

void QTextTable::enterAt( QTextDocument *&doc, QTextParag *&parag, int &idx, int &ox, int &oy, const QPoint &pos )
{
    currCell = -1;
    for ( int i = 0; i < (int)cells.count(); ++i ) {
	QTextTableCell *cell = cells.at( i );
	if ( cell->geometry().x() <= pos.x() &&
	     cell->geometry().y() <= pos.y() &&
	     cell->geometry().x() + cell->geometry().width() >= pos.x() &&
	     cell->geometry().y() + cell->geometry().height() >= pos.y() ) {
	    currCell = i;
	    break;
	}
    }

    if ( currCell == -1 ) {
	QTextCustomItem::enterAt( doc, parag, idx, ox, oy, pos );
	return;
    }

    QTextTableCell *cell = cells.at( currCell );
    doc = cell->richText();
    parag = doc->firstParag();
    idx = 0;
    ox += cell->geometry().x() + outerborder + parent->x();
    oy += cell->geometry().y() + outerborder;
}

void QTextTable::next( QTextDocument *&doc, QTextParag *&parag, int &idx, int &ox, int &oy )
{
    currCell++;
    if ( currCell >= (int)cells.count() ) {
	currCell = 0;
	QTextCustomItem::next( doc, parag, idx, ox, oy );
	QTextTableCell *cell = cells.at( 0 );
	doc = cell->richText();
	idx = -1;
	return;
    }
	
    QTextTableCell *cell = cells.at( currCell );
    doc = cell->richText();
    parag = doc->firstParag();
    idx = 0;
    ox += cell->geometry().x() + outerborder + parent->x();
    oy += cell->geometry().y() + outerborder;
}

void QTextTable::prev( QTextDocument *&doc, QTextParag *&parag, int &idx, int &ox, int &oy )
{
    currCell--;
    if ( currCell < 0 ) {
	currCell = 0;
	QTextCustomItem::prev( doc, parag, idx, ox, oy );
	QTextTableCell *cell = cells.at( 0 );
	doc = cell->richText();
	idx = -1;
	return;
    }
	
    QTextTableCell *cell = cells.at( currCell );
    doc = cell->richText();
    parag = doc->firstParag();
    idx = parag->length() - 1;
    ox += cell->geometry().x() + outerborder + parent->x();
    oy += cell->geometry().y() + outerborder;
}

void QTextTable::down( QTextDocument *&doc, QTextParag *&parag, int &idx, int &ox, int &oy )
{
    QTextTableCell *cell = cells.at( currCell );
    if ( cell->row_ == layout->numRows() - 1 ) {
	currCell = 0;
	QTextCustomItem::down( doc, parag, idx, ox, oy );
	QTextTableCell *cell = cells.at( 0 );
	doc = cell->richText();
	idx = -1;
	return;
    }
	
    int oldRow = cell->row_;
    int oldCol = cell->col_;
    for ( int i = currCell; i < (int)cells.count(); ++i ) {
	cell = cells.at( i );
	if ( cell->row_ > oldRow && cell->col_ == oldCol ) {
	    currCell = i;
	    break;
	}
    }
    doc = cell->richText();
    parag = doc->firstParag();
    idx = 0;
    ox += cell->geometry().x() + outerborder + parent->x();
    oy += cell->geometry().y() + outerborder;
}

void QTextTable::up( QTextDocument *&doc, QTextParag *&parag, int &idx, int &ox, int &oy )
{
    QTextTableCell *cell = cells.at( currCell );
    if ( cell->row_ == 0 ) {
	currCell = 0;
	QTextCustomItem::up( doc, parag, idx, ox, oy );
	QTextTableCell *cell = cells.at( 0 );
	doc = cell->richText();
	idx = -1;
	return;
    }
	
    int oldRow = cell->row_;
    int oldCol = cell->col_;
    for ( int i = currCell; i >= 0; --i ) {
	cell = cells.at( i );
	if ( cell->row_ < oldRow && cell->col_ == oldCol ) {
	    currCell = i;
	    break;
	}
    }
    doc = cell->richText();
    parag = doc->lastParag();
    idx = parag->length() - 1;
    ox += cell->geometry().x() + outerborder + parent->x();
    oy += cell->geometry().y() + outerborder;
}

QTextTableCell::QTextTableCell( QTextTable* table,
				int row, int column,
				const QMap<QString, QString> &attr,
				const QStyleSheetItem* style,
				const QTextFormat& fmt, const QString& context,
				const QMimeSourceFactory &factory, const QStyleSheet *sheet,
				const QString& doc)
{
#if defined(PARSER_DEBUG)
    qDebug( debug_indent + "new QTextTableCell1 (pappi: %p)", table );
    qDebug( debug_indent + doc );
#endif
    cached_width = -1;
    cached_sizehint = -1;
    Q_UNUSED( style ); // #### use them
    Q_UNUSED( fmt );

    maxw = QWIDGETSIZE_MAX;
    minw = 0;

    parent = table;
    row_ = row;
    col_ = column;
    stretch_ = 0;
    richtext = new QTextDocument( table->parent );
    richtext->setTableCell( this );
    QString align = *attr.find( "align" );
    if ( !align.isEmpty() ) {
	if ( align.lower() == "left" )
	    richtext->setAlignment( Qt::AlignLeft );
	else if ( align.lower() == "center" )
	    richtext->setAlignment( Qt::AlignHCenter );
	else if ( align.lower() == "right" )
	    richtext->setAlignment( Qt::AlignRight );
    }
    richtext->setFormatter( table->parent->formatter() );
    richtext->setUseFormatCollection( table->parent->useFormatCollection() );
    richtext->setMimeSourceFactory( &factory );
    richtext->setStyleSheet( sheet );
    richtext->setRichText( doc, context );
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

QTextTableCell::QTextTableCell( QTextTable* table, int row, int column )
{
#if defined(PARSER_DEBUG)
    qDebug( debug_indent + "new QTextTableCell2( pappi: %p", table );
#endif
    maxw = QWIDGETSIZE_MAX;
    minw = 0;
    cached_width = -1;
    cached_sizehint = -1;

    parent = table;
    row_ = row;
    col_ = column;
    stretch_ = 0;
    richtext = new QTextDocument( table->parent );
    richtext->setTableCell( this );
    richtext->setFormatter( table->parent->formatter() );
    richtext->setUseFormatCollection( table->parent->useFormatCollection() );
    richtext->setRichText( "<html></html>", QString::null );
    rowspan_ = 1;
    colspan_ = 1;
    background = 0;
    hasFixedWidth = FALSE;
    parent->addCell( this );
}


QTextTableCell::~QTextTableCell()
{
    delete background;
    background = 0;
    delete richtext;
}

QSize QTextTableCell::sizeHint() const
{
    if ( cached_sizehint != -1 )
	return QSize( cached_sizehint, 0 );
    QTextTableCell *that = (QTextTableCell*)this;
    if ( stretch_ )
	return QSize( ( that->cached_sizehint = QWIDGETSIZE_MAX ), 0 );
    return QSize( ( that->cached_sizehint = richtext->widthUsed() + 2 * ( table()->innerborder + 4 ) ), 0 );
}

QSize QTextTableCell::minimumSize() const
{
    if ( stretch_ )
	return QSize( QMAX( minw, parent->width * stretch_ / 100 - 2*parent->cellspacing), 0);
    return QSize(QMAX( richtext->minimumWidth(), minw ),0);
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
    if ( r.width() != cached_width )
	richtext->doLayout( 0, r.width() ); // ### should pass painter() here, but this has to be removed from the formatCollection nad formats when the painter is dead
    cached_width = r.width();
    richtext->setWidth( r.width() );
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
    w = QMAX( minw, w );

    if ( cached_width != w ) {
 	QTextTableCell* that = (QTextTableCell*) this;
	that->richtext->doLayout( 0, w ); // ### should pass painter() here, but this has to be removed from the formatCollection nad formats when the painter is dead
	that->cached_width = w;
    }
    return richtext->height();
}

void QTextTableCell::adjustToPainter()
{
    if ( hasFixedWidth )
	return;

    richtext->doLayout( 0, QWIDGETSIZE_MAX ); // ### should pass painter() here, but this has to be removed from the formatCollection nad formats when the painter is dead
    maxw = richtext->widthUsed() + 6;
    richtext->doLayout( 0, 0 ); // ### should pass painter() here, but this has to be removed from the formatCollection nad formats when the painter is dead
    minw = richtext->widthUsed();
}

QPainter* QTextTableCell::painter() const
{
    return parent->painter;
}

void QTextTableCell::draw( int x, int y, int cx, int cy, int cw, int ch, const QColorGroup& cg )
{
    if ( cached_width != geom.width() ) {
	richtext->doLayout( 0, geom.width() ); // ### should pass painter() here, but this has to be removed from the formatCollection nad formats when the painter is dead
	cached_width = geom.width();
    }
    QColorGroup g( cg );
    if ( background )
	g.setBrush( QColorGroup::Base, *background );
    else if ( richtext->paper() )
	g.setBrush( QColorGroup::Base, *richtext->paper() );
	
    painter()->save();
    painter()->translate( x + geom.x(), y + geom.y() );
    if ( background )
	painter()->fillRect( 0, 0, geom.width(), geom.height(), *background );
    else if ( richtext->paper() )
	painter()->fillRect( 0, 0, geom.width(), geom.height(), *richtext->paper() );

    QRegion r;
    QTextCursor *c = 0;
    if ( richtext->parent()->tmpCursor )
	c = richtext->parent()->tmpCursor;
    if ( cx >= 0 && cy >= 0 )
	richtext->draw( painter(), cx - ( x + geom.x() ), cy - ( y + geom.y() ), cw, ch, g, FALSE, (bool)c, c );
    else
	richtext->draw( painter(), -1, -1, -1, -1, g, FALSE, (bool)c, c );

#if defined(DEBUG_TABLE_RENDERING)
    painter()->save();
    painter()->setPen( Qt::blue );
    painter()->drawRect( 0, 0, geom.width(), geom.height() );
    painter()->restore();
#endif

    painter()->restore();
}


/* a small helper class used internally to resolve Bidi embedding levels.
   Each line of text caches the embedding level at the start of the line for faster
   relayouting
*/
QTextBidiContext::QTextBidiContext(unsigned char l, QChar::Direction e, QTextBidiContext *p, bool o)
    : level(l) , override(o), dir(e)
{
    if ( p )
	p->ref();
    parent = p;
    count = 0;
}

QTextBidiContext::~QTextBidiContext()
{
    if( parent )
	parent->deref();
}

void QTextBidiContext::ref() const
{
    count++;
}

void QTextBidiContext::deref() const
{
    count--;
    if ( count <= 0 )
	delete this;
}

