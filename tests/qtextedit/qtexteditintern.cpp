#include "qtexteditintern_h.cpp"
#include "qtextedit.h"

#include <qstringlist.h>
#include <qfont.h>
#include <qtextstream.h>
#include <qfile.h>
#include <qregexp.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qmap.h>

#include <stdlib.h>

static QMap<QChar, QStringList> *eCompletionMap = 0;

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void QTextEditCommandHistory::addCommand( QTextEditCommand *cmd )
{
    if ( current < (int)history.count() - 1 ) {
	QList<QTextEditCommand> commands;
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

QTextEditCursor *QTextEditCommandHistory::undo( QTextEditCursor *c )
{
    if ( current > -1 ) {
	QTextEditCursor *c2 = history.at( current )->unexecute( c );
	--current;
	return c2;
    }
    return 0;
}

QTextEditCursor *QTextEditCommandHistory::redo( QTextEditCursor *c )
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

QTextEditCursor *QTextEditDeleteCommand::execute( QTextEditCursor *c )
{
    QTextEditParag *s = doc->paragAt( id );
    if ( !s ) {
	qWarning( "can't locate parag at %d, last parag: %d", id, doc->lastParag()->paragId() );
	return 0;
    }

    cursor.setParag( s );
    cursor.setIndex( index );
    int len = text.length();
    doc->setSelectionStart( QTextEditDocument::Temp, &cursor );
    for ( int i = 0; i < len; ++i )
	cursor.gotoRight();
    doc->setSelectionEnd( QTextEditDocument::Temp, &cursor );
    doc->removeSelectedText( QTextEditDocument::Temp, &cursor );

    if ( c ) {
	c->setParag( s );
	c->setIndex( index );
    }

    return c;
}

QTextEditCursor *QTextEditDeleteCommand::unexecute( QTextEditCursor *c )
{
    QTextEditParag *s = doc->paragAt( id );
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

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

QTextEditCursor::QTextEditCursor( QTextEditDocument *d )
    : doc( d )
{
    idx = 0;
    string = doc->firstParag();
    tmpIndex = -1;
}

void QTextEditCursor::insert( const QString &s, bool checkNewLine )
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
	QTextEditParag *p = string->next();
	while ( p ) {
	    p->setParagId( p->prev()->paragId() + 1 );
	    p->move( dy );
	    p->invalidate( 0 );
	    p->setEndState( -1 );
	    p = p->next();
	}
    }	
}

void QTextEditCursor::gotoLeft()
{
    tmpIndex = -1;
    if ( idx > 0 ) {
	idx--;
    } else if ( string->prev() ) {
	string = string->prev();
	idx = string->length() - 1;
    }
}

void QTextEditCursor::gotoRight()
{
    tmpIndex = -1;
    if ( idx < string->length() - 1 ) {
	idx++;
    } else if ( string->next() ) {
	string = string->next();
	idx = 0;
    }
}

void QTextEditCursor::gotoUp()
{
    int indexOfLineStart;
    int line;
    QTextEditString::Char *c = string->lineStartOfChar( idx, &indexOfLineStart, &line );
    if ( !c )
	return;

    tmpIndex = QMAX( tmpIndex, idx - indexOfLineStart );
    if ( indexOfLineStart == 0 ) {
	if ( !string->prev() )
	    return;
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

void QTextEditCursor::gotoDown()
{
    int indexOfLineStart;
    int line;
    QTextEditString::Char *c = string->lineStartOfChar( idx, &indexOfLineStart, &line );
    if ( !c )
	return;

    tmpIndex = QMAX( tmpIndex, idx - indexOfLineStart );
    if ( line == string->lines() - 1 ) {
	if ( !string->next() )
	    return;
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

void QTextEditCursor::gotoLineEnd()
{
    tmpIndex = -1;
    idx = string->length() - 1;
}

void QTextEditCursor::gotoLineStart()
{
    tmpIndex = -1;
    idx = 0;
}

void QTextEditCursor::gotoHome()
{
    tmpIndex = -1;
    string = doc->firstParag();
    idx = 0;
}

void QTextEditCursor::gotoEnd()
{
    if ( !doc->lastParag()->isValid() )
	return;

    tmpIndex = -1;
    string = doc->lastParag();
    idx = string->length() - 1;
}

void QTextEditCursor::gotoPageUp( QTextEdit *view )
{
    tmpIndex = -1;
    QTextEditParag *s = string;
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

void QTextEditCursor::gotoPageDown( QTextEdit *view )
{
    tmpIndex = -1;
    QTextEditParag *s = string;
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

void QTextEditCursor::gotoWordLeft()
{
    tmpIndex = -1;
    QString s( string->string()->toString() );
    bool allowSame = FALSE;
    for ( int i = idx - 1; i >= 0; --i ) {
	if ( !s[ i ].isNumber() && !s[ i ].isLetter() && s[ i ] != '_'  && idx - i > 1 ) {
	    if ( !allowSame &&  s[ i ] == s[ idx ] )
		continue;
	    idx = i;
	    return;
	}
	if ( !allowSame && s[ i ] != s[ idx ] )
	    allowSame = TRUE;
    }

    if ( string->prev() ) {
	string = string->prev();
	idx = string->length() - 1;
    } else {
	gotoLineStart();
    }
}

void QTextEditCursor::gotoWordRight()
{
    tmpIndex = -1;
    QString s( string->string()->toString() );
    bool allowSame = FALSE;
    for ( int i = idx + 1; i < (int)s.length(); ++i ) {
	if ( !s[ i ].isNumber() && !s[ i ].isLetter() && s[ i ] != '_'  && i - idx > 1 ) {
	    if ( !allowSame &&  s[ i ] == s[ idx ] )
		continue;
	    idx = i;
	    return;
	}
	if ( !allowSame && s[ i ] != s[ idx ] )
	    allowSame = TRUE;
    }

    if ( string->next() ) {
	string = string->next();
	idx = 0;
    } else {
	gotoLineEnd();
    }
}

bool QTextEditCursor::atParagStart()
{
    return idx == 0;
}

bool QTextEditCursor::atParagEnd()
{
    return idx == string->length() - 1;
}

void QTextEditCursor::splitAndInsertEmtyParag( bool ind, bool updateIds )
{
    tmpIndex = -1;
    if ( atParagStart() ) {
	QTextEditParag *p = string->prev();
	QTextEditParag *s = new QTextEditParag( doc, p, string, updateIds );
	s->append( " " );
	if ( ind ) {
	    s->indent();
	    s->format();
	    indent();
	    string->format();
	}
    } else if ( atParagEnd() ) {
	QTextEditParag *n = string->next();
	QTextEditParag *s = new QTextEditParag( doc, string, n, updateIds );
	s->append( " " );
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
	string->truncate( idx );
	QTextEditParag *n = string->next();
	QTextEditParag *s = new QTextEditParag( doc, string, n, updateIds );
	s->append( str );
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
}

bool QTextEditCursor::remove()
{
    tmpIndex = -1;
    if ( !atParagEnd() ) {
	string->remove( idx, 1 );
	return FALSE;
    } else if ( string->next() ) {
	string->join( string->next() );
	return TRUE;
    }
    return FALSE;
}

void QTextEditCursor::indent()
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

bool QTextEditCursor::checkOpenParen()
{
    if ( !doc->isParenCheckingEnabled() )
	return FALSE; 
    
    QTextEditParag::ParenList parenList = string->parenList();

    QTextEditParag::Paren openParen, closedParen;
    QTextEditParag *closedParenParag = string;

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
	if ( closedParen.type == QTextEditParag::Paren::Open ) {
	    ignore++;
	    ++i;
	    continue;
	} else {
	    if ( ignore > 0 ) {
		ignore--;
		++i;
		continue;
	    }
	
	    int id = QTextEditDocument::ParenMatch;
	    if ( c == '{' && closedParen.chr != '}' ||
		 c == '(' && closedParen.chr != ')' ||
		 c == '[' && closedParen.chr != ']' )
		id = QTextEditDocument::ParenMismatch;
	    doc->setSelectionStart( id, this );
	    int tidx = idx;
	    QTextEditParag *tstring = string;
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

bool QTextEditCursor::checkClosedParen()
{
    if ( !doc->isParenCheckingEnabled() )
	return FALSE;

    QTextEditParag::ParenList parenList = string->parenList();

    QTextEditParag::Paren openParen, closedParen;
    QTextEditParag *openParenParag = string;

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
	if ( openParen.type == QTextEditParag::Paren::Closed ) {
	    ignore++;
	    --i;
	    continue;
	} else {
	    if ( ignore > 0 ) {
		ignore--;
		--i;
		continue;
	    }
	
	    int id = QTextEditDocument::ParenMatch;
	    if ( c == '}' && openParen.chr != '{' ||
		 c == ')' && openParen.chr != '(' ||
		 c == ']' && openParen.chr != '[' )
		id = QTextEditDocument::ParenMismatch;
	    doc->setSelectionStart( id, this );
	    int tidx = idx;
	    QTextEditParag *tstring = string;
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

QTextEditDocument::QTextEditDocument( const QString &fn, bool tabify )
    : filename( fn )
{
    syntaxHighlighte = 0;
    pFormatter = 0;
    indenter = 0;
    parenCheck = FALSE;
    completion = FALSE;
    fCollection = new QTextEditFormatCollection;
    fParag = 0;

    if ( !filename.isEmpty() ) {
	QFile file( filename );
	file.open( IO_ReadOnly );
	QTextStream ts( &file );

	QString s;
	lParag = 0;
	while ( !ts.atEnd() ) {
	    lParag = new QTextEditParag( this, lParag, 0 );
	    if ( !fParag )
		fParag = lParag;
	    QString s = ts.readLine();
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
	    } else {
		lParag->append( " " );
	    }
	}
    } else {
	lParag = fParag = new QTextEditParag( this, 0, 0 );
	lParag->append( " " );
    }
				
    cx = 2;
    cy = 2;
    cw = 600;

    selectionColors[ Standard ] = Qt::lightGray;
    selectionColors[ ParenMismatch ] = Qt::magenta;
    selectionColors[ ParenMatch ] = Qt::green;
    selectionColors[ Search ] = Qt::yellow;
    commandHistory = new QTextEditCommandHistory( 100 ); // ### max undo/redo steps should be configurable
}

void QTextEditDocument::invalidate()
{
    QTextEditParag *s = fParag;
    while ( s ) {
	s->invalidate( 0 );
	s = s->next();
    }
}

void QTextEditDocument::save( const QString &fn )
{
    if ( !fn.isEmpty() )
	filename = fn;
    if ( !filename.isEmpty() ) {
	QFile file( filename );
	file.open( IO_WriteOnly );
	QTextStream ts( &file );

	QTextEditParag *s = fParag;
	while ( s ) {
	    QString line = s->string()->toString();
	    if ( line.right( 1 ) == " " )
		line.remove( line.length() - 1, 1 );
	    ts << line;
	    ts << "\n";
	    s = s->next();
	}
	file.close();
    }
}

QString QTextEditDocument::fileName() const
{
    return filename;
}

void QTextEditDocument::selectionStart( int id, int &paragId, int &index )
{
    QMap<int, Selection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return;
    Selection &sel = *it;
    paragId = QMIN( sel.startParag->paragId(), sel.endParag->paragId() );
    index = sel.startIndex;
}

QTextEditParag *QTextEditDocument::selectionStart( int id )
{
    QMap<int, Selection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return 0;
    Selection &sel = *it;
    if ( sel.startParag->paragId() <  sel.endParag->paragId() )
	return sel.startParag;
    return sel.endParag;
}

QTextEditParag *QTextEditDocument::selectionEnd( int id )
{
    QMap<int, Selection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return 0;
    Selection &sel = *it;
    if ( sel.startParag->paragId() >  sel.endParag->paragId() )
	return sel.startParag;
    return sel.endParag;
}

bool QTextEditDocument::setSelectionEnd( int id, QTextEditCursor *cursor )
{
    QMap<int, Selection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return FALSE;

    Selection &sel = *it;
    QTextEditParag *oldEndParag = sel.endParag;
    QTextEditParag *oldStartParag = sel.startParag;
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

	QTextEditParag *p = 0;
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
	QTextEditParag *p = sel.startParag;
	p->setSelection( id, start, p->length() - 1 );
	p->setChanged( TRUE );
	p = p->next();
	if ( p )
	    p->setChanged( TRUE );
	while ( p && p != sel.endParag ) {
	    p->setSelection( id, 0, p->length() - 1 );
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

bool QTextEditDocument::removeSelection( int id )
{
    QMap<int, Selection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return FALSE;

    QTextEditParag *start = ( *it ).startParag;
    QTextEditParag *end = ( *it ).endParag;
    if ( end->paragId() < start->paragId() ) {
	end = ( *it ).startParag;
	start = ( *it ).endParag;
    }

    QTextEditParag *p = start;
    while ( p ) {
	p->removeSelection( id );
 	if ( p == end )
 	     break;
	p = p->next();
    }

    selections.remove( id );
    return TRUE;
}

QString QTextEditDocument::selectedText( int id ) const
{
    QMap<int, Selection>::ConstIterator it = selections.find( id );
    if ( it == selections.end() )
	return QString::null;

    Selection sel = *it;

    QTextEditParag *endParag = sel.endParag;
    QTextEditParag *startParag = sel.startParag;
    if ( sel.endParag->paragId() < sel.startParag->paragId() ) {
	startParag = sel.endParag;
	endParag = sel.startParag;
    }

    QString buffer;
    QString s;
    QTextEditParag *p = startParag;
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

void QTextEditDocument::setFormat( int id, QTextEditFormat *f )
{
    QMap<int, Selection>::ConstIterator it = selections.find( id );
    if ( it == selections.end() )
	return;

    Selection sel = *it;

    QTextEditParag *endParag = sel.endParag;
    QTextEditParag *startParag = sel.startParag;
    if ( sel.endParag->paragId() < sel.startParag->paragId() ) {
	startParag = sel.endParag;
	endParag = sel.startParag;
    }

    QTextEditParag *p = startParag;
    while ( p ) {
	p->setFormat( p->selectionStart( id ),
		      p->selectionEnd( id ) - p->selectionStart( id ),
		      f, TRUE );
	if ( p == endParag )
	    break;
	p = p->next();
    }
}

void QTextEditDocument::copySelectedText( int id )
{
    if ( !hasSelection( id ) )
	return;

    QApplication::clipboard()->setText( selectedText( id ) );
}

void QTextEditDocument::removeSelectedText( int id, QTextEditCursor *cursor )
{
    QMap<int, Selection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return;

    Selection sel = *it;
    QTextEditParag *startParag = sel.startParag;
    QTextEditParag *endParag = sel.endParag;
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
	QTextEditParag *cp = 0;
	
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
	QTextEditParag *p = startParag, *tmp;
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

void QTextEditDocument::addCompletionEntry( const QString &s )
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

QStringList QTextEditDocument::completionList( const QString &s ) const
{
    ( (QTextEditDocument*)this )->updateCompletionMap();

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

void QTextEditDocument::updateCompletionMap()
{
    // #############
    // Quite slow the first time the completion list is setup
    // It should be imprved somehow
    // #############
    QTextEditParag *s = fParag;
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

void QTextEditDocument::addCommand( QTextEditCommand *cmd )
{
    commandHistory->addCommand( cmd );
}

QTextEditCursor *QTextEditDocument::undo( QTextEditCursor *c )
{
    return commandHistory->undo( c );
}

QTextEditCursor *QTextEditDocument::redo( QTextEditCursor *c )
{
    return commandHistory->redo( c );
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

QTextEditString::QTextEditString( QTextEditParag *p )
    : parag( p )
{
}

void QTextEditString::insert( int index, const QString &s, QTextEditFormat *f )
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
	data[ (int)index + i ].c = s[ i ];
	data[ (int)index + i ].format = f;
    }
    cache.insert( index, s );
}

void QTextEditString::truncate( int index )
{
    data.truncate( index );
    cache.truncate( index );
}

void QTextEditString::remove( int index, int len )
{
    memmove( data.data() + index, data.data() + index + len,
	     sizeof( Char ) * ( data.size() - index - len ) );
    data.resize( data.size() - len );
    cache.remove( index, len );
}

void QTextEditString::setFormat( int index, QTextEditFormat *f, QTextEditFormatCollection * )
{
    // ############ use formatcollection here
    data[ index ].format = f;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

QTextEditParag::QTextEditParag( QTextEditDocument *d, QTextEditParag *pr, QTextEditParag *nx, bool updateIds )
    : invalid( -1 ), p( pr ), n( nx ), doc( d )
{
    if ( p )
 	p->n = this;
    if ( n )
 	n->p = this;
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
	QTextEditParag *s = n;
	while ( s ) {
	    s->id = s->p->id + 1;
	    s = s->n;
	}
    }
    firstHilite = TRUE;
    lastLenForCompletion = -1;
    
    str = new QTextEditString( this );
}

void QTextEditParag::setNext( QTextEditParag *s )
{
    n = s;
    if ( !n )
	doc->setLastParag( this );
}

void QTextEditParag::setPrev( QTextEditParag *s )
{
    p = s;
    if ( !p )
	doc->setFirstParag( this );
}

void QTextEditParag::invalidate( int chr )
{
    if ( invalid < 0 )
	invalid = chr;
    else
	invalid = QMIN( invalid, chr );
}

void QTextEditParag::insert( int index, const QString &s )
{
    if ( doc->syntaxHighlighter() )
	str->insert( index, s, 
		     doc->syntaxHighlighter()->format( QTextEditSyntaxHighlighter::Standard ) );
    else
	str->insert( index, s, doc->formatCollection()->defaultFormat() );
    invalidate( index );
    needHighlighte = TRUE;
}

void QTextEditParag::truncate( int index )
{
    str->truncate( index );
    append( " " );
    needHighlighte = TRUE;
}

void QTextEditParag::remove( int index, int len )
{
    str->remove( index, len );
    invalidate( 0 );
    needHighlighte = TRUE;
}

void QTextEditParag::join( QTextEditParag *s )
{
    int oh = r.height() + s->r.height();
    n = s->n;
    if ( n )
	n->p = this;
    else
	doc->setLastParag( this );

    if ( str->at( str->length() -1 ).c == ' ' ) // #### check this
	str->truncate( str->length() - 1 );
    append( s->str->toString() );
    delete s;
    invalidate( 0 );
    r.setHeight( oh );
    format();
    needHighlighte = TRUE;
    if ( n ) {
	QTextEditParag *s = n;
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

void QTextEditParag::move( int dy )
{
    if ( dy == 0 )
	return;
    changed = TRUE;
    r.moveBy( 0, dy );
}

void QTextEditParag::format( int start, bool doMove )
{
    if ( str->length() == 0 || !doc->formatter() )
	return;

    if ( doc->syntaxHighlighter() &&
	 ( needHighlighte || state == -1 ) )
 	doc->syntaxHighlighter()->highlighte( this, 0 );
    needHighlighte = FALSE;

    if ( invalid == -1 )
	return;

    r.moveTopLeft( QPoint( doc->x(), p ? p->r.y() + p->r.height() : doc->y() ) );
    r.setWidth( doc->width() );
    QMap<int, LineStart*>::Iterator it = lineStarts.begin();
    for ( ; it != lineStarts.end(); ++it )
	delete *it;
    lineStarts.clear();
    int y = doc->formatter()->format( this, start );

    QTextEditString::Char *c = 0;
    if ( lineStarts.count() == 1 ) {
	c = &str->at( str->length() - 1 );
	r.setWidth( c->x + c->format->width( c->c ) );
    }

    if ( y != r.height() ) {
	int oh = r.height();
	r.setHeight( y );
	if ( doMove && n && n->invalid == -1 ) {
	    int dy;
	    if ( !firstFormat )
		dy = r.height() - oh;
	    else
		dy = r.height();
	    QTextEditParag *s = n;
	    while ( s ) {
		s->move( dy );
		s = s->n;
	    }
	}
    }

    firstFormat = FALSE;
    changed = TRUE;
    invalid = -1;
}

int QTextEditParag::lineHeightOfChar( int i, int *bl, int *y ) const
{
    if ( !isValid() )
	( (QTextEditParag*)this )->format();

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
	
    qWarning( "QTextEditParag::lineHeightOfChar: couldn't find lh for %d", i );
    return 15;
}

QTextEditString::Char *QTextEditParag::lineStartOfChar( int i, int *index, int *line ) const
{
    if ( !isValid() )
	( (QTextEditParag*)this )->format();

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

    qWarning( "QTextEditParag::lineStartOfChar: couldn't find %d", i );
    return 0;
}

int QTextEditParag::lines() const
{
    if ( !isValid() )
	( (QTextEditParag*)this )->format();

    return lineStarts.count();
}

QTextEditString::Char *QTextEditParag::lineStartOfLine( int line, int *index ) const
{
    if ( !isValid() )
	( (QTextEditParag*)this )->format();

    if ( line >= 0 && line < (int)lineStarts.count() ) {
	QMap<int, LineStart*>::ConstIterator it = lineStarts.begin();
	while ( line-- > 0 )
	    ++it;
	int i = it.key();
	if ( index )
	    *index = i;
	return &str->at( i );
    }

    qWarning( "QTextEditParag::lineStartOfLine: couldn't find %d", line );
    return 0;
}

void QTextEditParag::setFormat( int index, int len, QTextEditFormat *f, bool useCollection )
{
    if ( index < 0 )
	index = 0;
    if ( index > str->length() - 1 )
	index = str->length() - 1;
    if ( index + len > str->length() )
	len = str->length() - 1 - index;

    QTextEditFormatCollection *fc = 0;
    if ( useCollection )
	fc = doc->formatCollection();
    QTextEditFormat *of;
    for ( int i = 0; i < len; ++i ) {
	of = str->at( i + index ).format;
	if ( !changed && f != of )
	    changed = TRUE;
	if ( invalid == -1 &&
	     ( f->font().family() != of->font().family() ||
	       f->font().pointSize() != of->font().pointSize() ||
	       f->font().weight() != of->font().weight() ||
	       f->font().italic() != of->font().italic() ) ) {
	    invalidate( 0 );
	}
	str->setFormat( i + index, f, fc );
    }
}

void QTextEditParag::indent( int *oldIndent, int *newIndent )
{
    if ( !doc->indent() ) {
	if ( oldIndent )
	    *oldIndent = 0;
	if ( newIndent )
	    *newIndent = 0;
	if ( oldIndent && newIndent )
	    *newIndent = *oldIndent;
	return;
    }
    doc->indent()->indent( this, oldIndent, newIndent );
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


QTextEditSyntaxHighlighter::QTextEditSyntaxHighlighter( QTextEditDocument *d )
    : doc( d )
{
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

QTextEditFormatter::QTextEditFormatter( QTextEditDocument *d )
    : doc( d )
{
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

QTextEditFormatterBreakInWords::QTextEditFormatterBreakInWords( QTextEditDocument *d )
    : QTextEditFormatter( d )
{
}
    
int QTextEditFormatterBreakInWords::format( QTextEditParag *parag, int start )
{
    QTextEditString::Char *c = 0;
    int x = 0;
    int w = doc->width();
    int y = 0;
    int h = 0;

    // #########################################
    // Should be optimized so that we start formatting
    // really at start (this means the last line begin before start)
    // and not always at the beginnin of the parag!
    start = 0;
    if ( start == 0 ) {
	c = &parag->string()->at( 0 );
    }
    // #########################################

    int i = start;
    QTextEditParag::LineStart *lineStart = new QTextEditParag::LineStart( 0, 0, 0 );
    parag->lineStartList().insert( 0, lineStart );

    for ( ; i < parag->string()->length(); ++i ) {
	c = &parag->string()->at( i );
	if ( i > 0 ) {
	    c->lineStart = 0;
	} else {
	    c->lineStart = 1;
	}
	int ww = 0;
	if ( c->c.unicode() >= 32 || c->c == '\t' ) {
	    ww = c->format->width( c->c );
	} else {
	    ww = c->format->width( ' ' );
	}
	
	if ( x + ww > w ) {
	    x = 0;
	    y += h;
	    h = c->format->height();
	    lineStart = new QTextEditParag::LineStart( y, 0, 0 );
	    parag->lineStartList().insert( i, lineStart );
	    lineStart->baseLine = c->format->ascent();
	    lineStart->h = c->format->height();
	    c->lineStart = 1;
	} else if ( lineStart ) {
	    lineStart->baseLine = QMAX( lineStart->baseLine, c->format->ascent() );
	    h = QMAX( h, c->format->height() );
	    lineStart->h = h;
	}
	
	c->x = x;
	x += ww;
    }
    
    y += h;
    return y;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

QTextEditFormatterBreakWords::QTextEditFormatterBreakWords( QTextEditDocument *d )
    : QTextEditFormatter( d )
{
}
    
int QTextEditFormatterBreakWords::format( QTextEditParag *parag, int start )
{
    QTextEditString::Char *c = 0;
    int x = 0;
    int w = doc->width();
    int y = 0;
    int h = 0;

    // #########################################
    // Should be optimized so that we start formatting
    // really at start (this means the last line begin before start)
    // and not always at the beginnin of the parag!
    start = 0;
    if ( start == 0 ) {
	c = &parag->string()->at( 0 );
    }
    // #########################################

    int i = start;
    QTextEditParag::LineStart *lineStart = new QTextEditParag::LineStart( 0, 0, 0 );
    parag->lineStartList().insert( 0, lineStart );
    int lastSpace = -1;
    int tmpBaseLine = 0, tmph = 0;
    
    for ( ; i < parag->string()->length(); ++i ) {
	c = &parag->string()->at( i );
	if ( i > 0 && x > 0 ) {
	    c->lineStart = 0;
	} else {
	    c->lineStart = 1;
	}
	int ww = 0;
	if ( c->c.unicode() >= 32 || c->c == '\t' ) {
	    ww = c->format->width( c->c );
	} else {
	    ww = c->format->width( ' ' );
	}
	
	if ( x + ww > w ) {
	    if ( lastSpace == -1 ) {
		if ( lineStart ) {
		    lineStart->baseLine = QMAX( lineStart->baseLine, tmpBaseLine );
		    h = QMAX( h, tmph );
		    lineStart->h = h;
		}
		x = 0;
		y += h;
		tmph = c->format->height();
		h = 0;
		lineStart = new QTextEditParag::LineStart( y, 0, 0 );
		parag->lineStartList().insert( i, lineStart );
		lineStart->baseLine = c->format->ascent();
		lineStart->h = c->format->height();
		c->lineStart = 1;
		tmpBaseLine = lineStart->baseLine;
		lastSpace = -1;
	    } else {
		i = lastSpace;
		x = 0;
		y += h;
		tmph = c->format->height();
		h = tmph;
		lineStart = new QTextEditParag::LineStart( y, 0, 0 );
		parag->lineStartList().insert( i + 1, lineStart );
		lineStart->baseLine = c->format->ascent();
		lineStart->h = c->format->height();
		c->lineStart = 1;
		tmpBaseLine = lineStart->baseLine;
		lastSpace = -1;
		continue;
	    }
	} else if ( lineStart && c->c == ' ' ) {
	    tmpBaseLine = QMAX( tmpBaseLine, c->format->ascent() );
	    tmph = QMAX( tmph, c->format->height() );
	    lineStart->baseLine = QMAX( lineStart->baseLine, tmpBaseLine );
	    h = QMAX( h, tmph );
	    lineStart->h = h;
	    lastSpace = i;
	} else {
	    tmpBaseLine = QMAX( tmpBaseLine, c->format->ascent() );
	    tmph = QMAX( tmph, c->format->height() );
	}
	
	c->x = x;
	x += ww;
    }
    
    y += h;
    return y;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

QTextEditIndent::QTextEditIndent( QTextEditDocument *d )
    : doc( d )
{
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

QTextEditFormatCollection::QTextEditFormatCollection()
{
    defFormat = new QTextEditFormat( QApplication::font(), 
				     QApplication::palette().color( QPalette::Normal, QColorGroup::Text ) );
}

QTextEditFormat *QTextEditFormatCollection::format( QTextEditFormat *f )
{
    QTextEditFormat *fm = new QTextEditFormat( *f );
    return fm;
}

QTextEditFormat *QTextEditFormatCollection::format( const QFont &f, const QColor &c )
{
    return new QTextEditFormat( f, c );
}
