/*
  main.cpp
*/

#include <qdatetime.h>
#include <qdir.h>
#include <qfile.h>
#include <qmap.h>
#include <qregexp.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtextstream.h>

#include <errno.h>
#include <stdlib.h>

static QString yyIn;
static int yyPos;

struct Book
{
    QString fileName;
    QString title;
    QStringList chapters;
    QStringList chapterTitles;
    QMap<QString, int> links;

    Book() { }
    Book( const QString& fileName0 ) : fileName( fileName0 ) { }
};

static QValueList<Book>::Iterator yyCurrentBook;
static QValueList<Book> yyBooks;
static QMap<QString, int> yyUsedQdocFiles;

static QString qdocOutputDir;
static QString inputDir; // never set
static QStringList fileNameList;

static const char tableDataFmt[] =
    ">{\\small\\raggedright\\arraybackslash}";
static const char tableHeaderFmt[] =
    ">{\\bfseries\\small\\centering\\arraybackslash}";

static const char laTeXPrologue[] =
    "% %1\n"
    "% Generated by qdoc2latex on %3\n"
    "\n"
    "\\documentclass%4{%5}\n"
    "\\usepackage[T1]{fontenc}\n"
    "\\usepackage{graphicx}\n"
    "\\usepackage{makeidx}\n"
    "\\usepackage{multicol}\n"
    "\\usepackage{multirow}\n"
    "\\usepackage{tabularx}\n"
    "\n"
    "\\makeindex\n"
    "\n"
    "\\renewcommand{\\familydefault}{bch}\n"
    "\n"
    "\\setlength{\\hoffset}{15mm}\n"
    "\\setlength{\\voffset}{15mm}\n"
    "\\setlength{\\oddsidemargin}{%6}\n"
    "\\setlength{\\evensidemargin}{0pt}\n"
    "\\setlength{\\topmargin}{0pt}\n"
    "\\setlength{\\marginparsep}{0pt}\n"
    "\\setlength{\\marginparwidth}{0pt}\n"
    "\n"
    "\\setlength{\\textheight}{\\paperheight}\n"
    "\\addtolength{\\textheight}{-2\\voffset}\n"
    "\\addtolength{\\textheight}{-\\topmargin}\n"
    "\\addtolength{\\textheight}{-\\headheight}\n"
    "\\addtolength{\\textheight}{-\\headsep}\n"
    "\\addtolength{\\textheight}{-\\footskip}\n"
    "\n"
    "\\setlength{\\textwidth}{\\paperwidth}\n"
    "\\addtolength{\\textwidth}{-2\\hoffset}\n"
    "\\addtolength{\\textwidth}{-\\oddsidemargin}\n"
    "\n"
    "\\addtolength{\\hoffset}{-1in}\n"
    "\\addtolength{\\voffset}{-1in}\n"
    "\n"
    "\\setlength{\\parskip}{0.65em}\n"
    "\\setlength{\\parindent}{0pt}\n"
    "\n"
    "\\pagestyle{myheadings}\n"
    "\\setcounter{secnumdepth}{-1}\n"
    "\n"
    "\\def\\bull{\\vrule height0.9ex width0.8ex depth-0.1ex}\n"
    "\n"
    "\\catcode`\\@=11 % access LaTeX2e internal commands (with care)\n"
    "\n"
    "% cf. The LaTeX Companion, p. 364\n"
    "\\renewenvironment{theindex}{\\newpage\n"
    "  \\label{index}%\n"
    "  \\markboth{\\indexname}{\\indexname}%\n"
    "  \\thispagestyle{plain}\n"
    "  \\begin{multicols}{3}[\\@makechapterhead{\\indexname}]%\n"
    "  \\parindent=0pt \\parskip=0pt\n"
    "  \\raggedright\n"
    "  \\small\n"
    "  \\par\\bigskip\n"
    "  \\let\\item\\@idxitem}%\n"
    " {\\end{multicols}}\n"
    "\n"
    "\\let\\dottedtocline=\\@dottedtocline\n"
    "\n"
    "\\catcode`\\@=12\n"
    "\n"
    "\\begin{document}\n"
    "\\begin{sloppy}\n"
    "\\vfill\n"
    "\n"
    "\\begin{center}\n"
    "\n"
    "\\thispagestyle{empty}\n"
    "\n"
    "\\vspace*{0pt plus 1fill}\n"
    "\n"
    "{\\Huge %2}\n"
    "\n"
    "\\bigskip\n"
    "\\bigskip\n"
    "\n"
    "{\\Large Qt 3.0}\n"
    "\n"
    "\\vfill\n"
    "\n"
    "Copyright \\copyright{} 2001 Trolltech AS. All rights reserved.\n"
    "\\end{center}\n"
    "\n"
    "\\bigskip\n"
    "\n"
    "TROLLTECH, Qt and the Trolltech logo are registered trademarks of\n"
    "Trolltech AS. Linux is a registered trademark of Linus Torvalds. UNIX\n"
    "is a registered trademark of X/Open Company Ltd. Mac is a registered\n"
    "trademark of Apple Computer Inc. MS Windows is a registered trademark\n"
    "of Microsoft Corporation. All other products named are trademarks of\n"
    "their respective owners.\n"
    "\n"
    "\\bigskip\n"
    "\\bigskip\n"
    "\n"
    "{\\small The definitive Qt documentation is provided in HTML\n"
    "format supplied with Qt, and available online at\n"
    "http://doc.trolltech.com. This PDF file was generated\n"
    "automatically from the HTML source as a convenience to users,\n"
    "although PDF is not an official Qt documentation format.}\n"
    "\n"
    "\\vspace*{0pt plus 1fill}\n"
    "\\newpage\n";

static const char laTeXEpilogue[] =
    "\\printindex\n"
    "\\end{sloppy}\n"
    "\\end{document}\n";

static bool eoi()
{
    return yyPos == (int) yyIn.length();
}

static void skipSpaces()
{
    while ( !eoi() && yyIn[yyPos].isSpace() )
	yyPos++;
}

static QString getWord()
{
    QString t;
    skipSpaces();
    while ( !eoi() && !yyIn[yyPos].isSpace() ) {
	t += yyIn[yyPos];
	yyPos++;
    }
    return t;
}

static QString untabified( const QString& in )
{
    QString res;
    int col = 0;

    for ( int i = 0; i < (int) in.length(); i++ ) {
	if ( in[i] == QChar('\t') ) {
	    res += QString( "        " + (col & 0x7) );
	    col = ( col + 8 ) & ~0x7;
	} else {
	    res += in[i];
	    if ( in[i] == QChar('\n') )
		col = 0;
	    else
		col++;
	}
    }
    return res;
}

static QString fileContents( const QString& fileName )
{
    QFile f( fileName );
    if ( !f.open(IO_ReadOnly) ) {
	qWarning( "Cannot open file '%s' for reading: %s", fileName.latin1(),
		  strerror(errno) );
	return QString::null;
    }

    QTextStream t( &f );
    QString contents = t.read();
    f.close();
    if ( contents.isEmpty() )
	qWarning( "File '%s' is empty", fileName.latin1() );
    return contents;
}

static void use( const QString& fileName )
{
    int slash = fileName.findRev( QChar('/') );
    if ( yyUsedQdocFiles.contains(fileName.mid(slash + 1)) )
	qWarning( "File '%s' used twice", fileName.mid(slash + 1).latin1() );
    yyUsedQdocFiles.insert( fileName.mid(slash + 1), 0 );
}

static void add( const QString& fileName )
{
    (*yyCurrentBook).chapters.append( fileName );
    use( fileName );
}

static void stripComments()
{
    QRegExp cmt( QString("#[^\n]*") );

    yyIn.replace( cmt, QString::null );
}

static bool matchCommand()
{
    QRegExp cmd( QString("^\\\\[a-z]+") );
    QRegExp titleEnd( QString("\n\\s*\n") );

    QString t;

    if ( cmd.search(yyIn.mid(yyPos, 16)) == -1 )
	return FALSE;
    yyPos += cmd.matchedLength();

    if ( cmd.cap(0) == QString("\\book") ) {
	t = getWord();
	t.replace( QRegExp(QString("\\.book$")), QString::null );
	t += QString( ".html" );
	add( qdocOutputDir + t );
    } else if ( cmd.cap(0) == QString("\\class") ) {
	add( qdocOutputDir + getWord().lower() + QString(".html") );
    } else if ( cmd.cap(0) == QString("\\defgroup") ) {
	add( qdocOutputDir + getWord() + QString(".html") );
    } else if ( cmd.cap(0) == QString("\\file") ) {
	t = getWord();
	t.replace( QRegExp(QString("[./]")), QChar('-') );
	t += QString( ".html" );
	add( qdocOutputDir + t );
    } else if ( cmd.cap(0) == QString("\\input") ) {
	add( inputDir + getWord() );
    } else if ( cmd.cap(0) == QString("\\page") ) {
	add( qdocOutputDir + getWord() );
    } else if ( cmd.cap(0) == QString("\\plainpage") ) {
	add( qdocOutputDir + getWord() );
    } else if ( cmd.cap(0) == QString("\\title") ) {
	int k = titleEnd.search( yyIn.mid(yyPos) );
	if ( k == -1 )
	    k = yyIn.length() - yyPos;
	(*yyCurrentBook).title = yyIn.mid( yyPos, k ).simplifyWhiteSpace();
	yyPos += k;
    } else {
	return FALSE;
    }
    return TRUE;
}

static bool processQbook()
{
    QRegExp friendly( QString("<!-- friendly -->") );

    QRegExp h1( QString("<h1[^>]*>(.*)</h1>") );
    h1.setMinimal( TRUE );

    QString base = (*yyCurrentBook).fileName;
    int k = base.findRev( QChar('/') );
    if ( k != -1 )
	base = base.mid( k + 1 );
    base.truncate( base.length() - 4 ); // chop '.tex'

    QRegExp chapX( QString("^%1-([0-9]+).html$").arg(base) );

    QMap<int, QString> chapters;
    QStringList::ConstIterator f = fileNameList.begin();
    while ( f != fileNameList.end() ) {
	if ( (*f).find(chapX) != -1 )
	    chapters.insert( chapX.cap(1).toInt(), *f );
	++f;
    }

    QMap<int, QString>::ConstIterator c = chapters.begin();
    while ( c != chapters.end() ) {
	add( qdocOutputDir + *c );
	++c;
    }

    QString fn = qdocOutputDir + QString( "%1.html" ).arg( base );
    QString master = fileContents( fn );
    use( fn );

    if ( h1.search(master) != -1 )
	(*yyCurrentBook).title = h1.cap( 1 ).simplifyWhiteSpace();
    if ( friendly.search(master) == -1 ) {
	qWarning( "File '%s' must be regenerated with qdoc option '--friendly'",
		  fn.latin1() );
	return FALSE;
    }
    return TRUE;
}

QString yyOut;

static void extendedReplace( QString& in, const QRegExp& rx,
			     const QString& str )
{
    QRegExp tx = rx;
    int index = 0;
    while ( index < (int) in.length() ) {
	index = tx.search( in, index );
	if ( index == -1 )
	    break;

	QString tstr;
	int j = 0;
	while ( j < (int) str.length() - 1 ) {
	    if ( str[j] == QChar('%') && str[j + 1].isDigit() ) {
		int nth = str[j + 1].unicode() - '0';
		if ( nth <= (int) tx.capturedTexts().count() )
		    tstr += tx.cap( nth );
		j++;
	    } else {
		tstr += str[j];
	    }
	    j++;
	}
	if ( j < (int) str.length() )
	    tstr += str[j];

	in.replace( index, tx.matchedLength(), tstr );
	index += tstr.length();

	// avoid infinite loop on 0-length matches (e.g., [a-z]*)
	if ( tx.matchedLength() == 0 )
	    index++;
    }
}

static QRegExp aname( "<\\s*[aA]\\s+[nN][aA][mM][eE]\\s*=\\s*([^>]+)\\s*>" );

static void moveANameAfterHeading( QString& html )
{
    QRegExp anameBeforeHx( aname.pattern() + QString(
	"\\s*(?:<\\s*/\\s*[aA]\\s*>\\s*)?"
	"(<\\s*[hH][1-6]\\s*>.*<\\s*/\\s*[hH][1-6]\\s*>)") );
    anameBeforeHx.setMinimal( TRUE );

    int k = 0;
    while ( (k = html.find(anameBeforeHx, k)) != -1 ) {
	QString hx = anameBeforeHx.cap( 2 );
	int hxpos = anameBeforeHx.pos( 2 );
	int hxlen = anameBeforeHx.cap( 2 ).length();

	html.replace( hxpos, hxlen, QString::null );
	html.insert( k, hx );
	k += anameBeforeHx.matchedLength();
    }
}

static void cleanLabel( QString& lab )
{
    if ( lab.startsWith(QChar('"')) || lab.startsWith(QChar('\'')) )
	lab = lab.mid( 1, lab.length() - 2 );
    lab.replace( QRegExp(QString("\\{\\\\char`\\\\~\\}")), QString("tilde-") );
    lab.replace( QRegExp(QString("\\{\\\\char`\\\\\\^\\}")),
		 QString("caret-") );
    lab.replace( QRegExp(QString("\\\\_")), QString("-underscore-") );
    lab = lab.simplifyWhiteSpace();
}

static QString compressedLabel( const QString& lab )
{
    static QMap<QString, int> labels;

    return QString::number( *labels.insert(lab, labels.count(), FALSE), 36 );
}

static void indexify( QString& html )
{
    QRegExp h3( QString("<h3 class=fn>.*<a name=\"[^\"]*\"></a>(.*)</h3>") );
    h3.setMinimal( TRUE );

    QRegExp function( QString("([A-Za-z0-9_]+)::([^(]*)(?:&nbsp;| )\\(.*") );
    QRegExp xtor( QString("([A-Za-z0-9_]+)::~?\\1(?:&nbsp;| ).*") );
    QRegExp type( QString("([A-Za-z0-9_]+)::([A-Za-z0-9_]+)") );
    QRegExp property( QString("([A-Za-z0-9_]+)") );

    QRegExp enumValue( QString(
	"<li><a name=\"([^\"]*)\"></a><tt>([A-Za-z0-9_]*)"
	"::\\1</tt>") );

    QString cl;

    int k = 0;
    while ( (k = html.find(h3, k)) != -1 ) {
	QString index;
	QString member = h3.cap( 1 );

	if ( function.exactMatch(member) ) {
	    if ( !xtor.exactMatch(member) ) {
		cl = function.cap( 1 );
		QString fn = function.cap( 2 );
		index = QString( "%1()!%2" ).arg( fn ).arg( cl );
	    }
	} else if ( type.exactMatch(member) ) {
	    cl = type.cap( 1 );
	    QString en = type.cap( 2 );
	    index = QString( "%1!%2" ).arg( en ).arg( cl );
	} else if ( property.exactMatch(member) ) {
	    // cl is almost certainly set correctly
	    QString pr = property.cap( 1 );
	    index = QString( "%1!%2" ).arg( pr ).arg( cl );
	}

	if ( !index.isEmpty() )
	    html.insert( k + h3.matchedLength(),
			 QString("\\index{%1}\n").arg(index) );
	k++;
    }

    k = 0;
    while ( (k = html.find(enumValue, k)) != -1 ) {
	QString ev = enumValue.cap( 1 );
	cl = enumValue.cap( 2 );
	html.insert( k + enumValue.matchedLength(),
		     QString("\\index{%1!%2}\n").arg(ev).arg(cl) );
	k++;
    }
}

static void laTeXifyAHref( QString& html, int pos, const QString& chapterLabel )
{
    QRegExp ahref( QString(
	"^<\\s*[aA]\\s+[hH][rR][eE][fF]\\s*=\\s*([^>]+)>.*<\\s*/\\s*a\\s*>"
	"(?:\\(\\))?(?!\\(\\))") );
    QRegExp protocol( QString("^(?http|ftp):") );
    ahref.setMinimal( TRUE );

    QConstString cstr( html.unicode() + pos, html.length() - pos );
    if ( cstr.string().find(ahref) != -1 ) {
	QString n = ahref.cap( 1 );
	n.replace( QRegExp(QString("\\\\#")), QChar('#') );
	cleanLabel( n );
	if ( n.startsWith(QChar('#')) )
	    n.prepend( chapterLabel );
	n = compressedLabel( n );

	int ins = pos + ahref.matchedLength();

	if ( (*yyCurrentBook).links.contains(n) ) {
	    html.insert( ins, QString(" [p.~\\pageref{%1}]").arg(n) );
	} else if ( n.find(protocol) == 0 ) {
	    // avoid silly "http://www.troll.no [http://www.troll.no]"
	    if ( ahref.cap(0).contains(protocol) == 1 )
		html.insert( ins, QString(" [%1]").arg(n) );
	} else {
	    QValueList<Book>::ConstIterator b = yyBooks.begin();
	    while ( b != yyBooks.end() ) {
		if ( (*b).links.contains(n) ) {
		    html.insert( ins, QString(" [%1]").arg((*b).title) );
		    break;
		}
		++b;
	    }
	}
    }
}

static bool laTeXifyHeadings( QString& html, const QString& chapterLabel )
{
    QRegExp hx( QString("<\\s*([hH]([1-6]))[^>]*>(.*)<\\s*/\\s*\\1\\s*>") );
    hx.setMinimal( TRUE );

    QRegExp obsolete( QString("<br><small>\\[obsolete\\]</small>") );
    QRegExp module( QString("<br><small>\\[.*>([^ ]*) module</a>\\]</small>") );
    QRegExp tt( QString("\\\\texttt") );

    int numH1 = 0;

    int k = 0;
    while ( (k = html.find(hx, k)) != -1 ) {
	int level = hx.cap( 2 )[0].unicode() - '0';
	QString title = hx.cap( 3 ).simplifyWhiteSpace();
	QString s;
	QString extra;

	title.replace( tt, QString("\\hbox") );

	if ( level == 1 ) {
	    int ell;

	    ell = title.find( module );
	    if ( ell != -1 )
		title.truncate( ell );
	    ell = title.find( obsolete );
	    if ( ell != -1 ) {
		title.truncate( ell );
		title.append( QString(" (obsolete)") );
	    }

	    s = QString( "chapter" );
	    extra = QString( "\n\\markboth{%1}{%2}\n"
			     "\\label{%3}" )
		    .arg( title )
		    .arg( title )
		    .arg( compressedLabel(chapterLabel) );
	    if ( !module.cap(1).isEmpty() ) {
		ell = html.find( QString("include"), k );
		if ( ell != -1 ) {
		    ell = html.findRev( QString("\n\n"), ell );
		    if ( ell != -1 )
			html.insert( ell, QString("\n\nThis class is part of"
						  " the \\textbf{%1} module.")
					  .arg(module.cap(1)) );
		}
	    }
	    (*yyCurrentBook).chapterTitles.append( title );
	    numH1++;
	} else {
	    s = QString( "section" );
	    while ( level > 2 ) {
		s.prepend( QString("sub") );
		level--;
	    }
	}

	QString hang;
	if ( hx.cap(0).find(QString("class=fn")) != -1 )
	    hang = QString( "\\hangindent=\\leftmargini\\hangafter=1" );

	html.replace( k, hx.matchedLength(),
		      QString("\n\\%1*{\\raggedright%2 %3}%4\n")
		      .arg(s).arg(hang).arg(title).arg(extra) );
	k++;
    }

    if ( numH1 != 1 ) {
	qWarning( "Confusing hardcoded '<h1>' tags in '%s'",
		  chapterLabel.latin1() );
	return FALSE;
    }
    return TRUE;
}

static void laTeXifyLists( QString& html )
{
    QRegExp empty( QString("<\\s*[oOuU][lL][^>]*>\\s*<\\s*/\\s*\\1\\s*>") );
    QRegExp ol( QString("<\\s*[oO][lL][^>]*>") );
    QRegExp ul( QString("<\\s*[uU][lL][^>]*>") );
    QRegExp slashOl( QString("<\\s*/\\s*[oO][lL][^>]*>") );
    QRegExp slashUl( QString("<\\s*/\\s*[uU][lL][^>]*>") );
    QRegExp li( QString("<\\s*[lL][iI][^>]*>") );

    html.replace( empty, QString::null );
    html.replace( ol, QString(
	"\\begin{enumerate}\n"
	"\\setlength{\\itemsep}{0pt}\n") );
    int k = 0;
    while ( (k = html.find(ul, k)) != -1 ) {
	if ( html.mid(k, 100).find("<div class=fn>") == -1 ) {
	    html.replace( k, ul.matchedLength(), QString(
		"\\begin{itemize}\n"
		"\\setlength{\\itemsep}{0pt}\n") );
	} else {
	    /*
	      Function declaration lists are treated specially.
	    */
	    html.replace( k, ul.matchedLength(), QString(
		"\\begin{itemize}\n"
		"\\raggedright\n"
		"\\hyphenpenalty=10000\n"
		"\\setlength{\\itemsep}{-0.2em}\n") );

	    html.replace( QRegExp(QString("(?:&nbsp;|~)- ")),
			  QString(" --- ") );
	    int end = html.find( slashUl, k );
	    int ell = k;
	    while ( (ell = html.find(li, ell)) != -1 ) {
		if ( ell > end )
		    break;
		html.replace( ell, li.matchedLength(),
			      QString("\\item[$\\bull$] ") );
		ell++;
	    }
	}
	k++;
    }
    html.replace( slashOl, QString("\\end{enumerate}\n") );
    html.replace( slashUl, QString("\\end{itemize}\n") );
    html.replace( li, QString("\\item ") );
}

static void laTeXifyEntities( QString& html )
{
    QRegExp backslash( QString("&backslash;?") );
    QRegExp lt( QString("&lt;?") );
    QRegExp gt( QString("&gt;?") );
    QRegExp nbsp( QString("&nbsp;?") );
    QRegExp num( QString("&\\\\?#([0-9]+);?") );
    QRegExp amp( QString("&amp;?") );
    QRegExp looseAmp( QChar('&') );

    html.replace( backslash, QString("{\\char`\\\\}") );
    html.replace( lt, QString("{\\char60}") );
    html.replace( gt, QString("{\\char62}") );
    html.replace( nbsp, QChar('~') );

    int k = 0;
    while ( (k = html.find(num, k)) != -1 ) {
	html.replace( k, num.matchedLength(),
		      QString("{\\char%1}").arg(num.cap(1)) );
	k++;
    }

    html.replace( amp, QChar('&') );
    html.replace( looseAmp, QString("\\&") );
}

struct Cell
{
    int row;
    int col;
    int rowSpan;
    int colSpan;
    QString text;
    bool header;
    int percent;

    Cell()
	: row( 0 ), col( 0 ), rowSpan( 1 ), colSpan( 1 ), header( FALSE ),
	  percent( 0 ) { }
};

static const int MaxCols = 64;

static int approxTextWidth( const QString& text )
{
    return text.length();
}

static void computeTableColumnWidths( const QValueList<Cell>& cells,
				      QMemArray<int> *columnPercents )
{
    int n = columnPercents->count();
    if ( n == 0 )
	return;

    QMemArray<int> sumWidths( n );
    sumWidths.fill( 0 );

    QMemArray<int> numNonEmpty( n );
    numNonEmpty.fill( 0 );

    QValueList<Cell>::ConstIterator c = cells.begin();
    while ( c != cells.end() ) {
	if ( (*c).colSpan == 1 ) {
	    sumWidths[(*c).col] += approxTextWidth( (*c).text );
	    numNonEmpty[(*c).col] += (*c).rowSpan;
	}
	++c;
    }

    int sum = 0;
    for ( int i = 0; i < n; i++ ) {
	(*columnPercents)[i] = 1000 * sumWidths[i] / ( numNonEmpty[i] + 1 ) +
			       10000;
	sum += (*columnPercents)[i];
    }

    const int Total = 97;
    int totalSoFar = 0;
    for ( int i = 0; i < n - 1; i++ ) {
	(*columnPercents)[i] /= ( sum / Total );
	totalSoFar += (*columnPercents)[i];
    }
    (*columnPercents)[n - 1] = Total - totalSoFar;
}

static double percentToDouble( int percent )
{
    return (double) qMax( percent - 3, 3 ) / 100;
}

static void laTeXifyTables( QString& html )
{
    QRegExp table( QString("<table\\b[^>]*>") );
    QRegExp slashTable( QString("</table>") );
    QRegExp tx( QString("<t([rhd])\\b([^>]*)>") );
    QRegExp closeTx( QString("\\s*</t[rhd]>") );
    QRegExp xspan( QString("(col|row)span=\"?([1-9])\"?") );

    int k = 0;
    while ( (k = html.find(table, k)) != -1 ) {
	int tableEnd = html.find( slashTable, k );
	if ( tableEnd != -1 ) {
	    QValueList<Cell> cells;
	    int numCols = 0;
	    int numRows = -1;
	    int nextCol = 0;

	    QMemArray<int> leftOvers( MaxCols );
	    leftOvers.fill( 0 );

	    QValueList<Cell>::Iterator c;

	    /*
	      Extract the whole HTML table.
	    */
	    QString t = html.mid( k + table.matchedLength(),
				  tableEnd - (k + table.matchedLength()) );
	    html.remove( k, tableEnd - k + slashTable.matchedLength() );
	    t.replace( closeTx, QString::null );

	    /*
	      Parse the table cells.
	    */
	    int start = t.find( tx );
	    if ( start == -1 || tx.cap(1) != QChar('r') ) {
		qWarning( "Missing '<tr>' in '<table>'" );
		return;
	    }
	    start += tx.matchedLength();

	    for ( ;; ) {
		QString x = tx.cap( 1 );
		QString attrs = tx.cap( 2 );

		int end = t.find( tx, start );
		if ( end == -1 )
		    end = t.length();

		if ( x == QChar('r') ) {
		    for ( int i = 0; i < numCols; i++ ) {
			if ( leftOvers[i] > 0 )
			    leftOvers[i]--;
		    }
		    numRows++;
		    nextCol = 0;
		} else {
		    Cell cell;

		    while ( leftOvers[nextCol] > 0 )
			nextCol++;
		    cell.col = nextCol;
		    cell.row = numRows;

		    int ell = 0;
		    while ( attrs.find(xspan, ell) != -1 ) {
			int span = xspan.cap( 2 ).toInt();
			if ( xspan.cap(1) == QString("col") ) {
			    cell.colSpan = span;
			} else {
			    cell.rowSpan = span;
			}
			ell++;
		    }

		    cell.text = t.mid( start, end - start ).stripWhiteSpace();
		    cell.header = ( x == QChar('h') );

		    cells.append( cell );

		    nextCol += cell.colSpan;
		    if ( nextCol > numCols ) {
			numCols = nextCol;
			if ( numCols > MaxCols ) {
			    qWarning( "Too many columns in table (%d)",
				      numCols );
			}
		    }
		}

		if ( end == (int) t.length() )
		    break;
		start = end + tx.matchedLength();
	    }

	    /*
	      Compute the column widths and fill in the cell widths.
	    */
	    QMemArray<int> columnPercents( numCols );
	    columnPercents.fill( 0 );
	    computeTableColumnWidths( cells, &columnPercents );

	    c = cells.begin();
	    while ( c != cells.end() ) {
	        for ( int i = 0; i < (*c).colSpan; i++ )
		    (*c).percent += columnPercents[(*c).col + i];
		++c;
	    }

	    /*
	      Generate LaTeX code for table cells.
	    */
	    QString laTeX;
	    laTeX += QString( "\n\n\\noindent\\begin{tabularx}"
			      "{0.98\\linewidth}{" );
	    for ( int i = 0; i < numCols - 1; i++ )
		laTeX += QString( "|%1p{%2\\linewidth}" )
			 .arg( tableDataFmt )
			 .arg( percentToDouble(columnPercents[i]) );
	    laTeX += QString( "|%1X|}\n\\hline\n" ).arg( tableDataFmt );

	    int row = 0;
	    int col = 0;

	    c = cells.begin();
	    while ( c != cells.end() ) {
		if ( (*c).row != row ) {
		    laTeX += QString( "\\\\\\hline\n" );
		    row = (*c).row;
		    col = 0;
		}

		while ( col < (*c).col ) {
		    laTeX += QString( "& " );
		    col++;
		}

		if ( (*c).rowSpan > 1 ) {
		    if ( (*c).colSpan > 1 ) {
			qWarning( "Cannot handle cells that are both"
				  " multi-column and multi-row" );
			return;
		    }

		    laTeX += QString( "\\multirow{%1}{%2\\linewidth}{%3}" )
			     .arg( (*c).rowSpan )
			     .arg( percentToDouble((*c).percent) )
			     .arg( (*c).text );
		} else if ( (*c).colSpan > 1 || (*c).header ) {
		    QString flag;
		    if ( (*c).col == numCols - 1 ) {
			flag = QChar( 'X' );
		    } else {
			flag = QString( "p{%1\\linewidth}" )
			       .arg( percentToDouble((*c).percent) );
		    }
		    laTeX += QString( "\\multicolumn{%1}"
				      "{%2%3%4|}{%5}" )
			     .arg( (*c).colSpan )
			     .arg( (*c).col == 0 ? "|" : "" )
			     .arg( (*c).header ? tableHeaderFmt : tableDataFmt )
			     .arg( flag )
			     .arg( (*c).text );
		} else {
		    laTeX += (*c).text;
		}
		++c;
	    }

	    laTeX += QString( "\\\\\\hline\n\\end{tabularx}\n" );
	    html.insert( k, laTeX );
	}
    }
}

static void laTeXifyEntitiesInVerbatim( QString& html )
{
    QRegExp lt( QString("&lt;") );
    QRegExp gt( QString("&gt;") );
    QRegExp unconverted( QString("&(?!amp;)") );
    QRegExp amp( QString("&amp;") );

    html.replace( lt, QString("<") );
    html.replace( gt, QString(">") );
    if ( html.find(unconverted) != -1 )
	qWarning( "Unconverted entities in verbatim" );
    html.replace( amp, QString("&") );
}

static void laTeXifyDivFn( QString& html )
{
    QRegExp fn( QString("class=fn>.*</(?:h3|div)>") );
    fn.setMinimal( TRUE );

    QRegExp funcParen( QString("[a-zA-Z0-9_](?:\\}</a>)?(&nbsp;| )\\(") );
    QRegExp leftParen( QString("\\((?:&nbsp;|~| )") );
    QRegExp rightParen( QString("(?:&nbsp;|~| )\\)") );

    int k = 0;
    while ( (k = html.find(fn, k)) != -1 ) {
	QString t = fn.cap( 0 );

	int ell = t.find( funcParen );
	if ( ell != -1 )
	    t.replace( funcParen.pos(1), funcParen.cap(1).length(),
		       QString("{\\thinspace}") );
	t.replace( leftParen, QString("({\\thinspace}") );
	t.replace( rightParen, QString("\\thinspace)") );
	html.replace( k, fn.matchedLength(), t );
	k++;
    }
}

static QString getAName()
{
    QString n = aname.cap( 1 );
    cleanLabel( n );
    return n;
}

static void laTeXifyANames( QString& html, const QString& chapterLabel )
{
    int k = 0;
    while ( (k = html.find(aname, k)) != -1 ) {
	QString n = compressedLabel( chapterLabel + QChar('#') + getAName() );
	html.replace( k, aname.matchedLength(), QString("\\label{%1}").arg(n) );
	k++;
    }
}

/*
  Replace some <a href>s by a bracketed page number or book title:
  those in "see also"s mostly.
*/
static void laTeXifyAHrefs( QString& html, const QString& chapterLabel )
{
    QRegExp seeAlso( QString(
	"\n(?:See (?:also|the)|Inherit(?:s|ed by)|Reimplemented from).*\n\n") );
    QRegExp ahref( QString("<\\s*a\\s+href\\s*=") );
    seeAlso.setMinimal( TRUE );

    int k = 0;
    while ( (k = html.find(seeAlso, k)) != -1 ) {
	QString t = seeAlso.cap( 0 );
	int oldLen = html.length();

	int ell = 0;
	while ( (ell = t.find(ahref, ell)) != -1 ) {
	    laTeXifyAHref( html, k + ell, chapterLabel );
	    t = html.mid( k, t.length() + (html.length() - oldLen) );
	    oldLen = html.length();
	    ell++;
	}
	k++;
    }
}

static void laTeXifyImages( QString& html )
{
    QRegExp img( QString(
	"<\\s*[iI][mM][gG][^>]+[sS][rR][cC]\\s*=\\s*([^> \n\t]+)[^>]*>") );

    int k = 0;
    while ( (k = html.find(img, k)) != -1 ) {
	QString src = img.cap( 1 );
	if ( src.startsWith(QChar('"')) || src.startsWith(QChar('\'')) )
	    src = src.mid( 1, src.length() - 2 );
	src.replace( QRegExp(QString("\\\\")), QString::null );
	if ( fileContents(qdocOutputDir + src).isEmpty() )
	    return;

	if ( src.endsWith(QString(".png")) || src.endsWith(QString(".jpg")) ) {
	    QString base = src.left( src.length() - 4 );
	    QString dst = base + ".eps";
	    if ( system(QString("convert %1 %2")
			.arg(qdocOutputDir + src)
			.arg(qdocOutputDir + dst).latin1()) == 0 ) {
		html.replace( k, img.matchedLength(),
			      QString("\\includegraphics[scale=0.45]{%1}")
			      .arg(qdocOutputDir + base) );
	    } else {
		qWarning( "Problem with generation of '%s'", dst.latin1() );
	    }
	} else {
	    qWarning( "Cannot handle non-PNG or non-JPG image '%s'",
		      src.latin1() );
	    return;
	}
	k++;
    }
}

static void sloppify( QString& html )
{
    QRegExp sloppifiable( QString(
	"\n\n((?:See also|Examples|Inherited by).*)(?=\n\n)") );
    sloppifiable.setMinimal( TRUE );

    extendedReplace( html, sloppifiable, QString(
	"\n\n"
	"{\\raggedright\\hyphenpenalty=10000\n"
	"%1\n\n"
	"}") );
}

static bool laTeXify( QString& html,
		      const QString& chapterLabel = QString::null )
{
    QStringList preformattedTexts;

    QRegExp comment( QString("<!--.*-->") );
    comment.setMinimal( TRUE );

    QRegExp em( QString(
	"<\\s*([eE][mM]|[iI]|[vV][aA][rR])\\s*>(.*)<\\s*/\\s*\\1\\s*>") );
    em.setMinimal( TRUE );

    QRegExp b( QString("<\\s*([b|B])\\s*>(.*)<\\s*/\\s*\\1\\s*>") );
    b.setMinimal( TRUE );

    QRegExp tt( QString(
	"<\\s*([tT][tT]|[cC][oO][dD][eE])\\s*>(.*)<\\s*/\\s*\\1\\s*>") );
    tt.setMinimal( TRUE );

    QRegExp u( QString(
	"<\\s*[uU]\\s*>(.*)<\\s*/\\s*[uU]\\s*>") );
    u.setMinimal( TRUE );

    QRegExp pre( QString(
	"<\\s*[pP][rR][eE]\\s*>(.*)<\\s*/\\s*[pP][rR][eE]\\s*>") );
    pre.setMinimal( TRUE );

    QRegExp bigRed( QString(
	"<center><font color=\"red\"><b>(\\s*)(.*)</b><br>(\\s*)(.*)"
	"</font></center>") );
    bigRed.setMinimal( TRUE );

    QRegExp center( QString(
	"<\\s*([cC][eE][nN][tT][eE][rR])\\s*>(.*)<\\s*/\\s*\\1\\s*>") );
    center.setMinimal( TRUE );

    QRegExp index( QString("<!-- index (.*) -->") );
    index.setMinimal( TRUE );

    QRegExp toc( QString("<!-- toc -->.*<!-- endtoc -->") );
    toc.setMinimal( TRUE );

    QRegExp preMarker( QString("<premarker>") );

    QRegExp parasiteSpacesAtEnd( QString("[ \t]+\n") );
    QRegExp parag( QString("<\\s*[pP]\\b[^>]*>\\s*") );
    QRegExp more( QString("<a href=\"#details\">More...</a>") );
    QRegExp listOfAllMemberFunctions( QString(
	"<a href=\"[^\"]*-members.html\">List of all member functions.</a>") );
    QRegExp blankLines( QString("\n{3,}") );
    QRegExp backslashable( QString("[#$_{}%]") );
    QRegExp caret( QString("\\^") );
    QRegExp tilde( QChar('~') );
    QRegExp backslash( QString("\\\\") );
    QRegExp tag( QString("<[^<>]*>") );
    QRegExp dash( QString("\\s+--\\s+") );
    QRegExp minusMinus( QString("--") );

    int k = 0;

    k = html.find( QString("<h1") );
    if ( k != -1 )
	html = html.mid( k );

    k = html.find( QString("<!-- eof -->") );
    if ( k != -1 )
	html.truncate( k );

    k = 0;
    while ( (k = html.find(pre, k)) != -1 ) {
	QString t = pre.cap( 1 );
	while ( t.length() > 0 && t[t.length() - 1].isSpace() )
	    t.truncate( t.length() - 1 );
	laTeXifyEntitiesInVerbatim( t );
	preformattedTexts.append( untabified(t) );
	html.replace( k, pre.matchedLength(), QString("<premarker>") );
	k++;
    }

    html.replace( more, QString::null );
    html.replace( listOfAllMemberFunctions, QString::null );
    html.replace( parag, QString("\n\n") );
    html.replace( backslash, QString("&backslash;") );
    extendedReplace( html, backslashable, QString("\\%0") );

    extendedReplace( html, index, QString("\\index{%1}") );
    html.replace( toc, QString::null );
    html.replace( comment, QString::null );
    html.replace( dash, QString("-%\n-%\n-") );
    html.replace( minusMinus, QString("{-}{-}") );
    html.replace( QRegExp(QString("-%\n-%\n-")), QString(" --- ") );

    indexify( html );

    extendedReplace( html, bigRed, QString(
	"\\begin{center}\n"
	"\\begin{tabular}{|@{\\enskip}c@{\\enskip}|}\n"
	"\\hline\n"
	"\\textbf{%2}\\\\\n"
	"%4\\\\\n"
	"\\hline\n"
	"\\end{tabular}\n"
	"\\end{center}\n") );
    extendedReplace( html, em, QString("\\emph{%2}") );
    extendedReplace( html, b, QString("\\textbf{%2}") );
    extendedReplace( html, tt, QString("\\texttt{%2}") );
    extendedReplace( html, u, QString("\\underline{%1}") );
    extendedReplace( html, center, QString(
	"\\begin{center}\n"
	"%2\n"
	"\\end{center}\n") );
    moveANameAfterHeading( html );
    laTeXifyDivFn( html );
    if ( !laTeXifyHeadings(html, chapterLabel) )
	return FALSE;
    laTeXifyLists( html );
    html.replace( parasiteSpacesAtEnd, QChar('\n') );
    html.replace( blankLines, QString("\n\n") );
    html = html.stripWhiteSpace();
    html += QString( "\n\n" );
    html.replace( caret, QString("{\\char`\\^}") );
    html.replace( tilde, QString("{\\char`\\~}") );
    laTeXifyEntities( html );
    laTeXifyTables( html );
    laTeXifyANames( html, chapterLabel );
    laTeXifyAHrefs( html, chapterLabel );
    laTeXifyImages( html );
    sloppify( html );

    k = 0;
    while ( !preformattedTexts.isEmpty() ) {
	k = html.find( preMarker, k );
	if ( k == -1 ) {
	    qWarning( "Internal error at %s:%d", __FILE__, __LINE__ );
	    return FALSE;
	}
	html.replace( k, preMarker.matchedLength(),
		      QString("\\begin{verbatim}%1\\end{verbatim}")
		      .arg(preformattedTexts.first()) );
	k += preformattedTexts.first().length();
	preformattedTexts.remove( preformattedTexts.begin() );
    }
    if ( html.find(pre, k) != -1 ) {
	qWarning( "Internal error at %s:%d", __FILE__, __LINE__ );
	return FALSE;
    }

    html.replace( tag, QString::null );
    return TRUE;
}

static void emitTableOfContents( int pos )
{
    QString toc;

    toc += QString( "\\chapter*{\\contentsname}\n"
		    "\\markboth{\\contentsname}"
		    "{\\contentsname}\n\n" );
    /*
      \dottedtocline is a synonym for \@dottedtocline, thanks to our
      prologue. \@dottedtocline is an internal LaTeX2e command. We
      should not use it, but we do.
    */
    for ( int i = 0; i < (int) (*yyCurrentBook).chapters.count(); i++ ) {
	QString fn = *(*yyCurrentBook).chapters.at( i );
	int slash = fn.findRev( QChar('/') );

	toc += QString(
	    "\\dottedtocline{0}{0em}{1.5em}{\\bf %1}{\\bf \\pageref{%2}}\n" )
	    .arg( *(*yyCurrentBook).chapterTitles.at(i) )
	    .arg( compressedLabel(fn.mid(slash + 1)) );
    }
    toc += QString( "\\dottedtocline{0}{0em}{1.5em}{\\bf \\indexname}"
		    "{\\bf \\pageref{index}}\n" );
    toc += QString( "\\newpage\n\n" );

    yyOut.insert( pos, toc );
}

static bool emitChapter( const QString& html, const QString& label )
{
    qDebug( "Generating chapter '%s'", label.latin1() );
    QString t = html;
    if ( !laTeXify(t, label) )
	return FALSE;
    yyOut += QString( "\n% Generated from '%1'\n\n" ).arg( label );
    yyOut += t;
    return TRUE;
}

static bool emitBook( bool a4, bool letter, bool twoSided )
{
    qDebug( "Generating book '%s'", (*yyCurrentBook).title.latin1() );
    yyOut = QString( laTeXPrologue )
	    .arg( (*yyCurrentBook).title )
	    .arg( (*yyCurrentBook).title )
	    .arg( QDateTime::currentDateTime().toString() )
	    .arg( a4 ? "[a4paper]" : letter ? "[letterpaper]" : "" )
	    .arg( twoSided ? "book" : "report" )
	    .arg( twoSided ? "10mm" : "5mm" );

    int tableOfContentsPos = yyOut.length();

    QStringList::ConstIterator c = (*yyCurrentBook).chapters.begin();
    while ( c != (*yyCurrentBook).chapters.end() ) {
	int slash = (*c).findRev( QChar('/') );
	QString html = fileContents( *c );
	if ( html.isEmpty() )
	    return FALSE;
	if ( !emitChapter(html, (*c).mid(slash + 1)) )
	    return FALSE;
	++c;
    }
    yyOut += QString( laTeXEpilogue );

    QFile f( (*yyCurrentBook).fileName );
    if ( !f.open(IO_WriteOnly) ) {
	qWarning( "qdoc2latex error: Cannot open file '%s' for writing: %s",
		  (*yyCurrentBook).fileName.latin1(), strerror(errno) );
	return FALSE;
    }

    emitTableOfContents( tableOfContentsPos );

    QTextStream t( &f );
    t << yyOut;
    f.close();
    return TRUE;
}

static bool findANames()
{
    QStringList::ConstIterator c = (*yyCurrentBook).chapters.begin();
    while ( c != (*yyCurrentBook).chapters.end() ) {
	int slash = (*c).findRev( QChar('/') );
	QString html = fileContents( *c );
	if ( html.isEmpty() )
	    return FALSE;
	QString n = compressedLabel( (*c).mid(slash + 1) );
	(*yyCurrentBook).links.insert( n, 0 );

	int k = 0;
	while ( (k = html.find(aname, k)) != -1 ) {
	    n = compressedLabel( (*c).mid(slash + 1) + QChar('#') +
				 getAName() );
	    (*yyCurrentBook).links.insert( n, 0 );
	    k++;
	}
	++c;
    }
    return TRUE;
}

static void checkUnusedQdocFiles()
{
    /*
      qbubble-members.html and qbubble-h.html are mostly irrelevant.
      linguist-manual-2-3.html is a left-over from a previous run of
      qdoc with --friendly=no.
    */
    QRegExp irrelevantFiles( QString(
	".*-(?:members|h|[0-9]+(?:-[0-9]+)+)\\.html") );
    QStringList::Iterator f;

    f = fileNameList.begin();

    FILE *logfile = fopen( "qdoc2latex.log", "w" );
    if ( logfile == 0 ) {
	qWarning( "Cannot open 'qdoc2latex.log' for writing: %s",
		  strerror(errno) );
	return;
    }

    int numUsed = 0;
    int numAll = 0;

    while ( f != fileNameList.end() ) {
	if ( !irrelevantFiles.exactMatch(*f) ) {
	    if ( yyUsedQdocFiles.contains(*f) ) {
		numUsed++;
	    } else {
		fprintf( logfile, "Unused '%s'\n", (*f).latin1() );
	    }
	    numAll++;
	}
	++f;
    }
    qWarning( "Used %d of %d potentially relevant file%s", numUsed, numAll,
	      numAll == 1 ? "" : "s" );
    qWarning( "(See 'qdoc2latex.log' for details)" );
    fclose( logfile );
}

int main( int argc, char **argv )
{
    bool a4 = FALSE;
    bool letter = FALSE;
    bool twoSided = FALSE;
    int i = 1;

    while ( i < argc ) {
	if ( strcmp(argv[i], "-a4") == 0 ) {
	    a4 = TRUE;
	    letter = FALSE;
	} else if ( strcmp(argv[i], "-letter") == 0 ) {
	    letter = TRUE;
	    a4 = FALSE;
	} else if ( strcmp(argv[i], "-2sided") == 0 ) {
	    twoSided = TRUE;
	} else {
	    break;
	}
	i++;
    }

    qdocOutputDir = QString( argv[i++] );
    if ( !qdocOutputDir.endsWith(QChar('/')) )
	qdocOutputDir += QChar( '/' );

    if ( i >= argc ) {
	qWarning( "Usage: qdoc2latex [-a4|-letter] [-2sided] qdoc-output-dir"
		  " book-files..." );
	return 1;
    }

    QDir dir( qdocOutputDir );
    if ( dir.exists() ) {
	dir.setSorting( QDir::Name );

	dir.setNameFilter( QString("*.html") );
	dir.setFilter( QDir::Files );
	fileNameList = dir.entryList();
    }

    while ( i < argc ) {
	QString fn( argv[i++] );

	if ( fn.endsWith(QString(".q2l")) ) {
	    yyIn = fileContents( fn );
	    if ( yyIn.isEmpty() )
		continue;

	    stripComments();

	    QString outFileName( fn );
	    outFileName.truncate( outFileName.length() - 4 );
	    outFileName += QString( ".tex" );

	    yyCurrentBook = yyBooks.append( Book(outFileName) );
	    yyPos = 0;

	    skipSpaces();
	    while ( !eoi() ) {
		if ( !matchCommand() ) {
		    qWarning( "%s:%d: Parse error", fn.latin1(),
			      yyIn.left(yyPos).contains(QChar('\n')) + 1 );
		    return 1;
		}
		skipSpaces();
	    }

	    if ( !findANames() )
		return 1;
	} else if ( fn.endsWith(".book") ) {
	    QString outFileName = fn;
	    outFileName.truncate( outFileName.length() - 5 );
	    outFileName += QString( ".tex" );
	    yyCurrentBook = yyBooks.append( Book(outFileName) );
	    if ( !processQbook() )
		return 1;
	} else {
	    qWarning( "File '%s' has an unknown extension", fn.latin1() );
	}
    }

    yyCurrentBook = yyBooks.begin();
    while ( yyCurrentBook != yyBooks.end() ) {
	if ( !emitBook(a4, letter, twoSided) )
	    return 1;
	yyCurrentBook++;
    }

    checkUnusedQdocFiles();

    return 0;
}
