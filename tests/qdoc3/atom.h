/*
  atom.h
*/

#ifndef ATOM_H
#define ATOM_H

#include <qstring.h>

class Atom
{
public:
    enum Type { AbstractLeft, AbstractRight, BaseName, BriefLeft, BriefRight, C,
		CaptionLeft, CaptionRight, Code, FootnoteLeft, FootnoteRight,
		FormatLeft, FormatRight, GeneratedList, Image, Link, LinkNode,
		ListLeft, ListItemNumber, ListItemLeft, ListItemRight,
		ListRight, Nop, ParagraphLeft, ParagraphRight, QuotationLeft,
		QuotationRight, RawFormat, RawString, SectionLeft, SectionRight,
		SectionHeadingLeft, SectionHeadingRight, SidebarLeft,
		SidebarRight, String, TableLeft, TableRight, TableOfContents,
		Target, UnknownCommand, Last = UnknownCommand };

    Atom( Type type, const QString& string = "" )
	: nex( 0 ), typ( type ), str( string ) { }
    Atom( Atom *prev, Type type, const QString& string = "" )
	: nex( prev->nex ), typ( type ), str( string ) { prev->nex = this; }

    void appendChar( QChar ch ) { str += ch; }
    void appendString( const QString& string ) { str += string; }
    void chopString() { str.truncate( str.length() - 1 ); }
    Atom *next() { return nex; }

    const Atom *next() const { return nex; }
    Type type() const { return typ; }
    QString typeString() const;
    const QString& string() const { return str; }

private:
    Atom *nex;
    Type typ;
    QString str;
};

#define ATOM_FORMAT_BOLD        "bold"
#define ATOM_FORMAT_INDEX       "index"
#define ATOM_FORMAT_ITALIC      "italic"
#define ATOM_FORMAT_LINK        "link"
#define ATOM_FORMAT_SUBSCRIPT   "subscript"
#define ATOM_FORMAT_SUPERSCRIPT "superscript"
#define ATOM_FORMAT_TELETYPE    "teletype"
#define ATOM_FORMAT_UNDERLINE   "underline"

#define ATOM_LIST_BULLET        "bullet"
#define ATOM_LIST_LOWERALPHA    "loweralpha"
#define ATOM_LIST_LOWERROMAN    "lowerroman"
#define ATOM_LIST_NUMERIC       "numeric"
#define ATOM_LIST_UPPERALPHA    "upperalpha"
#define ATOM_LIST_UPPERROMAN    "upperroman"

#endif
