/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "completion.h"
#include "paragdata.h"
#include "editor.h"
#include <qlistbox.h>
#include <qvbox.h>
#include <qmap.h>
#include <private/qrichtext_p.h>
#include <qapplication.h>
#include <qregexp.h>
#include "arghintwidget.h"
#include <qsizegrip.h>
#include <qtimer.h>
#include <qdesktopwidget.h>
#include <qevent.h>

static QColor getColor( const QString &type )
{
    if ( type == "function" || type == "slot" || type == "package" )
	return Qt::blue;
    else if ( type == "variable" || type == "widget" || type == "dir" )
	return Qt::darkRed;
    else if ( type == "object" || type == "class" )
	return Qt::darkBlue;
    else if ( type == "property" )
	return Qt::darkGreen;
    else if ( type == "enum" )
	return Qt::darkYellow;
    return Qt::black;
}

class CompletionItem : public QListBoxItem
{
public:
    CompletionItem( QListBox *lb, const QString &txt, const QString &t, const QString &p,
		    const QString &pre, const QString &p2 )
	: QListBoxItem( lb ), type( t ), postfix( p ), prefix( pre ), postfix2( p2 ),
	  parag( 0 ), lastState( false ) { setText( txt ); }
    ~CompletionItem() { delete parag; }
    void paint( QPainter *painter ) {
	if ( lastState != isSelected() ) {
	    delete parag;
	    parag = 0;
	}
	lastState = isSelected();
	if ( !parag )
	    setupParagraph();
	parag->paint( *painter, listBox()->colorGroup() );
    }

    int height( const QListBox * ) const {
	if ( !parag )
	    ( (CompletionItem*)this )->setupParagraph();
	return parag->rect().height();
    }
    int width( const QListBox * ) const {
	if ( !parag )
	    ( (CompletionItem*)this )->setupParagraph();
	return parag->rect().width() - 2;
    }
    QString text() const { return QListBoxItem::text() + postfix; }

private:
    void setupParagraph();
    QString type, postfix, prefix, postfix2;
    Q3TextParagraph *parag;
    bool lastState;

};

void CompletionItem::setupParagraph() {
    if ( !parag ) {
	Q3TextFormatter *formatter;
	formatter = new Q3TextFormatterBreakWords;
	formatter->setWrapEnabled( false );
	parag = new Q3TextParagraph( 0 );
	parag->setTabStops( listBox()->fontMetrics().width( "propertyXXXX" ) );
	parag->pseudoDocument()->pFormatter = formatter;
	parag->insert( 0, " " + type + ( type.isEmpty() ? " " : "\t" ) + prefix +
		       QListBoxItem::text() + postfix + postfix2 );
	bool selCol = isSelected() && listBox()->colorGroup().highlightedText() != listBox()->colorGroup().text();
	QColor sc = selCol ? listBox()->colorGroup().highlightedText() : getColor( type );
	Q3TextFormat *f1 = parag->formatCollection()->format( listBox()->font(), sc );
	Q3TextFormat *f3 = parag->formatCollection()->format( listBox()->font(), isSelected() ?
							     listBox()->colorGroup().highlightedText() :
							     listBox()->colorGroup().text() );
	QFont f( listBox()->font() );
	f.setBold( true );
	Q3TextFormat *f2 =
	    parag->formatCollection()->format( f, isSelected() ? listBox()->colorGroup().highlightedText() :
					       listBox()->colorGroup().text() );
	parag->setFormat( 1, type.length() + 1, f1 );
	parag->setFormat( type.length() + 2, prefix.length() + QListBoxItem::text().length(), f2 );
	if ( !postfix.isEmpty() )
	    parag->setFormat( type.length() + 2 + prefix.length() + QListBoxItem::text().length(),
			      postfix.length(), f3 );
	parag->setFormat( type.length() + 2 + prefix.length() + QListBoxItem::text().length() + postfix.length(),
			  postfix2.length(), f3 );
	f1->removeRef();
	f2->removeRef();
	f3->removeRef();
	parag->format();
    }
}


EditorCompletion::EditorCompletion( Editor *e )
{
    enabled = true;
    lastDoc = 0;
    completionPopup = new QVBox( e->topLevelWidget(), 0, WType_Popup );
    completionPopup->setFrameStyle( QFrame::Box | QFrame::Plain );
    completionPopup->setLineWidth( 1 );
    functionLabel = new ArgHintWidget( e->topLevelWidget(), "editor_function_lbl" );
    functionLabel->hide();
    completionListBox = new QListBox( completionPopup, "editor_completion_lb" );
    completionListBox->setFrameStyle( QFrame::NoFrame );
    completionListBox->installEventFilter( this );
    completionListBox->setHScrollBarMode( QScrollView::AlwaysOn );
    completionListBox->setVScrollBarMode( QScrollView::AlwaysOn );
    completionListBox->setCornerWidget( new QSizeGrip( completionListBox, "editor_cornerwidget" ) );
    completionPopup->installEventFilter( this );
    functionLabel->installEventFilter( this );
    completionPopup->setFocusProxy( completionListBox );
    completionOffset = 0;
    curEditor = e;
    curEditor->installEventFilter( this );
}

EditorCompletion::~EditorCompletion()
{
    delete completionPopup;
    delete functionLabel;
}

void EditorCompletion::addCompletionEntry( const QString &s, Q3TextDocument *, bool strict )
{
    QChar key = s.length() ? s[ 0 ] : QChar();
    QMap<QChar, QStringList>::Iterator it = completionMap.find( key );
    if ( it == completionMap.end() ) {
	completionMap.insert( key, QStringList( s ) );
    } else {
	if ( strict ) {
	    QStringList::Iterator sit;
	    for ( sit = (*it).begin(); sit != (*it).end(); ) {
		QStringList::Iterator it2 = sit;
		++sit;
		if ( (*it2).length() > s.length() && (*it2).left( s.length() ) == s ) {
		    if ( (*it2)[ (int)s.length() ].isLetter() && (*it2)[ (int)s.length() ].upper() != (*it2)[ (int)s.length() ] )
			return;
		} else if ( s.length() > (*it2).length() && s.left( (*it2).length() ) == *it2 ) {
		    if ( s[ (int)(*it2).length() ].isLetter() && s[ (int)(*it2).length() ].upper() != s[ (int)(*it2).length() ] )
			(*it).remove( it2 );
		}
	    }
	}
	(*it).append( s );
    }
}

QList<CompletionEntry> EditorCompletion::completionList( const QString &s, Q3TextDocument *doc ) const
{
    if ( doc )
	( (EditorCompletion*)this )->updateCompletionMap( doc );

    QChar key( s[ 0 ] );
    QMap<QChar, QStringList>::ConstIterator it = completionMap.find( key );
    if ( it == completionMap.end() )
	return QList<CompletionEntry>();
    QStringList::ConstIterator it2 = (*it).begin();
    QList<CompletionEntry> lst;
    int len = s.length();
    for ( ; it2 != (*it).end(); ++it2 ) {
	CompletionEntry c;
	c.type = "";
	c.text = *it2;
	c.postfix = "";
	c.prefix = "";
	c.postfix2 = "";
	if ( (int)(*it2).length() > len && (*it2).left( len ) == s && lst.find( c ) == lst.end() )
	    lst << c;
    }

    return lst;
}

void EditorCompletion::updateCompletionMap( Q3TextDocument *doc )
{
    bool strict = true;
    if ( doc != lastDoc )
	strict = false;
    lastDoc = doc;
    Q3TextParagraph *s = doc->firstParagraph();
    if ( !s->extraData() )
	s->setExtraData( new ParagData );
    while ( s ) {
	if ( s->length() == ( (ParagData*)s->extraData() )->lastLengthForCompletion ) {
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
		addCompletionEntry( buffer, doc, strict );
		buffer = QString::null;
	    }
	}
	if ( !buffer.isEmpty() )
	    addCompletionEntry( buffer, doc, strict );

	( (ParagData*)s->extraData() )->lastLengthForCompletion = s->length();
	s = s->next();
    }
}

bool EditorCompletion::doCompletion()
{
    searchString = "";
    if ( !curEditor )
	return false;

    Q3TextCursor *cursor = curEditor->textCursor();
    Q3TextDocument *doc = curEditor->document();

    if ( cursor->index() > 0 && cursor->paragraph()->at( cursor->index() - 1 )->c == '.' &&
	 ( cursor->index() == 1 || cursor->paragraph()->at( cursor->index() - 2 )->c != '.' ) )
	return doObjectCompletion();

    int idx = cursor->index();
    if ( idx == 0 )
	return false;
    QChar c = cursor->paragraph()->at( idx - 1 )->c;
    if ( !c.isLetter() && !c.isNumber() && c != '_' && c != '#' )
	return false;

    QString s;
    idx--;
    completionOffset = 1;
    for (;;) {
	s.prepend( QString( cursor->paragraph()->at( idx )->c ) );
	idx--;
	if ( idx < 0 )
	    break;
	if ( !cursor->paragraph()->at( idx )->c.isLetter() &&
	     !cursor->paragraph()->at( idx )->c.isNumber() &&
	     cursor->paragraph()->at( idx )->c != '_' &&
	     cursor->paragraph()->at( idx )->c != '#' )
	    break;
	completionOffset++;
    }

    searchString = s;

    QList<CompletionEntry> lst( completionList( s, doc ) );
    if ( lst.count() > 1 ) {
	Q3TextStringChar *chr = cursor->paragraph()->at( cursor->index() );
	int h = cursor->paragraph()->lineHeightOfChar( cursor->index() );
	int x = cursor->paragraph()->rect().x() + chr->x;
	int y, dummy;
	cursor->paragraph()->lineHeightOfChar( cursor->index(), &dummy, &y );
	y += cursor->paragraph()->rect().y();
	completionListBox->clear();
	for ( QList<CompletionEntry>::ConstIterator it = lst.begin(); it != lst.end(); ++it )
	    (void)new CompletionItem( completionListBox, (*it).text, (*it).type, (*it).postfix,
				      (*it).prefix, (*it).postfix2 );
	cList = lst;
	completionPopup->resize( completionListBox->sizeHint() +
				 QSize( completionListBox->verticalScrollBar()->width() + 4,
					completionListBox->horizontalScrollBar()->height() + 4 ) );
	completionListBox->setCurrentItem( 0 );
	completionListBox->setFocus();
	if ( curEditor->mapToGlobal( QPoint( 0, y ) ).y() + h + completionPopup->height() < QApplication::desktop()->height() )
	    completionPopup->move( curEditor->mapToGlobal( curEditor->
							   contentsToViewport( QPoint( x, y + h ) ) ) );
	else
	    completionPopup->move( curEditor->mapToGlobal( curEditor->
							   contentsToViewport( QPoint( x, y - completionPopup->height() ) ) ) );
	completionPopup->show();
    } else if ( lst.count() == 1 ) {
	curEditor->insert( lst.first().text.mid( completionOffset, 0xFFFFFF ),
 			   (uint) ( QTextEdit::RedoIndentation |
				    QTextEdit::CheckNewLines |
				    QTextEdit::RemoveSelected ) );
    } else {
	return false;
    }

    return true;
}

bool EditorCompletion::eventFilter( QObject *o, QEvent *e )
{
    if ( !enabled )
	return false;
    if ( e->type() == QEvent::KeyPress && qt_cast<Editor*>(o)) {
	curEditor = (Editor*)o;
	QKeyEvent *ke = (QKeyEvent*)e;
	if ( ke->key() == Key_Tab ) {
	    QString s = curEditor->textCursor()->paragraph()->string()->toString().
			left( curEditor->textCursor()->index() );
	    if ( curEditor->document()->hasSelection( Q3TextDocument::Standard ) ||
		 s.simplifyWhiteSpace().isEmpty() ) {
		if ( curEditor->document()->indent() ) {
		    curEditor->indent();
		    int i = 0;
		    for ( ; i < curEditor->textCursor()->paragraph()->length() - 1; ++i ) {
			if ( curEditor->textCursor()->paragraph()->at( i )->c != ' ' &&
			     curEditor->textCursor()->paragraph()->at( i )->c != '\t' )
			    break;
		    }
		    curEditor->drawCursor( false );
		    curEditor->textCursor()->setIndex( i );
		    curEditor->drawCursor( true );
		} else {
		    curEditor->insert( "\t" );
		}
		return true;
	    }
	}

	if ( functionLabel->isVisible() ) {
	    if ( ke->key() == Key_Up && ( ke->state() & ControlButton ) == ControlButton ) {
		functionLabel->gotoPrev();
		return true;
	    } else if ( ke->key() == Key_Down && ( ke->state() & ControlButton ) == ControlButton ) {
		functionLabel->gotoNext();
		return true;
	    }
	}

	if ( ke->text().length() && !( ke->state() & AltButton ) &&
	     ( !ke->ascii() || ke->ascii() >= 32 ) ||
	     ( ke->text() == "\t" && !( ke->state() & ControlButton ) ) ) {
	    if ( ke->key() == Key_Tab ) {
		if ( curEditor->textCursor()->index() == 0 &&
		     curEditor->textCursor()->paragraph()->isListItem() )
		    return false;
		if ( doCompletion() )
			return true;
	    } else if ( ke->key() == Key_Period &&
			( curEditor->textCursor()->index() == 0 ||
			  curEditor->textCursor()->paragraph()->at( curEditor->textCursor()->index() - 1 )->c != '.' )
			||
			ke->key() == Key_Greater &&
			curEditor->textCursor()->index() > 0 &&
			curEditor->textCursor()->paragraph()->at( curEditor->textCursor()->index() - 1 )->c == '-' ) {
		doObjectCompletion();
	    } else {
		if ( !doArgumentHint( ke->text() == "(" ) )
		    functionLabel->hide();
	    }
	}
    } else if ( o == completionPopup || o == completionListBox ||
	 o == completionListBox->viewport() ) {
	if ( e->type() == QEvent::KeyPress ) {
	    QKeyEvent *ke = (QKeyEvent*)e;
	    if ( ke->key() == Key_Enter || ke->key() == Key_Return || ke->key() == Key_Tab ) {
		if ( ke->key() == Key_Tab && completionListBox->count() > 1 &&
		     completionListBox->currentItem() < (int)completionListBox->count() - 1 ) {
		    completionListBox->setCurrentItem( completionListBox->currentItem() + 1 );
		    return true;
		}
		completeCompletion();
		return true;
	    } else if ( ke->key() == Key_Left || ke->key() == Key_Right ||
			ke->key() == Key_Up || ke->key() == Key_Down ||
			ke->key() == Key_Home || ke->key() == Key_End ||
			ke->key() == Key_Prior || ke->key() == Key_Next ) {
		return false;
	    } else if ( ke->key() != Key_Shift && ke->key() != Key_Control &&
			ke->key() != Key_Alt ) {
		int l = searchString.length();
		if ( ke->key() == Key_Backspace ) {
		    searchString.remove( searchString.length() - 1, 1 );
		} else {
		    searchString += ke->text();
		    l = 1;
		}
		if ( !l || !continueComplete() ) {
		    completionPopup->close();
		    curEditor->setFocus();
		}
		QApplication::sendEvent( curEditor, e );
		return true;
	    }
	} else if ( e->type() == QEvent::MouseButtonDblClick ) {
	    completeCompletion();
	    return true;
	}
    }
    if ( o == functionLabel || qt_cast<Editor*>(o) && functionLabel->isVisible() ) {
	if ( e->type() == QEvent::KeyPress ) {
	    QKeyEvent *ke = (QKeyEvent*)e;
	    if ( ke->key() == Key_Escape ) {
		functionLabel->hide();
	    } else {
		if ( !doArgumentHint( ke->text() == "(" ) )
		    functionLabel->hide();
		if ( o == functionLabel ) {
		    QApplication::sendEvent( curEditor, e );
		    return true;
		}
	    }
	}
    }
    return false;
}

void EditorCompletion::completeCompletion()
{
    int idx = curEditor->textCursor()->index();
    QString s = completionListBox->currentText().mid( searchString.length() );
    curEditor->insert( s, (uint) ( QTextEdit::RedoIndentation |
				   QTextEdit::CheckNewLines |
				   QTextEdit::RemoveSelected ) );
    int i = s.find( '(' );
    completionPopup->close();
    curEditor->setFocus();
    if ( i != -1 && i < (int)s.length() ) {
	curEditor->setCursorPosition( curEditor->textCursor()->paragraph()->paragId(), idx + i + 1 );
	doArgumentHint( false );
    }
}

void EditorCompletion::setCurrentEdior( Editor *e )
{
    curEditor = e;
    curEditor->installEventFilter( this );
}

void EditorCompletion::addEditor( Editor *e )
{
    e->installEventFilter( this );
}

bool EditorCompletion::doObjectCompletion()
{
    searchString = "";
    QString object;
    int i = curEditor->textCursor()->index();
    i--;
    Q3TextParagraph *p = curEditor->textCursor()->paragraph();
    for (;;) {
	if ( i < 0 )
	    break;
	if ( p->at( i )->c == ' ' || p->at( i )->c == '\t' )
	    break;
	object.prepend( p->at( i )->c );
	i--;
    }

    if ( object[ (int)object.length() - 1 ] == '-' )
	object.remove( object.length() - 1, 1 );

    if ( object.isEmpty() )
	return false;
    return doObjectCompletion( object );
}

bool EditorCompletion::doObjectCompletion( const QString & )
{
    return false;
}

static void strip( QString &txt )
{
    int i = txt.find( "(" );
    if ( i == -1 )
	return;
    txt = txt.left( i );
}

bool EditorCompletion::continueComplete()
{
    if ( searchString.isEmpty() ) {
	completionListBox->clear();
	for ( QList<CompletionEntry>::ConstIterator it = cList.begin(); it != cList.end(); ++it )
	    (void)new CompletionItem( completionListBox, (*it).text, (*it).type,
				      (*it).postfix, (*it).prefix, (*it).postfix2 );
	completionListBox->setCurrentItem( 0 );
	completionListBox->setSelected( completionListBox->currentItem(), true );
	return true;
    }

    QListBoxItem *i = completionListBox->findItem( searchString );
    if ( !i )
	return false;

    QString txt1 = i->text();
    QString txt2 = searchString;
    strip( txt1 );
    strip( txt2 );
    if ( txt1 == txt2 && !i->next() )
	return false;

    QList<CompletionEntry> res;
    for ( QList<CompletionEntry>::ConstIterator it = cList.begin(); it != cList.end(); ++it ) {
	if ( (*it).text.left( searchString.length() ) == searchString )
	    res << *it;
    }
    if ( res.isEmpty() )
	return false;
    completionListBox->clear();
    for ( QList<CompletionEntry>::ConstIterator it2 = res.begin(); it2 != res.end(); ++it2 )
	(void)new CompletionItem( completionListBox, (*it2).text, (*it2).type,
				  (*it2).postfix, (*it2).prefix, (*it2).postfix2 );
    completionListBox->setCurrentItem( 0 );
    completionListBox->setSelected( completionListBox->currentItem(), true );
    return true;
}

bool EditorCompletion::doArgumentHint( bool useIndex )
{
    Q3TextCursor *cursor = curEditor->textCursor();
    int i = cursor->index() ;
    if ( !useIndex ) {
	bool foundParen = false;
	int closeParens = 0;
	while ( i >= 0 ) {
	    if ( cursor->paragraph()->at( i )->c == ')' && i != cursor->index() )
		closeParens++;
	    if ( cursor->paragraph()->at( i )->c == '(' ) {
		closeParens--;
		if ( closeParens == -1 ) {
		    foundParen = true;
		    break;
		}
	    }
	    --i;
	}

	if ( !foundParen )
	    return false;
    }
    int j = i - 1;
    bool foundSpace = false;
    bool foundNonSpace = false;
    while ( j >= 0 ) {
	if ( foundNonSpace && ( cursor->paragraph()->at( j )->c == ' ' || cursor->paragraph()->at( j )->c == ',' ) ) {
	    foundSpace = true;
	    break;
	}
	if ( !foundNonSpace && ( cursor->paragraph()->at( j )->c != ' ' || cursor->paragraph()->at( j )->c != ',' ) )
	    foundNonSpace = true;
	--j;
    }
    if ( foundSpace )
	++j;
    j = qMax( j, 0 );
    QString function( cursor->paragraph()->string()->toString().mid( j, i - j + 1 ) );
    QString part = cursor->paragraph()->string()->toString().mid( j, cursor->index() - j + 1 );
    function = function.simplifyWhiteSpace();
    for (;;) {
	if ( function[ (int)function.length() - 1 ] == '(' ) {
	    function.remove( function.length() - 1, 1 );
	    function = function.simplifyWhiteSpace();
	} else if ( function[ (int)function.length() - 1 ] == ')' ) {
	    function.remove( function.length() - 1, 1 );
	    function = function.simplifyWhiteSpace();
	} else {
	    break;
	}
    }

    QChar sep;
    QString pre, post;
    QList<QStringList> argl = functionParameters( function, sep, pre, post );
    if ( argl.isEmpty() )
	return false;

    QString label;
    int w = 0;
    int num = 0;
    if ( !functionLabel->isVisible() )
	functionLabel->setNumFunctions( argl.count() );
    for ( QList<QStringList>::Iterator vit = argl.begin(); vit != argl.end(); ++vit, ++num ) {
	QStringList args = *vit;
	int argNum = 0;
	int inParen = 0;
	for ( int k = 0; k < (int)part.length(); ++k ) {
	    if ( part[ k ] == sep && inParen < 2 )
		argNum++;
	    if ( part[ k ] == '(' )
		inParen++;
	    if ( part[ k ] == ')' )
		inParen--;
	}

	QString func = function;
	int pnt = -1;
	pnt = func.findRev( '.' );
	if ( pnt == -1 )
	    func.findRev( '>' );
	if ( pnt != -1 )
	    func = func.mid( pnt + 1 );

	QString s = func + "( ";
	if ( s[ 0 ] == '\"' )
	    s.remove( (uint)0, 1 );
	i = 0;
	for ( QStringList::Iterator it = args.begin(); it != args.end(); ++it, ++i ) {
	    if ( i == argNum )
		s += "<b>" + *it + "</b>";
	    else
		s += *it;
	    if ( i < (int)args.count() - 1 )
		s += ", ";
	    else
		s += " ";
	}
	s += ")";
	s.prepend( pre );
	s.append( post );
	label += "<p>" + s + "</p>";
	functionLabel->setFunctionText( num, s );
	w = qMax( w, functionLabel->fontMetrics().width( s ) + 10 );
    }
    w += 16;
    if ( label.isEmpty() )
	return false;
    if ( functionLabel->isVisible() ) {
	functionLabel->resize( w + 50, qMax( functionLabel->fontMetrics().height(), 16 ) );
    } else {
	Q3TextStringChar *chr = cursor->paragraph()->at( cursor->index() );
	int h = cursor->paragraph()->lineHeightOfChar( cursor->index() );
	int x = cursor->paragraph()->rect().x() + chr->x;
	int y, dummy;
	cursor->paragraph()->lineHeightOfChar( cursor->index(), &dummy, &y );
	y += cursor->paragraph()->rect().y();
	functionLabel->resize( w + 50, qMax( functionLabel->fontMetrics().height(), 16 ) );
	functionLabel->move( curEditor->mapToGlobal( curEditor->contentsToViewport( QPoint( x, y + h ) ) ) );
	if ( functionLabel->x() + functionLabel->width() > QApplication::desktop()->width() )
	    functionLabel->move( qMax( 0, QApplication::desktop()->width() - functionLabel->width() ),
				 functionLabel->y() );
	functionLabel->show();
	curEditor->setFocus();
    }
    QTimer::singleShot( 0, functionLabel, SLOT( relayout() ) );

    return true;
}

QList<QStringList> EditorCompletion::functionParameters( const QString &, QChar &, QString &, QString & )
{
    return QList<QStringList>();
}

void EditorCompletion::setContext( QObject * )
{
}

void EditorCompletion::showCompletion( const QList<CompletionEntry> &lst )
{
    Q3TextCursor *cursor = curEditor->textCursor();
    Q3TextStringChar *chr = cursor->paragraph()->at( cursor->index() );
    int h = cursor->paragraph()->lineHeightOfChar( cursor->index() );
    int x = cursor->paragraph()->rect().x() + chr->x;
    int y, dummy;
    cursor->paragraph()->lineHeightOfChar( cursor->index(), &dummy, &y );
    y += cursor->paragraph()->rect().y();
    completionListBox->clear();
    for ( QList<CompletionEntry>::ConstIterator it = lst.begin(); it != lst.end(); ++it )
	(void)new CompletionItem( completionListBox, (*it).text, (*it).type,
				  (*it).postfix, (*it).prefix, (*it).postfix2 );
    cList = lst;
    completionPopup->resize( completionListBox->sizeHint() +
			     QSize( completionListBox->verticalScrollBar()->width() + 4,
				    completionListBox->horizontalScrollBar()->height() + 4 ) );
    completionListBox->setCurrentItem( 0 );
    completionListBox->setFocus();
    if ( curEditor->mapToGlobal( QPoint( 0, y ) ).y() + h + completionPopup->height() < QApplication::desktop()->height() )
	completionPopup->move( curEditor->mapToGlobal( curEditor->
						       contentsToViewport( QPoint( x, y + h ) ) ) );
    else
	completionPopup->move( curEditor->mapToGlobal( curEditor->
						       contentsToViewport( QPoint( x, y - completionPopup->height() ) ) ) );

    completionPopup->show();
}
