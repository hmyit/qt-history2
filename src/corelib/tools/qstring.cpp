/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qstringlist.h"
#include "qregexp.h"
#include "qunicodetables_p.h"
#ifndef QT_NO_TEXTCODEC
#include <qtextcodec.h>
#endif
#include <qdatastream.h>
#include <qlist.h>
#include "qlocale.h"
#include "qlocale_p.h"
#include "qstringmatcher.h"
#include "qtools_p.h"
#include "qhash.h"
#include "qdebug.h"

#ifdef Q_OS_MAC
#include <private/qcore_mac_p.h>
#endif

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef truncate
#undef truncate
#endif

#include "qchar.cpp"
#include "qstringmatcher.cpp"

#ifndef LLONG_MAX
#define LLONG_MAX qint64_C(9223372036854775807)
#endif
#ifndef LLONG_MIN
#define LLONG_MIN (-LLONG_MAX - qint64_C(1))
#endif
#ifndef ULLONG_MAX
#define ULLONG_MAX quint64_C(18446744073709551615)
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_TEXTCODEC
QTextCodec *QString::codecForCStrings;
#endif

#ifdef QT3_SUPPORT
static QHash<void *, QByteArray> *asciiCache = 0;
#endif

static int ucstricmp(const ushort *a, const ushort *ae, const ushort *b, const ushort *be)
{
    if (a == b)
        return 0;
    if (a == 0)
        return 1;
    if (b == 0)
        return -1;

    const ushort *e = ae;
    if (be - b < ae - a)
        e = a + (be - b);

    uint alast = 0;
    uint blast = 0;
    while (a != e) {
//         qDebug() << hex << alast << blast;
//         qDebug() << hex << "*a=" << *a << "alast=" << alast << "folded=" << foldCase (*a, alast);
//         qDebug() << hex << "*b=" << *b << "blast=" << blast << "folded=" << foldCase (*b, blast);
        int diff = foldCase(*a, alast) - foldCase(*b, blast);
        if ((diff))
            return diff;
        ++a;
        ++b;
    }
    if (a == ae) {
        if (b == be)
            return 0;
        return -1;
    }
    return 1;
}

static int ucstricmp(const ushort *a, const ushort *ae, const uchar *b)
{
    if (a == 0) {
        if (b == 0)
            return 0;
        return 1;
    }
    if (b == 0)
        return -1;

    while (a != ae && *b) {
        int diff = foldCase(*a) - foldCase(*b);
        if ((diff))
            return diff;
        ++a;
        ++b;
    }
    if (a == ae) {
        if (!*b)
            return 0;
        return -1;
    }
    return 1;
}


static int ucstrcmp(const QChar *a, int alen, const QChar *b, int blen)
{
    if (a == b)
        return 0;
    int l = qMin(alen, blen);
    while (l-- && *a == *b)
        a++,b++;
    if (l == -1)
        return (alen-blen);
    return a->unicode() - b->unicode();
}

inline int ucstrcmp(const QString &as, const QString &bs)
{
    return ucstrcmp(as.unicode(), as.size(), bs.unicode(), bs.size());
}

inline int ucstrcmp(const QStringRef &as, const QStringRef &bs)
{
    return ucstrcmp(as.unicode(), as.size(), bs.unicode(), bs.size());
}



static int ucstrncmp(const QChar *a, const QChar *b, int l)
{
    while (l-- && *a == *b)
        a++,b++;
    if (l==-1)
        return 0;
    return a->unicode() - b->unicode();
}


static int ucstrnicmp(const ushort *a, const ushort *b, int l)
{
    return ucstricmp(a, a + l, b, b + l);
}


inline bool qIsUpper(char ch)
{
    return ch >= 'A' && ch <= 'Z';
}

inline bool qIsDigit(char ch)
{
    return ch >= '0' && ch <= '9';
}

inline char qToLower(char ch)
{
    if (ch >= 'A' && ch <= 'Z')
        return ch - 'A' + 'a';
    else
        return ch;
}

const QString::Null QString::null = QString::Null();

/*!
  \macro QT_NO_CAST_FROM_ASCII
  \relates QString

  Disables automatic conversions from 8-bit strings (char *) to unicode QStrings

  \sa QT_NO_CAST_TO_ASCII
*/

/*!
  \macro QT_NO_CAST_TO_ASCII
  \relates QString

  disables automatic conversion from QString to ASCII 8-bit strings (char *)

  \sa QT_NO_CAST_FROM_ASCII
*/

/*!
  \macro QT_ASCII_CAST_WARNINGS
  \internal
  \relates QString

  This macro can be defined to force a warning whenever a function is
  called that automatically converts between unicode and 8-bit encodings.

  Note: This only works for compilers that support warnings for
  deprecated API.

  \sa QT_NO_CAST_TO_ASCII, QT_NO_CAST_FROM_ASCII
*/

/*!
    \class QCharRef
    \reentrant
    \brief The QCharRef class is a helper class for QString.

    \internal

    \ingroup text

    When you get an object of type QCharRef, if you can assign to it,
    the assignment will apply to the character in the string from
    which you got the reference. That is its whole purpose in life.
    The QCharRef becomes invalid once modifications are made to the
    string: if you want to keep the character, copy it into a QChar.

    Most of the QChar member functions also exist in QCharRef.
    However, they are not explicitly documented here.

    \sa QString::operator[]() QString::at() QChar
*/

/*!
    \class QString
    \reentrant

    \brief The QString class provides a Unicode character string.

    \ingroup tools
    \ingroup shared
    \ingroup text
    \mainclass
    \reentrant

    QString stores a string of 16-bit \l{QChar}s, where each QChar
    corresponds one Unicode 4.0 character. (Unicode characters
    with code values above 65535 are stored using surrogate pairs,
    i.e., two consecutive \l{QChar}s.)

    \l{Unicode} is an international standard that supports most of
    the writing systems in use today. It is a superset of ASCII and
    Latin-1 (ISO 8859-1), and all the ASCII/Latin-1 characters are
    available at the same code positions.

    Behind the scenes, QString uses \l{implicit sharing}
    (copy-on-write) to reduce memory usage and to avoid the needless
    copying of data. This also helps reduce the inherent overhead of
    storing 16-bit characters instead of 8-bit characters.

    In addition to QString, Qt also provides the QByteArray class to
    store raw bytes and traditional 8-bit '\\0'-terminated strings.
    For most purposes, QString is the class you want to use. It is
    used throughout the Qt API, and the Unicode support ensures that
    your applications will be easy to translate if you want to expand
    your application's market at some point. The two main cases where
    QByteArray is appropriate are when you need to store raw binary
    data, and when memory conservation is critical (e.g. with \l {
    Qtopia Core}).

    \tableofcontents

    \section1 Initializing a String

    One way to initialize a QString is simply to pass a \c{const char
    *} to its constructor. For example, the following code creates a
    QString of size 5 containing the data "Hello":

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::constCharPointer()
    \skipto Hello
    \printline Hello

    QString converts the \c{const char *} data into Unicode using the
    fromAscii() function. By default, fromAscii() treats character
    above 128 as Latin-1 characters, but this can be changed by
    calling QTextCodec::setCodecForCStrings().

    In all of the QString functions that take \c{const char *}
    parameters, the \c{const char *} is interpreted as a classic
    C-style '\\0'-terminated string. It is legal for the \c{const char
    *} parameter to be 0.

    You can also provide string data as an array of \l{QChar}s:

    \skipto Widget::constCharArray()
    \skipto static const
    \printuntil str

    QString makes a deep copy of the QChar data, so you can modify it
    later without experiencing side effects. (If for performance
    reasons you don't want to take a deep copy of the character data,
    use QString::fromRawData() instead.)

    Another approach is to set the size of the string using resize()
    and to initialize the data character per character. QString uses
    0-based indexes, just like C++ arrays. To access the character at
    a particular index position, you can use \l operator[](). On
    non-const strings, \l operator[]() returns a reference to a
    character that can be used on the left side of an assignment. For
    example:

    \skipto Widget::characterReference()
    \skipto QString str
    \printuntil str[3] = QChar(0x03a3)

    For read-only access, an alternative syntax is to use the at()
    function:

    \skipto Widget::atFunction()
    \skipto QString str
    \printuntil }

    The at() function can be faster than \l operator[](), because it
    never causes a \l{deep copy} to occur. Alternatively, use the
    left(), right(), or mid() functions to extract several characters
    at a time.

    A QString can embed '\\0' characters (QChar::null). The size()
    function always returns the size of the whole string, including
    embedded '\\0' characters.

    After a call to the resize() function, newly allocated characters
    have undefined values. To set all the characters in the string to
    a particular value, use the fill() function.

    QString provides dozens of overloads designed to simplify string
    usage. For example, if you want to compare a QString with a string
    literal, you can write code like this and it will work as expected:

    \skipto Widget::stringLiteral()
    \skipto QString str
    \printuntil }

    You can also pass string literals to functions that take QStrings
    as arguments, invoking the QString(const char *)
    constructor. Similarly, you can pass a QString to a function that
    takes a \c{const char *} argument using the \l qPrintable() macro
    which returns the given QString as a \c{const char *}. This is
    equivalent to calling <QString>.toAscii().constData().

    \section1 Manipulating String Data

    QString provides the following basic functions for modifying the
    character data: append(), prepend(), insert(), replace(), and
    remove(). For example:

    \skipto Widget::modify()
    \skipto QString str
    \printuntil str.replace(5, 3, "&")

    If you are building a QString gradually and know in advance
    approximately how many characters the QString will contain, you
    can call reserve(), asking QString to preallocate a certain amount
    of memory. You can also call capacity() to find out how much
    memory QString actually allocated.

    The replace() and remove() functions' first two arguments are the
    position from which to start erasing and the number of characters
    that should be erased.  If you want to replace all occurrences of
    a particular substring with another, use one of the two-parameter
    replace() overloads.

    A frequent requirement is to remove whitespace characters from a
    string ('\\n', '\\t', ' ', etc.). If you want to remove whitespace
    from both ends of a QString, use the trimmed() function. If you
    want to remove whitespace from both ends and replace multiple
    consecutive whitespaces with a single space character within the
    string, use simplified().

    If you want to find all occurrences of a particular character or
    substring in a QString, use the indexOf() or lastIndexOf()
    functions. The former searches forward starting from a given index
    position, the latter searches backward. Both return the index
    position of the character or substring if they find it; otherwise,
    they return -1.  For example, here's a typical loop that finds all
    occurrences of a particular substring:

    \skipto Widget::index()
    \skipto QString str
    \printuntil }

    QString provides many functions for converting numbers into
    strings and strings into numbers. See the arg() functions, the
    setNum() functions, the number() static functions, and the
    toInt(), toDouble(), and similar functions.

    To get an upper- or lowercase version of a string use toUpper() or
    toLower().

    Lists of strings are handled by the QStringList class. You can
    split a string into a list of strings using the split() function,
    and join a list of strings into a single string with an optional
    separator using QStringList::join(). You can obtain a list of
    strings from a string list that contain a particular substring or
    that match a particular QRegExp using the QStringList::find()
    function.
:
    \section1 Querying String Data

    If you want to see if a QString starts or ends with a particular
    substring use startsWith() or endsWith(). If you simply want to
    check whether a QString contains a particular character or
    substring, use the contains() function. If you want to find out
    how many times a particular character or substring occurs in the
    string, use count().

    QStrings can be compared using overloaded operators such as \l
    operator<(), \l operator<=(), \l operator==(), \l operator>=(),
    and so on.  Note that the comparison is based exclusively on the
    numeric Unicode values of the characters. It is very fast, but is
    not what a human would expect; the QString::localeAwareCompare()
    function is a better choice for sorting user-interface strings.

    To obtain a pointer to the actual character data, call data() or
    constData(). These functions return a pointer to the beginning of
    the QChar data. The pointer is guaranteed to remain valid until a
    non-const function is called on the QString.

    \section1 Converting Between 8-Bit Strings and Unicode Strings

    QString provides the following four functions that return a
    \c{const char *} version of the string as QByteArray: toAscii(),
    toLatin1(), toUtf8(), and toLocal8Bit().

    \list
    \o toAscii() returns an ASCII encoded 8-bit string.
    \o toLatin1() returns a Latin-1 (ISO 8859-1) encoded 8-bit string.
    \o toUtf8() returns a UTF-8 encoded 8-bit string. UTF-8 is a
       superset of ASCII that supports the entire Unicode character
       set through multibyte sequences.
    \o toLocal8Bit() returns an 8-bit string using the system's local
       encoding.
    \endlist

    To convert from one of these encodings, QString provides
    fromAscii(), fromLatin1(), fromUtf8(), and fromLocal8Bit(). Other
    encodings are supported through the QTextCodec class.

    As mentioned above, QString provides a lot of functions and
    operators that make it easy to interoperate with \c{const char *}
    strings. But this functionality is a double-edged sword: It makes
    QString more convenient to use if all strings are ASCII or
    Latin-1, but there is always the risk that an implicit conversion
    from or to \c{const char *} is done using the wrong 8-bit
    encoding. To minimize these risks, you can turn off these implicit
    conversions by defining the following two preprocessor symbols:

    \list
    \o \c QT_NO_CAST_FROM_ASCII disables automatic conversions from
       ASCII to Unicode.
    \o \c QT_NO_CAST_TO_ASCII disables automatic conversion from QString
       to ASCII.
    \endlist

    One way to define these preprocessor symbols globally for your
    application is to add the following entry to your
    \l{qmake Project Files}{qmake project file}:

    \code
        DEFINES += QT_NO_CAST_FROM_ASCII \
                   QT_NO_CAST_TO_ASCII
    \endcode

    You then need to explicitly call fromAscii(), fromLatin1(),
    fromUtf8(), or fromLocal8Bit() to construct a QString from an
    8-bit string, or use the lightweight QLatin1String class, for
    example:

    \code
        QString url = QLatin1String("http://www.unicode.org/");
    \endcode

    Similarly, you must call toAscii(), toLatin1(), toUtf8(), or
    toLocal8Bit() explicitly to convert the QString to an 8-bit
    string.  (Other encodings are supported through the QTextCodec
    class.)

    \table 100 %
    \row
    \o
    \section1 Note for C Programmers

    Due to C++'s type system and the fact that QString is
    \l{implicitly shared}, QStrings may be treated like \c{int}s or
    other basic types. For example:

    \skipto boolToString(bool b)
    \printuntil }

    The \c result variable, is a normal variable allocated on the
    stack. When \c return is called, and because we're returning by
    value, the copy constructor is called and a copy of the string is
    returned. No actual copying takes place thanks to the implicit
    sharing.

    \endtable

    \section1 Distinction Between Null and Empty Strings

    For historical reasons, QString distinguishes between a null
    string and an empty string. A \e null string is a string that is
    initialized using QString's default constructor or by passing
    (const char *)0 to the constructor. An \e empty string is any
    string with size 0. A null string is always empty, but an empty
    string isn't necessarily null:

    \quotefromfile snippets/qstring/main.cpp
    \skipto QString().isNull()
    \printuntil QString("abc").isEmpty()

    All functions except isNull() treat null strings the same as empty
    strings. For example, toAscii().constData() returns a pointer to a
    '\\0' character for a null string (\e not a null pointer), and
    QString() compares equal to QString(""). We recommend that you
    always use the isEmpty() function and avoid isNull().

    \sa fromRawData(), QChar, QLatin1String, QByteArray, QStringRef
*/

/*!
    \enum QString::SplitBehavior

    This enum specifies how the split() function should behave with
    respect to empty strings.

    \value KeepEmptyParts  If a field is empty, keep it in the result.
    \value SkipEmptyParts  If a field is empty, don't include it in the result.

    \sa split()
*/

QString::Data QString::shared_null = { Q_BASIC_ATOMIC_INITIALIZER(1),
                                       0, 0, shared_null.array, 0, 0, 0, 0, 0, 0, {0} };
QString::Data QString::shared_empty = { Q_BASIC_ATOMIC_INITIALIZER(1),
                                        0, 0, shared_empty.array, 0, 0, 0, 0, 0, 0, {0} };

int QString::grow(int size)
{
    return qAllocMore(size * sizeof(QChar), sizeof(Data)) / sizeof(QChar);
}

/*! \typedef QString::ConstIterator

    \internal

    Qt-style synonym for QString::const_iterator.
*/

/*! \typedef QString::Iterator

    \internal

    Qt-style synonym for QString::iterator.
*/

/*! \typedef QString::const_iterator

    \internal

    The QString::const_iterator typedef provides an STL-style const
    iterator for QString.

    \sa QString::iterator
*/

/*! \typedef QString::iterator

    \internal

    The QString::iterator typedef provides an STL-style non-const
    iterator for QString.

    \sa QString::const_iterator
*/

/*! \fn QString::iterator QString::begin()

    \internal
*/

/*! \fn QString::const_iterator QString::begin() const

    \internal
*/

/*! \fn QString::const_iterator QString::constBegin() const

    \internal
*/

/*! \fn QString::iterator QString::end()

    \internal
*/

/*! \fn QString::const_iterator QString::end() const

    \internal
*/

/*! \fn QString::const_iterator QString::constEnd() const

    \internal
*/

/*!
    \fn QString::QString()

    Constructs a null string. Null strings are also empty.

    \sa isEmpty()
*/

/*! \fn QString::QString(const char *str)

    Constructs a string initialized with the ASCII string \a str. The
    given const char pointer is converted to Unicode using the
    fromAscii() function.

    You can disable this constructor by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.

    \sa fromAscii(), fromLatin1(), fromLocal8Bit(), fromUtf8()
*/

/*! \fn QString QString::fromStdString(const std::string &str)

    Returns a copy of the \a str string. The given string is converted
    to Unicode using the fromAscii() function.

    This constructor is only available if Qt is configured with STL
    compatibility enabled.

    \sa  fromAscii(), fromLatin1(), fromLocal8Bit(), fromUtf8()
*/

/*! \fn QString QString::fromStdWString(const std::wstring &str)

    Returns a copy of the \a str string. The given string is assumed
    to be encoded in utf16 if the size of wchar_t is 2 bytes (e.g. on
    windows) and ucs4 if the size of wchar_t is 4 bytes (most Unix
    systems).

    This method is only available if Qt is configured with STL
    compatibility enabled.

    \sa fromUtf16(), fromLatin1(), fromLocal8Bit(), fromUtf8(), fromUcs4()
*/

/*!
    \since 4.2

    Returns a copy of the \a string string encoded in ucs4.

    If \a size is -1 (the default), the \a string has to be 0 terminated.

    \sa fromUtf16(), fromLatin1(), fromLocal8Bit(), fromUtf8(), fromUcs4(), fromStdWString()
*/
QString QString::fromWCharArray(const wchar_t *string, int size)
{
    if (sizeof(wchar_t) == sizeof(QChar)) {
        return fromUtf16((ushort *)string, size);
    } else {
        return fromUcs4((uint *)string, size);
    }
}

/*! \fn std::wstring QString::toStdWString() const

    Returns a std::wstring object with the data contained in this
    QString. The std::wstring is encoded in utf16 on platforms where
    wchar_t is 2 bytes wide (e.g. windows) and in ucs4 on platforms
    where wchar_t is 4 bytes wide (most Unix systems).

    This operator is mostly useful to pass a QString to a function
    that accepts a std::wstring object.

    This operator is only available if Qt is configured with STL
    compatibility enabled.

    \sa utf16(), toAscii(), toLatin1(), toUtf8(), toLocal8Bit()
*/

/*!
  \since 4.2

  Fills the \a array with the data contained in this QString object.
  The array is encoded in utf16 on platforms where
  wchar_t is 2 bytes wide (e.g. windows) and in ucs4 on platforms
  where wchar_t is 4 bytes wide (most Unix systems).

  \a array has to be allocated by the caller and contain enough space to
  hold the complete string (allocating the array with the same length as the
  string is always sufficient).

  returns the actual length of the string in \a array.

  \sa utf16(), toUcs4(), toAscii(), toLatin1(), toUtf8(), toLocal8Bit(), toStdWString()
*/
int QString::toWCharArray(wchar_t *array) const
{
    if (sizeof(wchar_t) == sizeof(QChar)) {
        memcpy(array, utf16(), sizeof(wchar_t)*length());
        return length();
    } else {
        wchar_t *a = array;
        const unsigned short *uc = utf16();
        for (int i = 0; i < length(); ++i) {
            uint u = uc[i];
            if (u >= 0xd800 && u < 0xdc00 && i < length()-1) {
                ushort low = uc[i+1];
                if (low >= 0xdc00 && low < 0xe000) {
                    ++i;
                    u = (u - 0xd800)*0x400 + (low - 0xdc00) + 0x10000;
                }
            }
            *a = wchar_t(u);
            ++a;
        }
        return a - array;
    }
}

/*! \fn QString::QString(const QString &other)

    Constructs a copy of \a other.

    This operation takes \l{constant time}, because QString is
    \l{implicitly shared}. This makes returning a QString from a
    function very fast. If a shared instance is modified, it will be
    copied (copy-on-write), and that takes \l{linear time}.

    \sa operator=()
*/

/*!
    Constructs a string initialized with the first \a size characters
    of the QChar array \a unicode.

    QString makes a deep copy of the string data.
*/
QString::QString(const QChar *unicode, int size)
{
   if (!unicode) {
        d = &shared_null;
        d->ref.ref();
    } else if (size <= 0) {
        d = &shared_empty;
        d->ref.ref();
    } else {
        d = (Data*) qMalloc(sizeof(Data)+size*sizeof(QChar));
        d->ref = 1;
        d->alloc = d->size = size;
        d->clean = d->asciiCache = d->simpletext = d->righttoleft = d->capacity = 0;
        d->data = d->array;
        memcpy(d->array, unicode, size * sizeof(QChar));
        d->array[size] = '\0';
    }
}


/*!
    Constructs a string of the given \a size with every character set
    to \a ch.

    \sa fill()
*/
QString::QString(int size, QChar ch)
{
   if (size <= 0) {
        d = &shared_empty;
        d->ref.ref();
    } else {
        d = (Data*) qMalloc(sizeof(Data)+size*sizeof(QChar));
        d->ref = 1;
        d->alloc = d->size = size;
        d->clean = d->asciiCache = d->simpletext = d->righttoleft = d->capacity = 0;
        d->data = d->array;
        d->array[size] = '\0';
        ushort *i = d->array + size;
        ushort *b = d->array;
        const ushort value = ch.unicode();
        while (i != b)
           *--i = value;
    }
}

/*! \fn QString::QString(const QLatin1String &str)

    Constructs a copy of the Latin-1 string \a str.

    \sa fromLatin1()
*/

/*!
    Constructs a string of size 1 containing the character \a ch.
*/
QString::QString(QChar ch)
{
    d = (Data *)qMalloc(sizeof(Data) + sizeof(QChar));
    d->ref = 1;
    d->alloc = d->size = 1;
    d->clean = d->asciiCache = d->simpletext = d->righttoleft = d->capacity = 0;
    d->data = d->array;
    d->array[0] = ch.unicode();
    d->array[1] = '\0';
}

/*! \fn QString::QString(const QByteArray &ba)

    Constructs a string initialized with the byte array \a ba. The
    given byte array is converted to Unicode using fromAscii(). Stops
    copying at the first 0 character, otherwise copies the entire byte
    array.

    You can disable this constructor by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.

    \sa fromAscii(), fromLatin1(), fromLocal8Bit(), fromUtf8()
*/

/*! \fn QString::QString(const Null &)
    \internal
*/

/*! \fn QString &QString::operator=(const Null &)
    \internal
*/

/*!
  \fn QString::~QString()

    Destroys the string.
*/


/*! \fn void QString::detach()

    \internal
*/

/*! \fn void QString::isDetached() const

    \internal
*/

void QString::free(Data *d)
{
#ifdef QT3_SUPPORT
    if (d->asciiCache) {
        Q_ASSERT(asciiCache);
        asciiCache->remove(d);
    }
#endif
    qFree(d);
}

/*!
    Sets the size of the string to \a size characters.

    If \a size is greater than the current size, the string is
    extended to make it \a size characters long with the extra
    characters added to the end. The new characters are uninitialized.

    If \a size is less than the current size, characters are removed
    from the end.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::resizeFunction()
    \skipto QString s
    \printuntil // s == "Hello???"

    If you want to append a certain number of identical characters to
    the string, use \l operator+=() as follows rather than resize():

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::resizeFunction()
    \skipto QString t
    \printuntil // t == "HelloXXXXXXXXXX"

    If you want to expand the string so that it reaches a certain
    width and fill the new positions with a particular character, use
    the leftJustified() function:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::resizeFunction()
    \skipto QString r
    \printuntil // r == "Hello     "

    \sa truncate(), reserve()
*/

void QString::resize(int size)
{
    if (size <= 0 && !d->capacity) {
        Data *x = &shared_empty;
        x->ref.ref();
        if (!d->ref.deref())
            free(d);
        d = x;
    } else {
        if (d->ref != 1 || size > d->alloc || (!d->capacity && size < d->size && size < d->alloc >> 1))
            realloc(grow(size));
        if (d->alloc >= size) {
            d->size = size;
            if (d->data == d->array) {
                d->array[size] = '\0';
            }
        }
    }
}

/*! \fn int QString::capacity() const

    Returns the maximum number of characters that can be stored in
    the string without forcing a reallocation.

    The sole purpose of this function is to provide a means of fine
    tuning QString's memory usage. In general, you will rarely ever
    need to call this function. If you want to know how many
    characters are in the string, call size().

    \sa reserve(), squeeze()
*/

/*!
    \fn void QString::reserve(int size)

    Attempts to allocate memory for at least \a size characters. If
    you know in advance how large the string will be, you can call
    this function, and if you resize the string often you are likely
    to get better performance. If \a size is an underestimate, the
    worst that will happen is that the QString will be a bit slower.

    The sole purpose of this function is to provide a means of fine
    tuning QString's memory usage. In general, you will rarely ever
    need to call this function. If you want to change the size of the
    string, call resize().

    This function is useful for code that needs to build up a long
    string and wants to avoid repeated reallocation. In this example,
    we want to add to the string until some condition is true, and
    we're fairly sure that size is large enough to make a call to
    reserve() worthwhile:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::reserveFunction()
    \skipto QString result
    \printuntil result.squeeze()

    \sa squeeze(), capacity()
*/

/*!
    \fn void QString::squeeze()

    Releases any memory not required to store the character data.

    The sole purpose of this function is to provide a means of fine
    tuning QString's memory usage. In general, you will rarely ever
    need to call this function.

    \sa reserve(), capacity()
*/

void QString::realloc(int alloc)
{
    if (d->ref != 1 || d->data != d->array) {
        Data *x = static_cast<Data *>(qMalloc(sizeof(Data) + alloc * sizeof(QChar)));
        if (!x)
            return;
        x->size = qMin(alloc, d->size);
        ::memcpy(x->array, d->data, x->size * sizeof(QChar));
        x->array[x->size] = 0;
        x->asciiCache = 0;
        x->ref = 1;
        x->alloc = alloc;
        x->clean = d->clean;
        x->simpletext = d->simpletext;
        x->righttoleft = d->righttoleft;
        x->capacity = d->capacity;
        x->data = x->array;
        if (!d->ref.deref())
            free(d);
        d = x;
    } else {
#ifdef QT3_SUPPORT
        if (d->asciiCache) {
            Q_ASSERT(asciiCache);
            asciiCache->remove(d);
        }
#endif
        Data *x = static_cast<Data *>(qRealloc(d, sizeof(Data) + alloc * sizeof(QChar)));
        if (!x)
            return;
        x->alloc = alloc;
        x->data = x->array;
        d = x;
    }
}

void QString::realloc()
{
    realloc(d->size);
}

void QString::expand(int i)
{
    int sz = d->size;
    resize(qMax(i + 1, sz));
    if (d->size - 1 > sz) {
        ushort *n = d->data + d->size - 1;
        ushort *e = d->data + sz;
        while (n != e)
           * --n = ' ';
    }
}

/*! \fn void QString::clear()

    Clears the contents of the string and makes it empty.

    \sa resize(), isEmpty()
*/

/*! \fn QString &QString::operator=(const QString &other)

    Assigns \a other to this string and returns a reference to this
    string.
*/

QString &QString::operator=(const QString &other)
{
    other.d->ref.ref();
    if (!d->ref.deref())
        free(d);
    d = other.d;
    return *this;
}


/*! \fn QString &QString::operator=(const QLatin1String &str)

    \overload

    Assigns the Latin-1 string \a str to this string.
*/

/*! \fn QString &QString::operator=(const QByteArray &ba)

    \overload

    Assigns \a ba to this string. The byte array is converted to
    Unicode using the fromAscii() function.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn QString &QString::operator=(const char *str)

    \overload

    Assigns \a str to this string. The const char pointer is converted
    to Unicode using the fromAscii() function.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn QString &QString::operator=(char ch)

    \overload

    Assigns character \a ch to this string. The character is converted
    to Unicode using the fromAscii() function.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*!
    \overload

    Sets the string to contain the single character \a ch.
*/
QString &QString::operator=(QChar ch)
{
    return operator=(QString(ch));
}

/*!
     \fn QString& QString::insert(int position, const QString &str)

    Inserts the string \a str at the given index \a position and
    returns a reference to this string.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::insertFunction()
    \skipto QString str
    \printuntil  // str == "Montreal"

    If the given \a position is greater than size(), the array is
    first extended using resize().

    \sa append(), prepend(), replace(), remove()
*/


/*!
    \fn QString &QString::insert(int position, const QLatin1String &str)
    \overload

    Inserts the Latin-1 string \a str at the given index \a position.
*/
QString &QString::insert(int i, const QLatin1String &str)
{
    const uchar *s = (const uchar *)str.latin1();
    if (i < 0 || !s || !(*s))
        return *this;

    int len = qstrlen(str.latin1());
    expand(qMax(d->size, i) + len - 1);

    ::memmove(d->data + i + len, d->data + i, (d->size - i - len) * sizeof(QChar));
    for (int j = 0; j < len; ++j)
        d->data[i + j] = s[j];
    return *this;
}

/*!
    \fn QString& QString::insert(int position, const QChar *unicode, int size)
    \overload

    Inserts the first \a size characters of the QChar array \a unicode
    at the given index \a position in the string.
*/
QString& QString::insert(int i, const QChar *unicode, int size)
{
    if (i < 0 || size <= 0)
        return *this;

    const ushort *s = (const ushort *)unicode;
    if (s >= d->data && s < d->data + d->alloc) {
        // Part of me - take a copy
        ushort *tmp = static_cast<ushort *>(malloc(size * sizeof(QChar)));
        memcpy(tmp, s, size * sizeof(QChar));
        insert(i, reinterpret_cast<const QChar *>(tmp), size);
        ::free(tmp);
        return *this;
    }

    expand(qMax(d->size, i) + size - 1);

    ::memmove(d->data + i + size, d->data + i, (d->size - i - size) * sizeof(QChar));
    memcpy(d->data + i, s, size * sizeof(QChar));
    return *this;
}

/*!
    \fn QString& QString::insert(int position, QChar ch)
    \overload

    Inserts \a ch at the given index \a position in the string.
*/

QString& QString::insert(int i, QChar ch)
{
    if (i < 0)
        i += d->size;
    if (i < 0)
        return *this;
    expand(qMax(i, d->size));
    ::memmove(d->data + i + 1, d->data + i, (d->size - i) * sizeof(QChar));
    d->data[i] = ch.unicode();
    return *this;
}

/*!
    Appends the string \a str onto the end of this string.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::appendFunction()
    \skipto QString
    \printuntil //

    This is the same as using the insert() function:

    \skipto insert
    \printline insert

    The append() function is typically very fast (\l{constant time}),
    because QString preallocates extra space at the end of the string
    data so it can grow without reallocating the entire string each
    time.

    \sa operator+=(), prepend(), insert()
*/
QString &QString::append(const QString &str)
{
    if (str.d != &shared_null) {
        if (d == &shared_null) {
            operator=(str);
        } else {
            if (d->ref != 1 || d->size + str.d->size > d->alloc)
                realloc(grow(d->size + str.d->size));
            memcpy(d->data + d->size, str.d->data, str.d->size * sizeof(QChar));
            d->size += str.d->size;
            d->data[d->size] = '\0';
        }
    }
    return *this;
}

/*! \overload

    Appends the Latin-1 string \a str to this string.
*/
QString &QString::append(const QLatin1String &str)
{
    const uchar *s = (const uchar *)str.latin1();
    if (s) {
        int len = qstrlen((char *)s);
        if (d->ref != 1 || d->size + len > d->alloc)
            realloc(grow(d->size + len));
        ushort *i = d->data + d->size;
        while ((*i++ = *s++))
            ;
        d->size += len;
    }
    return *this;
}

/*! \fn QString &QString::append(const QByteArray &ba)

    \overload

    Appends the byte array \a ba to this string. The given byte array
    is converted to Unicode using the fromAscii() function.

    You can disable this function by defining \c QT_NO_CAST_FROM_ASCII
    when you compile your applications. This can be useful if you want
    to ensure that all user-visible strings go through QObject::tr(),
    for example.
*/

/*! \fn QString &QString::append(const char *str)

    \overload

    Appends the string \a str to this string. The given const char
    pointer is converted to Unicode using the fromAscii() function.

    You can disable this function by defining \c QT_NO_CAST_FROM_ASCII
    when you compile your applications. This can be useful if you want
    to ensure that all user-visible strings go through QObject::tr(),
    for example.
*/

/*!
    \overload

    Appends the character \a ch to this string.
*/
QString &QString::append(QChar ch)
{
    if (d->ref != 1 || d->size + 1 > d->alloc)
        realloc(grow(d->size + 1));
    d->data[d->size++] = ch.unicode();
    d->data[d->size] = '\0';
    return *this;
}

/*! \fn QString &QString::prepend(const QString &str)

    Prepends the string \a str to the beginning of this string and
    returns a reference to this string.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::prependFunction()
    \skipto QString x
    \printuntil // x == "airship"

    \sa append(), insert()
*/

/*! \fn QString &QString::prepend(const QLatin1String &str)

    \overload

    Prepends the Latin-1 string \a str to this string.
*/

/*! \fn QString &QString::prepend(const QByteArray &ba)

    \overload

    Prepends the byte array \a ba to this string. The byte array is
    converted to Unicode using the fromAscii() function.

    You can disable this function by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn QString &QString::prepend(const char *str)

    \overload

    Prepends the string \a str to this string. The const char pointer
    is converted to Unicode using the fromAscii() function.

    You can disable this function by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn QString &QString::prepend(QChar ch)

    \overload

    Prepends the character \a ch to this string.
*/

/*!
    \fn QString &QString::remove(int position, int n)

    Removes \a n characters from the string, starting at the given \a
    position index, and returns a reference to the string.

    If the specified \a position index is within the string, but \a
    position + \a n is beyond the end of the string, the string is
    truncated at the specified \a position.

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::removeFunction()
    \skipto QString s
    \printuntil // s == "Meal"

    \sa insert(), replace()
*/

QString &QString::remove(int pos, int len)
{
    if (pos < 0)
        pos += d->size;
    if (pos < 0 || pos >= d->size) {
        // range problems
    } else if (pos + len >= d->size) {  // pos ok
        resize(pos);
    } else if (len > 0) {
        detach();
        memmove(d->data + pos, d->data + pos + len,
                (d->size - pos - len + 1) * sizeof(ushort));
        d->size -= len;
    }
    return *this;
}

/*! \overload

    Removes every occurrence of the given \a str string in this
    string, and returns a reference to this string.

    If \a cs is Qt::CaseSensitive (the default), the search is
    case sensitive; otherwise the search is case insensitive.

    This is the same as \c replace(str, "", cs).

    \sa replace()
*/
QString &QString::remove(const QString &str, Qt::CaseSensitivity cs)
{
    if (str.d->size) {
        int i = 0;
        while ((i = indexOf(str, i, cs)) != -1)
            remove(i, str.d->size);
    }
    return *this;
}

/*! \overload

    Removes every occurrence of the character \a ch in this string,
    and returns a reference to this string.

    If \a cs is Qt::CaseSensitive (the default), the search is
    case sensitive; otherwise the search is case insensitive.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::removeFunction()
    \skipto QString t
    \printuntil // t == "li Bb"

    This is the same as \c replace(ch, "", cs).

    \sa replace()
*/
QString &QString::remove(QChar ch, Qt::CaseSensitivity cs)
{
    int i = 0;
    ushort c = ch.unicode();
    if (cs == Qt::CaseSensitive) {
        while (i < d->size)
            if (d->data[i] == ch)
                remove(i, 1);
            else
                i++;
    } else {
        c = foldCase(c);
        while (i < d->size)
            if (foldCase(d->data[i]) == c)
                remove(i, 1);
            else
                i++;
    }
    return *this;
}

/*! \fn QString &QString::remove(const QRegExp &rx)

    \overload

    Removes every occurrence of the regular expression \a rx in the
    string, and returns a reference to the string. For example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::removeFunction()
    \skipto QString r
    \printuntil // r == "The"

    \sa indexOf(), lastIndexOf(), replace()
*/

/*!
    \fn QString &QString::replace(int position, int n, const QString &after)

    Replaces \a n characters from the specified index \a position with the string \a
    after, and returns a reference to this string.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::replaceFunction()
    \skipto QString x
    \printuntil // x == "Say no!"

    \sa insert(), remove()
*/

QString &QString::replace(int pos, int len, const QString &after)
{
    QString copy = after;
    remove(pos, len);
    return insert(pos, copy.constData(), copy.d->size);
}

/*!
    \fn QString &QString::replace(int position, int n, const QChar *unicode, int size)
    \overload

    Replaces \a n characters from the specified index \a position with
    the first \a size characters of the QChar array \a unicode.
*/
QString &QString::replace(int pos, int len, const QChar *unicode, int size)
{
    remove(pos, len);
    return insert(pos, unicode, size);
}

/*!
    \fn QString &QString::replace(int position, int n, QChar after)
    \overload

    Replaces \a n characters from the specified index \a position with
    the character \a after.
*/
QString &QString::replace(int pos, int len, QChar after)
{
    remove(pos, len);
    return insert(pos, after);
}

/*! \overload

    Replaces every occurrence of the string \a before with the string
    \a after.

    If \a cs is Qt::CaseSensitive (the default), the search is
    case sensitive; otherwise the search is case insensitive.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::replaceFunction()
    \skipto QString str
    \printuntil // str == "color behavior flavor neighbor"
*/
QString &QString::replace(const QString &before, const QString &after, Qt::CaseSensitivity cs)
{
    if (d->size == 0) {
        if (before.d->size)
            return *this;
    } else {
        if (cs == Qt::CaseSensitive && before == after)
            return *this;
    }
    if (d->ref != 1)
        realloc(d->size);

    QStringMatcher matcher(before, cs);
    int index = 0;
    const int bl = before.d->size;
    const int al = after.d->size;

    if (bl == al) {
        if (bl) {
            const QChar *auc = (const QChar*) after.d->data;
            while ((index = matcher.indexIn(*this, index)) != -1) {
                // we need memmove(), not memcpy(), in the rare case where
                // this == &after, before == after, and cs is Qt::CaseInsensitive
                memmove(d->data + index, auc, al * sizeof(QChar));
                index += bl;
            }
        }
    } else if (al < bl) {
        const QChar *auc = after.unicode();
        uint to = 0;
        uint movestart = 0;
        uint num = 0;
        while ((index = matcher.indexIn(*this, index)) != -1) {
            if (num) {
                int msize = index - movestart;
                if (msize > 0) {
                    memmove(d->data + to, d->data + movestart, msize * sizeof(QChar));
                    to += msize;
                }
            } else {
                to = index;
            }
            if (al) {
                memcpy(d->data+to, auc, al*sizeof(QChar));
                to += al;
            }
            index += bl;
            movestart = index;
            num++;
        }
        if (num) {
            int msize = d->size - movestart;
            if (msize > 0)
                memmove(d->data + to, d->data + movestart, msize * sizeof(QChar));
            resize(d->size - num*(bl-al));
        }
    } else {
        const QString copy = after;

        // the most complex case. We don't want to lose performance by doing repeated
        // copies and reallocs of the string.
        while (index != -1) {
            uint indices[4096];
            uint pos = 0;
            while (pos < 4095) {
                index = matcher.indexIn(*this, index);
                if (index == -1)
                    break;
                indices[pos++] = index;
                index += bl;
                // avoid infinite loop
                if (!bl)
                    index++;
            }
            if (!pos)
                break;

            // we have a table of replacement positions, use them for fast replacing
            int adjust = pos*(al-bl);
            // index has to be adjusted in case we get back into the loop above.
            if (index != -1)
                index += adjust;
            int newLen = d->size + adjust;
            int moveend = d->size;
            if (newLen > d->size)
                resize(newLen);

            while (pos) {
                pos--;
                int movestart = indices[pos] + bl;
                int insertstart = indices[pos] + pos*(al-bl);
                int moveto = insertstart + al;
                memmove(d->data + moveto, d->data + movestart, (moveend - movestart)*sizeof(QChar));
                memcpy(d->data + insertstart, copy.unicode(), al*sizeof(QChar));
                moveend = movestart-bl;
            }
        }
    }
    return *this;
}

/*! \overload

    Replaces every occurrence of the character \a ch in the string
    with \a after. Returns a reference to the string.

    If \a cs is Qt::CaseSensitive (the default), the search is
    case sensitive; otherwise the search is case insensitive.
*/

QString& QString::replace(QChar ch, const QString &after, Qt::CaseSensitivity cs)
{
    return replace(QString(ch), after, cs);
}

/*! \overload

    Replaces every occurrence of the character \a before with the
    character \a after. Returns a reference to the string.

    If \a cs is Qt::CaseSensitive (the default), the search is
    case sensitive; otherwise the search is case insensitive.
*/
QString& QString::replace(QChar before, QChar after, Qt::CaseSensitivity cs)
{
    ushort a = after.unicode();
    ushort b = before.unicode();
    if (d->size) {
        detach();
        ushort *i = d->data;
        const ushort *e = i + d->size;
        if (cs == Qt::CaseSensitive) {
            for (; i != e; ++i)
                if (*i == b)
                    *i = a;
        } else {
            b = foldCase(b);
            for (; i != e; ++i)
                if (foldCase(*i) == b)
                    *i = a;
        }
    }
    return *this;
}

/*!
    Returns true if string \a other is equal to this string;
    otherwise returns false.

    The comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would
    expect. Consider sorting user-interface strings with
    localeAwareCompare().
*/
bool QString::operator==(const QString &other) const
{
    return (size() == other.size()) &&
        (memcmp((char*)unicode(),(char*)other.unicode(), size()*sizeof(QChar))==0);
}

/*!
    \overload
*/
bool QString::operator==(const QLatin1String &other) const
{
    const ushort *uc = d->data;
    const ushort *e = uc + d->size;
    const uchar *c = (uchar *)other.latin1();

    if (!c)
        return isEmpty();

    while (*c) {
        if (uc == e || *uc != *c)
            return false;
        ++uc;
        ++c;
    }
    return (uc == e);
}

/*! \fn bool QString::operator==(const QByteArray &other) const

    \overload

    The \a other byte array is converted to a QString using the
    fromAscii() function.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn bool QString::operator==(const char *other) const

    \overload

    The \a other const char pointer is converted to a QString using
    the fromAscii() function.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*!
    Returns true if this string is lexically less than string \a
    other; otherwise returns false.

    The comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would
    expect. Consider sorting user-interface strings using the
    QString::localeAwareCompare() function.
*/
bool QString::operator<(const QString &other) const
{
    return ucstrcmp(*this, other) < 0;
}

/*!
    \overload
*/
bool QString::operator<(const QLatin1String &other) const
{
    const ushort *uc = d->data;
    const ushort *e = uc + d->size;
    const uchar *c = (uchar *) other.latin1();

    if (!c || *c == 0)
        return false;

    while (*c) {
        if (uc == e || *uc != *c)
            break;
        ++uc;
        ++c;
    }
    return (uc == e ? *c : *uc < *c);
}

/*! \fn bool QString::operator<(const QByteArray &other) const

    \overload

    The \a other byte array is converted to a QString using the
    fromAscii() function.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn bool QString::operator<(const char *other) const

    \overload

    The \a other const char pointer is converted to a QString using
    the fromAscii() function.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn bool QString::operator<=(const QString &other) const

    Returns true if this string is lexically less than or equal to
    string \a other; otherwise returns false.

    The comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would
    expect. Consider sorting user-interface strings with
    localeAwareCompare().
*/

/*! \fn bool QString::operator<=(const QLatin1String &other) const

    \overload
*/

/*! \fn bool QString::operator<=(const QByteArray &other) const

    \overload

    The \a other byte array is converted to a QString using the
    fromAscii() function.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn bool QString::operator<=(const char *other) const

    \overload

    The \a other const char pointer is converted to a QString using
    the fromAscii() function.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn bool QString::operator>(const QString &other) const

    Returns true if this string is lexically greater than string \a
    other; otherwise returns false.

    The comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would
    expect. Consider sorting user-interface strings with
    localeAwareCompare().
*/

/*!
    \overload
*/
bool QString::operator>(const QLatin1String &other) const
{
    const ushort *uc = d->data;;
    const ushort *e = uc + d->size;
    const uchar *c = (uchar *) other.latin1();

    if (!c || *c == '\0')
        return !isEmpty();

    while (*c) {
        if (uc == e || *uc != *c)
            break;
        ++uc;
        ++c;
    }
    return (uc == e ? false : *uc > *c);
}

/*! \fn bool QString::operator>(const QByteArray &other) const

    \overload

    The \a other byte array is converted to a QString using the
    fromAscii() function.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn bool QString::operator>(const char *other) const

    \overload

    The \a other const char pointer is converted to a QString using
    the fromAscii() function.

    You can disable this operator by defining \c QT_NO_CAST_FROM_ASCII
    when you compile your applications. This can be useful if you want
    to ensure that all user-visible strings go through QObject::tr(),
    for example.
*/

/*! \fn bool QString::operator>=(const QString &other) const

    Returns true if this string is lexically greater than or equal to
    string \a other; otherwise returns false.

    The comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would
    expect. Consider sorting user-interface strings with
    localeAwareCompare().
*/

/*! \fn bool QString::operator>=(const QLatin1String &other) const

    \overload
*/

/*! \fn bool QString::operator>=(const QByteArray &other) const

    \overload

    The \a other byte array is converted to a QString using the
    fromAscii() function.

    You can disable this operator by defining \c QT_NO_CAST_FROM_ASCII
    when you compile your applications. This can be useful if you want
    to ensure that all user-visible strings go through QObject::tr(),
    for example.
*/

/*! \fn bool QString::operator>=(const char *other) const

    \overload

    The \a other const char pointer is converted to a QString using
    the fromAscii() function.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn bool QString::operator!=(const QString &other) const

    Returns true if this string is not equal to string \a other;
    otherwise returns false.

    The comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would
    expect. Consider sorting user-interface strings with
    localeAwareCompare().
*/

/*! \fn bool QString::operator!=(const QLatin1String &other) const

    \overload
*/

/*! \fn bool QString::operator!=(const QByteArray &other) const

    \overload

    The \a other byte array is converted to a QString using the
    fromAscii() function.

    You can disable this operator by defining \c QT_NO_CAST_FROM_ASCII
    when you compile your applications. This can be useful if you want
    to ensure that all user-visible strings go through QObject::tr(),
    for example.
*/

/*! \fn bool QString::operator!=(const char *other) const

    \overload

    The \a other const char pointer is converted to a QString using
    the fromAscii() function.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

#define REHASH(a) \
    if (sl_minus_1 < (int)sizeof(int) * CHAR_BIT)       \
        hashHaystack -= (a) << sl_minus_1; \
    hashHaystack <<= 1

/*!
    Returns the index position of the first occurrence of the string
    \a str in this string, searching forward from index position \a
    from. Returns -1 if \a str is not found.

    If \a cs is Qt::CaseSensitive (the default), the search is
    case sensitive; otherwise the search is case insensitive.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::indexOfFunction()
    \skipto QString x
    \printuntil  x.indexOf(y, 11)

    If \a from is -1, the search starts at the last character; if it
    is -2, at the next to last character and so on.

    \sa lastIndexOf(), contains(), count()
*/
int QString::indexOf(const QString &str, int from, Qt::CaseSensitivity cs) const
{
    const int l = d->size;
    const int sl = str.d->size;
    if (from < 0)
        from += l;
    if (uint(sl + from) > (uint)l)
        return -1;
    if (!sl)
        return from;
    if (!l)
        return -1;

    if (sl == 1)
        return indexOf(QChar(str.d->data[0]), from, cs);

    /*
        We use the Boyer-Moore algorithm in cases where the overhead
        for the skip table should pay off, otherwise we use a simple
        hash function.
    */
    if (l > 500 && sl > 5)
        return QStringMatcher(str, cs).indexIn(*this, from);

    /*
        We use some hashing for efficiency's sake. Instead of
        comparing strings, we compare the hash value of str with that
        of a part of this QString. Only if that matches, we call
        ucstrncmp() or ucstrnicmp().
    */
    const ushort *needle = str.d->data;
    const ushort *haystack = d->data + from;
    const ushort *end = d->data + (l-sl);
    const int sl_minus_1 = sl-1;
    int hashNeedle = 0, hashHaystack = 0, idx;

    if (cs == Qt::CaseSensitive) {
        for (idx = 0; idx < sl; ++idx) {
            hashNeedle = ((hashNeedle<<1) + needle[idx]);
            hashHaystack = ((hashHaystack<<1) + haystack[idx]);
        }
        hashHaystack -= haystack[sl_minus_1];

        while (haystack <= end) {
            hashHaystack += haystack[sl_minus_1];
            if (hashHaystack == hashNeedle
                 && ucstrncmp((const QChar *)needle, (const QChar *)haystack, sl) == 0)
                return haystack - d->data;

            REHASH(*haystack);
            ++haystack;
        }
    } else {
        const ushort *haystack_start = d->data;
        for (idx = 0; idx < sl; ++idx) {
            hashNeedle = (hashNeedle<<1) + foldCase(needle + idx, needle);
            hashHaystack = (hashHaystack<<1) + foldCase(haystack + idx, haystack_start);
        }
        hashHaystack -= foldCase(haystack + sl_minus_1, haystack_start);

        while (haystack <= end) {
            hashHaystack += foldCase(haystack + sl_minus_1, haystack_start);
            if (hashHaystack == hashNeedle && ucstrnicmp(needle, haystack, sl) == 0)
                return haystack - d->data;

            REHASH(foldCase(haystack, haystack_start));
            ++haystack;
        }
    }
    return -1;
}

/*!
    \overload

    Returns the index position of the first occurrence of the
    character \a ch in the string, searching forward from index
    position \a from. Returns -1 if \a ch could not be found.
*/
int QString::indexOf(QChar ch, int from, Qt::CaseSensitivity cs) const
{
    ushort c = ch.unicode();
    if (from < 0)
        from = qMax(from + d->size, 0);
    if (from  < d->size) {
        const ushort *n = d->data + from - 1;
        const ushort *e = d->data + d->size;
        if (cs == Qt::CaseSensitive) {
            while (++n != e)
                if (*n == c)
                    return  n - d->data;
        } else {
            c = foldCase(c);
            while (++n != e)
                if (foldCase(*n) == c)
                    return  n - d->data;
        }
    }
    return -1;
}

/*!
    Returns the index position of the last occurrence of the string
    \a str in this string, searching backward from index position \a
    from. If \a from is -1 (the default), the search starts at the
    last character; if \a from is -2, at the next to last character
    and so on. Returns -1 if \a str is not found.

    If \a cs is Qt::CaseSensitive (the default), the search is
    case sensitive; otherwise the search is case insensitive.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::lastIndexOfFunction()
    \skipto QString x
    \printuntil x.lastIndexOf(y, 1)

    \sa indexOf(), contains(), count()
*/
int QString::lastIndexOf(const QString &str, int from, Qt::CaseSensitivity cs) const
{
    /*
        See indexOf() for explanations.
    */
    const int l = d->size;
    if (from < 0)
        from += l;
    const int sl = str.d->size;
    int delta = l-sl;
    if (from == l && sl == 0)
        return from;
    if (from < 0 || from >= l || delta < 0)
        return -1;
    if (from > delta)
        from = delta;

    if (sl == 1)
        return lastIndexOf(QChar(str.d->data[0]), from, cs);

    const ushort *needle = str.d->data;
    const ushort *haystack = d->data + from;
    const ushort *end = d->data;
    const int sl_minus_1 = sl-1;
    const ushort *n = needle+sl_minus_1;
    const ushort *h = haystack+sl_minus_1;
    int hashNeedle = 0, hashHaystack = 0, idx;

    if (cs == Qt::CaseSensitive) {
        for (idx = 0; idx < sl; ++idx) {
            hashNeedle = ((hashNeedle<<1) + *(n-idx));
            hashHaystack = ((hashHaystack<<1) + *(h-idx));
        }
        hashHaystack -= *haystack;

        while (haystack >= end) {
            hashHaystack += *haystack;
            if (hashHaystack == hashNeedle
                 && ucstrncmp((const QChar *)needle, (const QChar *)haystack, sl) == 0)
                return haystack - d->data;
            --haystack;
            REHASH(haystack[sl]);
        }
    } else {
        for (idx = 0; idx < sl; ++idx) {
            hashNeedle = ((hashNeedle<<1) + foldCase(n-idx, str.d->data));
            hashHaystack = ((hashHaystack<<1) + foldCase(h-idx, end));
        }
        hashHaystack -= foldCase(haystack, end);

        while (haystack >= end) {
            hashHaystack += foldCase(haystack, end);
            if (hashHaystack == hashNeedle && ucstrnicmp(needle, haystack, sl) == 0)
                return haystack - d->data;
            --haystack;
            REHASH(foldCase(haystack + sl, end));
        }
    }
    return -1;
}

/*! \overload

    Returns the index position of the last occurrence of the
    character \a ch, searching backward from position \a from.
*/
int QString::lastIndexOf(QChar ch, int from, Qt::CaseSensitivity cs) const
{
    ushort c = ch.unicode();
    if (from < 0)
        from += d->size;
    if (from < 0 || from >= d->size)
        return -1;
    if (from >= 0) {
        const ushort *n = d->data + from;
        const ushort *b = d->data;
        if (cs == Qt::CaseSensitive) {
            for (; n >= b; --n)
                if (*n == c)
                    return n - b;
        } else {
            c = foldCase(c);
            for (; n >= b; --n)
                if (foldCase(*n) == c)
                    return n - b;
        }
    }
    return -1;
}

#ifndef QT_NO_REGEXP
struct QStringCapture
{
    int pos;
    int len;
    int no;
};

/*! \overload

    Replaces every occurrence of the regular expression \a rx in the
    string with \a after. Returns a reference to the string. For
    example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::replaceFunction()
    \skipto QString s =
    \printuntil // s == "Boxoxa"

    For regular expressions containing \l{capturing parentheses},
    occurrences of \bold{\\1}, \bold{\\2}, ..., in \a after are
    replaced with \a{rx}.cap(1), cap(2), ...

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::replaceFunction()
    \skipto QString t
    \printuntil // t == "A \\emph{bon mot}."

    \sa indexOf(), lastIndexOf(), remove(), QRegExp::cap()
*/
QString& QString::replace(const QRegExp &rx, const QString &after)
{
    QRegExp rx2(rx);

    if (isEmpty() && rx2.indexIn(*this) == -1)
        return *this;

    realloc();

    int index = 0;
    int numCaptures = rx2.numCaptures();
    int al = after.length();
    QRegExp::CaretMode caretMode = QRegExp::CaretAtZero;

    if (numCaptures > 0) {
        const QChar *uc = after.unicode();
        int numBackRefs = 0;

        for (int i = 0; i < al - 1; i++) {
            if (uc[i] == QLatin1Char('\\')) {
                int no = uc[i + 1].digitValue();
                if (no > 0 && no <= numCaptures)
                    numBackRefs++;
            }
        }

        /*
            This is the harder case where we have back-references.
        */
        if (numBackRefs > 0) {
            QVarLengthArray<QStringCapture, 16> captures(numBackRefs);
            int j = 0;

            for (int i = 0; i < al - 1; i++) {
                if (uc[i] == QLatin1Char('\\')) {
                    int no = uc[i + 1].digitValue();
                    if (no > 0 && no <= numCaptures) {
                        QStringCapture capture;
                        capture.pos = i;
                        capture.len = 2;

                        if (i < al - 2) {
                            int secondDigit = uc[i + 2].digitValue();
                            if (secondDigit != -1 && ((no * 10) + secondDigit) <= numCaptures) {
                                no = (no * 10) + secondDigit;
                                ++capture.len;
                            }
                        }

                        capture.no = no;
                        captures[j++] = capture;
                    }
                }
            }

            while (index <= length()) {
                index = rx2.indexIn(*this, index, caretMode);
                if (index == -1)
                    break;

                QString after2(after);
                for (j = numBackRefs - 1; j >= 0; j--) {
                    const QStringCapture &capture = captures[j];
                    after2.replace(capture.pos, capture.len, rx2.cap(capture.no));
                }

                replace(index, rx2.matchedLength(), after2);
                index += after2.length();

                // avoid infinite loop on 0-length matches (e.g., QRegExp("[a-z]*"))
                if (rx2.matchedLength() == 0)
                    ++index;

                caretMode = QRegExp::CaretWontMatch;
            }
            return *this;
        }
    }

    /*
        This is the simple and optimized case where we don't have
        back-references.
    */
    while (index != -1) {
        struct {
            int pos;
            int length;
        } replacements[2048];

        int pos = 0;
        int adjust = 0;
        while (pos < 2047) {
            index = rx2.indexIn(*this, index, caretMode);
            if (index == -1)
                break;
            int ml = rx2.matchedLength();
            replacements[pos].pos = index;
            replacements[pos++].length = ml;
            index += ml;
            adjust += al - ml;
            // avoid infinite loop
            if (!ml)
                index++;
        }
        if (!pos)
            break;
        replacements[pos].pos = d->size;
        int newlen = d->size + adjust;

        // to continue searching at the right position after we did
        // the first round of replacements
        if (index != -1)
            index += adjust;
        QString newstring;
        newstring.reserve(newlen + 1);
        QChar *newuc = newstring.data();
        QChar *uc = newuc;
        int copystart = 0;
        int i = 0;
        while (i < pos) {
            int copyend = replacements[i].pos;
            int size = copyend - copystart;
            memcpy(uc, d->data + copystart, size * sizeof(QChar));
            uc += size;
            memcpy(uc, after.d->data, al * sizeof(QChar));
            uc += al;
            copystart = copyend + replacements[i].length;
            i++;
        }
        memcpy(uc, d->data + copystart, (d->size - copystart) * sizeof(QChar));
        newstring.resize(newlen);
        *this = newstring;
        caretMode = QRegExp::CaretWontMatch;
    }
    return *this;
}
#endif

/*!
    Returns the number of (potentially overlapping) occurrences of
    the string \a str in this string.

    If \a cs is Qt::CaseSensitive (the default), the search is
    case sensitive; otherwise the search is case insensitive.

    \sa contains(), indexOf()
*/
int QString::count(const QString &str, Qt::CaseSensitivity cs) const
{
    int num = 0;
    int i = -1;
    if (d->size > 500 && str.d->size > 5) {
        QStringMatcher matcher(str, cs);
        while ((i = matcher.indexIn(*this, i + 1)) != -1)
            ++num;
    } else {
        while ((i = indexOf(str, i + 1, cs)) != -1)
            ++num;
    }
    return num;
}

/*! \overload

    Returns the number of occurrences of character \a ch in the
    string.
*/
int QString::count(QChar ch, Qt::CaseSensitivity cs) const
{
    ushort c = ch.unicode();
    int num = 0;
    const ushort *i = d->data + d->size;
    const ushort *b = d->data;
    if (cs == Qt::CaseSensitive) {
        while (i != b)
            if (*--i == c)
                ++num;
    } else {
        c = foldCase(c);
        while (i != b)
            if (foldCase(*(--i)) == c)
                ++num;
    }
    return num;
}

/*! \fn bool QString::contains(const QString &str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const

    Returns true if this string contains an occurrence of the string
    \a str; otherwise returns false.

    If \a cs is Qt::CaseSensitive (the default), the search is
    case sensitive; otherwise the search is case insensitive.

    Example:
    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::containsFunction()
    \skipto QString str
    \printuntil contains

    \sa indexOf(), count()
*/

/*! \fn bool QString::contains(QChar ch, Qt::CaseSensitivity cs = Qt::CaseSensitive) const

    \overload

    Returns true if this string contains an occurrence of the
    character \a ch; otherwise returns false.
*/

/*! \fn bool QString::contains(const QRegExp &rx) const

    \overload

    Returns true if the regular expression \a rx matches somewhere in
    this string; otherwise returns false.
*/

#ifndef QT_NO_REGEXP
/*!
    \overload

    Returns the index position of the first match of the regular
    expression \a rx in the string, searching forward from index
    position \a from. Returns -1 if \a rx didn't match anywhere.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::firstIndexOfFunction()
    \skipto QString str
    \printuntil str.indexOf(QRegExp("m[aeiou]"), 0)
*/
int QString::indexOf(const QRegExp& rx, int from) const
{
    return rx.indexIn(*this, from);
}

/*!
    \overload

    Returns the index position of the last match of the regular
    expression \a rx in the string, searching backward from index
    position \a from. Returns -1 if \a rx didn't match anywhere.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::lastIndexOfFunction()
    \skipto QString str
    \printuntil str.lastIndexOf(QRegExp("m[aeiou]"))
*/
int QString::lastIndexOf(const QRegExp& rx, int from) const
{
    return rx.lastIndexIn(*this, from);
}

/*!
    \overload

    Returns the number of times the regular expression \a rx matches
    in the string.

    This function counts overlapping matches, so in the example
    below, there are four instances of "ana" or "ama":

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::countFunction()
    \skipto QString str
    \printuntil count

*/
int QString::count(const QRegExp& rx) const
{
    int count = 0;
    int index = -1;
    int len = length();
    while (index < len - 1) {                 // count overlapping matches
        index = rx.indexIn(*this, index + 1);
        if (index == -1)
            break;
        count++;
    }
    return count;
}
#endif // QT_NO_REGEXP

/*! \fn int QString::count() const

    \overload

    Same as size().
*/


/*!
    \enum QString::SectionFlag

    This enum specifies flags that can be used to affect various
    aspects of the section() function's behavior with respect to
    separators and empty fields.

    \value SectionDefault Empty fields are counted, leading and
    trailing separators are not included, and the separator is
    compared case sensitively.

    \value SectionSkipEmpty Treat empty fields as if they don't exist,
    i.e. they are not considered as far as \e start and \e end are
    concerned.

    \value SectionIncludeLeadingSep Include the leading separator (if
    any) in the result string.

    \value SectionIncludeTrailingSep Include the trailing separator
    (if any) in the result string.

    \value SectionCaseInsensitiveSeps Compare the separator
    case-insensitively.

    \sa section()
*/

/*!
    \fn QString QString::section(QChar sep, int start, int end = -1, SectionFlags flags) const

    This function returns a section of the string.

    This string is treated as a sequence of fields separated by the
    character, \a sep. The returned string consists of the fields from
    position \a start to position \a end inclusive. If \a end is not
    specified, all fields from position \a start to the end of the
    string are included. Fields are numbered 0, 1, 2, etc., counting
    from the left, and -1, -2, etc., counting from right to left.

    The \a flags argument can be used to affect some aspects of the
    function's behavior, e.g. whether to be case sensitive, whether
    to skip empty fields and how to deal with leading and trailing
    separators; see \l{SectionFlags}.

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::sectionFunction()
    \skipto QString str
    \printuntil str == "myapp"

    If \a start or \a end is negative, we count fields from the right
    of the string, the right-most field being -1, the one from
    right-most field being -2, and so on.

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::sectionFunction()
    \skipto str = csv.section(',', -3, -2)
    \printuntil str == "myapp"

    \sa split()
*/

/*!
    \overload

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::sectionFunction()
    \skipto QString str
    \printline QString str
    \skipto  QString data
    \printuntil data.section("**", -3, -2)

    \sa split()
*/

QString QString::section(const QString &sep, int start, int end, SectionFlags flags) const
{
    QStringList sections = split(sep, KeepEmptyParts,
                                 (flags & SectionCaseInsensitiveSeps) ? Qt::CaseInsensitive : Qt::CaseSensitive);
    if (sections.isEmpty())
        return QString();
    if (!(flags & SectionSkipEmpty)) {
        if (start < 0)
            start += sections.count();
        if (end < 0)
            end += sections.count();
    } else {
        int skip = 0;
        for (int k=0; k<sections.size(); ++k) {
            if (sections.at(k).isEmpty())
                skip++;
        }
        if (start < 0)
            start += sections.count() - skip;
        if (end < 0)
            end += sections.count() - skip;
    }
    int x = 0;
    QString ret;
    int first_i = start, last_i = end;
    for (int i = 0; x <= end && i < sections.size(); ++i) {
        QString section = sections.at(i);
        const bool empty = section.isEmpty();
        if (x >= start) {
            if(x == start)
                first_i = i;
            if(x == end)
                last_i = i;
            if(x > start)
                ret += sep;
            ret += section;
        }
        if (!empty || !(flags & SectionSkipEmpty))
            x++;
    }
    if((flags & SectionIncludeLeadingSep) && first_i)
        ret.prepend(sep);
    if((flags & SectionIncludeTrailingSep) && last_i < sections.size()-1)
        ret += sep;
    return ret;
}

#ifndef QT_NO_REGEXP
class qt_section_chunk {
public:
    qt_section_chunk(int l, QString s) { length = l; string = s; }
    int length;
    QString string;
};

/*!
    \overload

    This string is treated as a sequence of fields separated by the
    regular expression, \a reg.

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::sectionFunction()
    \skipto QString line
    \printuntil str = line.section(sep, -3, -2)

    \warning Using this QRegExp version is much more expensive than
    the overloaded string and character versions.

    \sa split() simplified()
*/
QString QString::section(const QRegExp &reg, int start, int end, SectionFlags flags) const
{
    const QChar *uc = unicode();
    if(!uc)
        return QString();

    QRegExp sep(reg);
    sep.setCaseSensitivity((flags & SectionCaseInsensitiveSeps) ? Qt::CaseInsensitive
                                                                : Qt::CaseSensitive);

    QList<qt_section_chunk> sections;
    int n = length(), m = 0, last_m = 0, last_len = 0;
    while ((m = sep.indexIn(*this, m)) != -1) {
        sections.append(qt_section_chunk(last_len, QString(uc + last_m, m - last_m)));
        last_m = m;
        last_len = sep.matchedLength();
        m += qMax(sep.matchedLength(), 1);
    }
    sections.append(qt_section_chunk(last_len, QString(uc + last_m, n - last_m)));

    if(start < 0)
        start += sections.count();
    if(end < 0)
        end += sections.count();

    QString ret;
    int x = 0;
    int first_i = start, last_i = end;
    for (int i = 0; x <= end && i < sections.size(); ++i) {
        const qt_section_chunk &section = sections.at(i);
        const bool empty = (section.length == section.string.length());
        if (x >= start) {
            if(x == start)
                first_i = i;
            if(x == end)
                last_i = i;
            if(x != start)
                ret += section.string;
            else
                ret += section.string.mid(section.length);
        }
        if (!empty || !(flags & SectionSkipEmpty))
            x++;
    }
    if((flags & SectionIncludeLeadingSep)) {
        const qt_section_chunk &section = sections.at(first_i);
        ret.prepend(section.string.left(section.length));
    }
    if((flags & SectionIncludeTrailingSep) && last_i+1 <= sections.size()-1) {
        const qt_section_chunk &section = sections.at(last_i+1);
        ret += section.string.left(section.length);
    }
    return ret;
}
#endif

/*!
    Returns a substring that contains the \a n leftmost characters
    of the string.

    The entire string is returned if \a n is greater than size() or
    less than zero.

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::leftFunction()
    \skipto QString x
    \printuntil QString y = x.left(4)

    \sa right(), mid(), startsWith()
*/
QString QString::left(int n)  const
{
    if (n >= d->size || n < 0)
        return *this;
    return QString((const QChar*) d->data, n);
}

/*!
    Returns a substring that contains the \a n rightmost characters
    of the string.

    The entire string is returned if \a n is greater than size() or
    less than zero.

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::rightFunction()
    \skipto QString x
    \printuntil y == "apple"

    \sa left(), mid(), endsWith()
*/
QString QString::right(int n) const
{
    if (n >= d->size || n < 0)
        return *this;
    return QString((const QChar*) d->data + d->size - n, n);
}

/*!
    Returns a string that contains \a n characters of this string,
    starting at the specified \a position index.

    Returns an empty string if the \a position index exceeds the
    length of the string. If there are less than \a n characters
    available in the string starting at the given \a position, or if
    \a n is -1 (the default), the function returns all characters that
    are available from the specified \a position.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::midFunction()
    \skipto QString x
    \printuntil QString z

    \sa left(), right()
*/

QString QString::mid(int position, int n) const
{
    if (d == &shared_null || position >= d->size)
        return QString();
    if (n < 0)
        n = d->size - position;
    if (position < 0) {
        n += position;
        position = 0;
    }
    if (n + position > d->size)
        n = d->size - position;
    if (position == 0 && n == d->size)
        return *this;
    return QString((const QChar*) d->data + position, n);
}

/*!
    Returns true if the string starts with \a s; otherwise returns
    false.

    If \a cs is Qt::CaseSensitive (the default), the search is
    case sensitive; otherwise the search is case insensitive.

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::startsWithFunction()
    \skipto QString str
    \printuntil str.startsWith("Car");     // returns false

    \sa endsWith()
*/
bool QString::startsWith(const QString& s, Qt::CaseSensitivity cs) const
{
    if (d == &shared_null)
        return (s.d == &shared_null);
    if (d->size == 0)
        return s.d->size == 0;
    if (s.d->size > d->size)
        return false;
    if (cs == Qt::CaseSensitive) {
        return memcmp((char*)d->data, (char*)s.d->data, s.d->size*sizeof(QChar)) == 0;
    } else {
        uint last = 0;
        uint olast = 0;
        for (int i = 0; i < s.d->size; ++i)
            if (foldCase(d->data[i], last) != foldCase(s.d->data[i], olast))
                return false;
    }
    return true;
}

/*!
  \overload
 */
bool QString::startsWith(const QLatin1String& s, Qt::CaseSensitivity cs) const
{
    if (d == &shared_null)
        return (s.latin1() == 0);
    if (d->size == 0)
        return !s.latin1() || *s.latin1() == 0;
    int slen = qstrlen(s.latin1());
    if (slen > d->size)
        return false;
    const uchar *latin = (const uchar *)s.latin1();
    if (cs == Qt::CaseSensitive) {
        for (int i = 0; i < slen; ++i)
            if (d->data[i] != latin[i])
                return false;
    } else {
        for (int i = 0; i < slen; ++i)
            if (foldCase(d->data[i]) != foldCase((ushort)latin[i]))
                return false;
    }
    return true;
}

/*!
  \overload

  Returns true if the string starts with \a c; otherwise returns
  false.
*/
bool QString::startsWith(const QChar &c, Qt::CaseSensitivity cs) const
{
    return d->size
           && (cs == Qt::CaseSensitive
               ? d->data[0] == c
               : foldCase(d->data[0]) == foldCase(c.unicode()));
}

/*!
    Returns true if the string ends with \a s; otherwise returns
    false.

    If \a cs is Qt::CaseSensitive (the default), the search is case
    sensitive; otherwise the search is case insensitive.

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::endsWithFunction
    \skipto QString str
    \printuntil str.endsWith("pple")

    \sa startsWith()
*/
bool QString::endsWith(const QString& s, Qt::CaseSensitivity cs) const
{
    if (d == &shared_null)
        return (s.d == &shared_null);
    if (d->size == 0)
        return s.d->size == 0;
    int pos = d->size - s.d->size;
    if (pos < 0)
        return false;
    if (cs == Qt::CaseSensitive) {
        return memcmp((char*)&d->data[pos], (char*)s.d->data, s.d->size*sizeof(QChar)) == 0;
    } else {
        uint last = 0;
        uint olast = 0;
        for (int i = 0; i < s.length(); i++)
            if (foldCase(d->data[pos+i], last) != foldCase(s.d->data[i], olast))
                return false;
    }
    return true;
}

/*!
    \overload
*/
bool QString::endsWith(const QLatin1String& s, Qt::CaseSensitivity cs) const
{
    if (d == &shared_null)
        return (s.latin1() == 0);
    if (d->size == 0)
        return !s.latin1() || *s.latin1() == 0;
    int slen = qstrlen(s.latin1());
    int pos = d->size - slen;
    const uchar *latin = (const uchar *)s.latin1();
    if (pos < 0)
        return false;
    if (cs == Qt::CaseSensitive) {
        for (int i = 0; i < slen; i++)
            if (d->data[pos+i] != latin[i])
                return false;
    } else {
        for (int i = 0; i < slen; i++)
            if (foldCase(d->data[pos+i]) != foldCase((ushort)latin[i]))
                return false;
    }
    return true;
}

/*!
  Returns true if the string ends with \a c; otherwise returns
  false.

  \overload
 */
bool QString::endsWith(const QChar &c, Qt::CaseSensitivity cs) const
{
    return d->size
           && (cs == Qt::CaseSensitive
               ? d->data[d->size - 1] == c
               : foldCase(d->data[d->size - 1]) == foldCase(c.unicode()));
}

/*! \fn const char *QString::ascii() const
    \nonreentrant

    Use toAscii() instead.
*/

/*! \fn const char *QString::latin1() const
    \nonreentrant

    Use toLatin1() instead.
*/

/*! \fn const char *QString::utf8() const
    \nonreentrant

    Use toUtf8() instead.
*/

/*! \fn const char *QString::local8Bit() const
    \nonreentrant

    Use toLocal8Bit() instead.
*/

/*!
    Returns a Latin-1 representation of the string as a QByteArray.
    The returned byte array is undefined if the string contains
    non-Latin1 characters.

    \sa fromLatin1(), toAscii(), toUtf8(), toLocal8Bit(), QTextCodec
*/
QByteArray QString::toLatin1() const
{
    QByteArray ba;
    if (d->size) {
        ba.resize(d->size);
        const ushort *i = d->data;
        const ushort *e = d->data + d->size;
        uchar *s = (uchar*) ba.data();
        while (i != e) {
            *s++ = (*i>0xff) ? '?' : (uchar) *i;
            ++i;
        }
    }
    return ba;
}

/*!
    Returns an 8-bit ASCII representation of the string as a QByteArray.

    If a codec has been set using QTextCodec::setCodecForCStrings(),
    it is used to convert Unicode to 8-bit char; otherwise this
    function does the same as toLatin1().

    \sa fromAscii(), toLatin1(), toUtf8(), toLocal8Bit(), QTextCodec
*/
QByteArray QString::toAscii() const
{
#ifndef QT_NO_TEXTCODEC
    if (codecForCStrings)
        return codecForCStrings->fromUnicode(*this);
#endif // QT_NO_TEXTCODEC
    return toLatin1();
}

/*!
    Returns the local 8-bit representation of the string as a
    QByteArray. The returned byte array is undefined if the string
    contains characters not supported by the local 8-bit encoding.

    QTextCodec::codecForLocale() is used to perform the conversion
    from Unicode.

    \sa fromLocal8Bit(), toAscii(), toLatin1(), toUtf8(), QTextCodec
*/
QByteArray QString::toLocal8Bit() const
{
#ifndef QT_NO_TEXTCODEC
    if (QTextCodec::codecForLocale())
        return QTextCodec::codecForLocale()->fromUnicode(*this);
#endif // QT_NO_TEXTCODEC
    return toLatin1();
}

/*!
    Returns a UTF-8 representation of the string as a QByteArray.

    \sa fromUtf8(), toAscii(), toLatin1(), toLocal8Bit(), QTextCodec
*/
QByteArray QString::toUtf8() const
{
    QByteArray ba;
    if (d->size) {
        int l = d->size;
        int rlen = l*3+1;
        ba.resize(rlen);
        uchar *cursor = (uchar*)ba.data();
        const ushort *ch =d->data;
        for (int i=0; i < l; i++) {
            uint u = *ch;
            if (u < 0x80) {
                *cursor++ = (uchar)u;
            } else {
                if (u < 0x0800) {
                    *cursor++ = 0xc0 | ((uchar) (u >> 6));
                } else {
                    if (QChar(u).isHighSurrogate() && i < l-1) {
                        ushort low = ch[1];
                        if (QChar(low).isLowSurrogate()) {
                            ++ch;
                            ++i;
                            u = QChar::surrogateToUcs4(u,low);
                        }
                    }
                    if (u > 0xffff) {
                        *cursor++ = 0xf0 | ((uchar) (u >> 18));
                        *cursor++ = 0x80 | (((uchar) (u >> 12)) & 0x3f);
                    } else {
                        *cursor++ = 0xe0 | ((uchar) (u >> 12));
                    }
                    *cursor++ = 0x80 | (((uchar) (u >> 6)) & 0x3f);
                }
                *cursor++ = 0x80 | ((uchar) (u&0x3f));
            }
            ++ch;
        }
        ba.resize(cursor - (uchar*)ba.constData());
    }
    return ba;
}

/*!
    \since 4.2

    Returns a UCS-4 representation of the string as a QVector<uint>.

    \sa fromUtf8(), toAscii(), toLatin1(), toLocal8Bit(), QTextCodec, fromUcs4(), toWCharArray()
*/
QVector<uint> QString::toUcs4() const
{
    QVector<uint> v(length());
    uint *a = v.data();
    const unsigned short *uc = utf16();
    for (int i = 0; i < length(); ++i) {
        uint u = uc[i];
        if (QChar(u).isHighSurrogate() && i < length()-1) {
            ushort low = uc[i+1];
            if (QChar(low).isLowSurrogate()) {
                ++i;
                u = QChar::surrogateToUcs4(u, low);
            }
        }
        *a = u;
        ++a;
    }
    v.resize(a - v.data());
    return v;
}

QString::Data *QString::fromLatin1_helper(const char *str, int size)
{
    Data *d;
    if (!str) {
        d = &shared_null;
        d->ref.ref();
    } else if (size == 0 || (!*str && size < 0)) {
        d = &shared_empty;
        d->ref.ref();
    } else {
        if (size < 0)
            size = qstrlen(str);
        d = static_cast<Data *>(qMalloc(sizeof(Data) + size * sizeof(QChar)));
        d->ref = 1;
        d->alloc = d->size = size;
        d->clean = d->asciiCache = d->simpletext = d->righttoleft = d->capacity = 0;
        d->data = d->array;
        ushort *i = d->data;
        d->array[size] = '\0';
        while (size--)
           *i++ = (uchar)*str++;
    }
    return d;
}

QString::Data *QString::fromAscii_helper(const char *str, int size)
{
#ifndef QT_NO_TEXTCODEC
    if (codecForCStrings) {
        Data *d;
        if (!str) {
            d = &shared_null;
            d->ref.ref();
        } else if (size == 0 || (!*str && size < 0)) {
            d = &shared_empty;
            d->ref.ref();
        } else {
            if (size < 0)
                size = qstrlen(str);
            QString s = codecForCStrings->toUnicode(str, size);
            d = s.d;
            d->ref.ref();
        }
        return d;
    }
#endif
    return fromLatin1_helper(str, size);
}

/*!
    Returns a QString initialized with the first \a size characters
    of the Latin-1 string \a str.

    If \a size is -1 (the default), it is taken to be qstrlen(\a
    str).

    \sa toLatin1(), fromAscii(), fromUtf8(), fromLocal8Bit()
*/
QString QString::fromLatin1(const char *str, int size)
{
    return QString(fromLatin1_helper(str, size), 0);
}


#ifdef QT3_SUPPORT

/*!
  \internal
*/
const char *QString::ascii_helper() const
{
    if (!asciiCache)
        asciiCache = new QHash<void *, QByteArray>();

    d->asciiCache = true;
    QByteArray ascii = toAscii();
    QByteArray old = asciiCache->value(d);
    if (old == ascii)
        return old.constData();
    asciiCache->insert(d, ascii);
    return ascii.constData();
}

/*!
  \internal
*/
const char *QString::latin1_helper() const
{
    if (!asciiCache)
        asciiCache = new QHash<void *, QByteArray>();

    d->asciiCache = true;
    QByteArray ascii = toLatin1();
    QByteArray old = asciiCache->value(d);
    if (old == ascii)
        return old.constData();
    asciiCache->insert(d, ascii);
    return ascii.constData();
}

#endif

QT_END_NAMESPACE

#ifdef Q_OS_WIN32
#include "qt_windows.h"

QT_BEGIN_NAMESPACE

QByteArray qt_winQString2MB(const QString& s, int uclen)
{
    if (uclen < 0)
        uclen = s.length();
    if (s.isNull())
        return QByteArray();
    if (uclen == 0)
        return QByteArray("");
    return qt_winQString2MB(s.constData(), uclen);
}

QByteArray qt_winQString2MB(const QChar *ch, int uclen)
{
    if (!ch)
	return QByteArray();
    if (uclen == 0)
        return QByteArray("");
    BOOL used_def;
    QByteArray mb(4096, 0);
    int len;
    while (!(len=WideCharToMultiByte(CP_ACP, 0, (const WCHAR*)ch, uclen,
                mb.data(), mb.size()-1, 0, &used_def)))
    {
        int r = GetLastError();
        if (r == ERROR_INSUFFICIENT_BUFFER) {
            mb.resize(1+WideCharToMultiByte(CP_ACP, 0,
                                (const WCHAR*)ch, uclen,
                                0, 0, 0, &used_def));
                // and try again...
        } else {
#ifndef QT_NO_DEBUG
            // Fail.
            qWarning("WideCharToMultiByte: Cannot convert multibyte text (error %d): %s (UTF-8)",
                r, QString(ch, uclen).toLocal8Bit().data());
#endif
            break;
        }
    }
    mb.resize(len);
    return mb;
}

QString qt_winMB2QString(const char *mb, int mblen)
{
    if (!mb || !mblen)
        return QString();
    const int wclen_auto = 4096;
    WCHAR wc_auto[wclen_auto];
    int wclen = wclen_auto;
    WCHAR *wc = wc_auto;
    int len;
    while (!(len=MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                mb, mblen, wc, wclen)))
    {
        int r = GetLastError();
        if (r == ERROR_INSUFFICIENT_BUFFER) {
            if (wc != wc_auto) {
                qWarning("MultiByteToWideChar: Size changed");
                break;
            } else {
                wclen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                                    mb, mblen, 0, 0);
                wc = new WCHAR[wclen];
                // and try again...
            }
        } else {
            // Fail.
            qWarning("MultiByteToWideChar: Cannot convert multibyte text");
            break;
        }
    }
    if (len <= 0)
        return QString();
    if (wc[len-1] == 0) // len - 1: we don't want terminator
        --len;
    QString s((QChar*)wc, len);
    if (wc != wc_auto)
        delete [] wc;
    return s;
}

QT_END_NAMESPACE

#endif // Q_OS_WIN32

QT_BEGIN_NAMESPACE

/*!
    Returns a QString initialized with the first \a size characters
    of the 8-bit string \a str.

    If \a size is -1 (the default), it is taken to be qstrlen(\a
    str).

    QTextCodec::codecForLocale() is used to perform the conversion
    from Unicode.

    \sa toLocal8Bit(), fromAscii(), fromLatin1(), fromUtf8()
*/
QString QString::fromLocal8Bit(const char *str, int size)
{
    if (!str)
        return QString();
    if (size == 0 || (!*str && size < 0))
        return QLatin1String("");
#if defined(Q_OS_WIN32)
    if(QSysInfo::WindowsVersion & QSysInfo::WV_DOS_based) {
        return qt_winMB2QString(str, size);
    }
#endif
#if !defined(QT_NO_TEXTCODEC)
    if (size < 0)
        size = qstrlen(str);
    QTextCodec *codec = QTextCodec::codecForLocale();
    if (codec)
        return codec->toUnicode(str, size);
#endif // !QT_NO_TEXTCODEC
    return fromLatin1(str, size);
}

/*!
    Returns a QString initialized with the first \a size characters
    of the 8-bit ASCII string \a str.

    If \a size is -1 (the default), it is taken to be qstrlen(\a
    str).

    If a codec has been set using QTextCodec::setCodecForCStrings(),
    it is used to convert \a str to Unicode; otherwise this function
    does the same as fromLatin1().

    \sa toAscii(), fromLatin1(), fromUtf8(), fromLocal8Bit()
*/
QString QString::fromAscii(const char *str, int size)
{
    return QString(fromAscii_helper(str, size), 0);
}

/*!
    Returns a QString initialized with the first \a size bytes
    of the UTF-8 string \a str.

    If \a size is -1 (the default), it is taken to be qstrlen(\a
    str).

    \sa toUtf8(), fromAscii(), fromLatin1(), fromLocal8Bit()
*/
QString QString::fromUtf8(const char *str, int size)
{
    if (!str)
        return QString();
    if (size < 0)
        size = qstrlen(str);

    QString result;
    result.resize(size); // worst case
    ushort *qch = result.d->data;
    uint uc = 0;
    uint min_uc = 0;
    int need = 0;
    int error = -1;
    uchar ch;
    int i = 0;

    // skip utf8-encoded byte order mark
    if (size >= 3
        && (uchar)str[0] == 0xef && (uchar)str[1] == 0xbb && (uchar)str[2] == 0xbf)
        i += 3;

    for (; i < size; ++i) {
        ch = str[i];
        if (need) {
            if ((ch&0xc0) == 0x80) {
                uc = (uc << 6) | (ch & 0x3f);
                need--;
                if (!need) {
                    if (uc > 0xffff && uc < 0x110000) {
                        // surrogate pair
                        *qch++ = QChar::highSurrogate(uc);
                        uc = QChar::lowSurrogate(uc);
                    } else if ((uc < min_uc) || (uc >= 0xd800 && uc <= 0xdfff) || (uc >= 0xfffe)) {
			// overlong seqence, UTF16 surrogate or BOM
                        uc = QChar::ReplacementCharacter;
                    }
                    *qch++ = uc;
                }
            } else {
                i = error;
                need = 0;
                *qch++ = QChar::ReplacementCharacter;
            }
        } else {
            if (ch < 128) {
                *qch++ = ch;
            } else if ((ch & 0xe0) == 0xc0) {
                uc = ch & 0x1f;
                need = 1;
                error = i;
                min_uc = 0x80;
            } else if ((ch & 0xf0) == 0xe0) {
                uc = ch & 0x0f;
                need = 2;
                error = i;
                min_uc = 0x800;
            } else if ((ch&0xf8) == 0xf0) {
                uc = ch & 0x07;
                need = 3;
                error = i;
                min_uc = 0x10000;
            } else {
                // Error
                *qch++ = QChar::ReplacementCharacter;
            }
        }
    }
    if (need) {
        // we have some invalid characters remaining we need to add to the string
        for (int i = error; i < size; ++i)
            *qch++ = QChar::ReplacementCharacter;
    }

    result.truncate(qch - result.d->data);
    return result;
}

/*!
    Returns a QString initialized with the first \a size characters
    of the Unicode string \a unicode (ISO-10646-UTF-16 encoded).

    If \a size is -1 (the default), \a unicode must be terminated
    with a 0.

    QString makes a deep copy of the Unicode data.

    \sa utf16(), setUtf16()
*/
QString QString::fromUtf16(const ushort *unicode, int size)
{
    if (!unicode)
        return QString();
    if (size < 0) {
        size = 0;
        while (unicode[size] != 0)
            ++size;
    }
    return QString((const QChar *)unicode, size);
}


/*!
    \since 4.2

    Returns a QString initialized with the first \a size characters
    of the Unicode string \a unicode (ISO-10646-UCS-4 encoded).

    If \a size is -1 (the default), \a unicode must be terminated
    with a 0.

    \sa toUcs4(), fromUtf16(), utf16(), setUtf16(), fromWCharArray()
*/
QString QString::fromUcs4(const uint *unicode, int size)
{
    if (!unicode)
        return QString();
    if (size < 0) {
        size = 0;
        while (unicode[size] != 0)
            ++size;
    }

    QString s;
    s.resize(size*2); // worst case
    ushort *uc = s.d->data;
    for (int i = 0; i < size; ++i) {
        uint u = unicode[i];
        if (u > 0xffff) {
            // decompose into a surrogate pair
            *uc++ = QChar::highSurrogate(u);
            u = QChar::lowSurrogate(u);
        }
        *uc++ = u;
    }
    s.resize(uc - s.d->data);
    return s;
}

/*!
    Resizes the string to \a size characters and copies \a unicode
    into the string.

    If \a unicode is 0, nothing is copied, but the string is still
    resized to \a size.

    \sa unicode(), setUtf16()
*/
QString& QString::setUnicode(const QChar *unicode, int size)
{
     resize(size);
     if (unicode && size)
         memcpy(d->data, unicode, size * sizeof(QChar));
     return *this;
}

/*!
    \fn QString &QString::setUtf16(const ushort *unicode, int size)

    Resizes the string to \a size characters and copies \a unicode
    into the string.

    If \a unicode is 0, nothing is copied, but the string is still
    resized to \a size.

    \sa utf16(), setUnicode()
*/

/*!
    Returns a string that has whitespace removed from the start
    and the end, and that has each sequence of internal whitespace
    replaced with a single space.

    Whitespace means any character for which QChar::isSpace() returns
    true. This includes the ASCII characters '\\t', '\\n', '\\v',
    '\\f', '\\r', and ' '.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::simplifiedFunction()
    \skipto QString str
    \printuntil // str == "lots of whitespace"

    \sa trimmed()
*/
QString QString::simplified() const
{
    if (d->size == 0)
        return *this;
    QString result;
    result.resize(d->size);
    const QChar *from = (const QChar*) d->data;
    const QChar *fromend = (const QChar*) from+d->size;
    int outc=0;
    QChar *to   = (QChar*) result.d->data;
    for (;;) {
        while (from!=fromend && from->isSpace())
            from++;
        while (from!=fromend && !from->isSpace())
            to[outc++] = *from++;
        if (from!=fromend)
            to[outc++] = QLatin1Char(' ');
        else
            break;
    }
    if (outc > 0 && to[outc-1] == QLatin1Char(' '))
        outc--;
    result.truncate(outc);
    return result;
}

/*!
    Returns a string that has whitespace removed from the start and
    the end.

    Whitespace means any character for which QChar::isSpace() returns
    true. This includes the ASCII characters '\\t', '\\n', '\\v',
    '\\f', '\\r', and ' '.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::trimmedFunction()
    \skipto QString str
    \printuntil // str == "lots\t of\nwhitespace"

    Unlike simplified(), trimmed() leaves internal whitespace alone.

    \sa simplified()
*/
QString QString::trimmed() const
{
    if (d->size == 0)
        return *this;
    const QChar *s = (const QChar*)d->data;
    if (!s->isSpace() && !s[d->size-1].isSpace())
        return *this;
    int start = 0;
    int end = d->size - 1;
    while (start<=end && s[start].isSpace())  // skip white space from start
        start++;
    if (start <= end) {                          // only white space
        while (end && s[end].isSpace())           // skip white space from end
            end--;
    }
    int l = end - start + 1;
    if (l <= 0) {
        shared_empty.ref.ref();
        return QString(&shared_empty, 0);
    }
    return QString(s + start, l);
}

/*! \fn const QChar QString::at(int position) const

    Returns the character at the given index \a position in the
    string.

    The \a position must be a valid index position in the string
    (i.e., 0 <= \a position < size()).

    \sa operator[]()
*/

/*!
    \fn QCharRef QString::operator[](int position)

    Returns the character at the specified \a position in the string as a
    modifiable reference.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::arrayOperator()
    \skipto QString str
    \printuntil str[0] = QChar('_')

    The return value is of type QCharRef, a helper class for QString.
    When you get an object of type QCharRef, you can use it as if it
    were a QChar &. If you assign to it, the assignment will apply to
    the character in the QString from which you got the reference.

    \sa at()
*/

/*!
    \fn const QChar QString::operator[](int position) const

    \overload
*/

/*! \fn QCharRef QString::operator[](uint position)

\overload

Returns the character at the specified \a position in the string as a
modifiable reference. Equivalent to \c at(position).
*/

/*! \fn const QChar QString::operator[](uint position) const

\overload
*/

/*!
    \fn void QString::truncate(int position)

    Truncates the string at the given \a position index.

    If the specified \a position index is beyond the end of the
    string, nothing happens.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::truncateFunction()
    \skipto QString str
    \printuntil // str == "Vlad"

    \sa chop(), resize(), left()
*/

void QString::truncate(int pos)
{
    if (pos < d->size)
        resize(pos);
}


/*!
    Removes \a n characters from the end of the string.

    If \a n is greater than size(), the result is an empty string.

    Example:
    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::chopFunction()
    \skipto QString
    \printuntil  // str == "LOGOUT"

    If you want to remove characters from the \e beginning of the
    string, use remove() instead.

    \sa truncate(), resize(), remove()
*/
void QString::chop(int n)
{
    if (n > 0)
        resize(d->size - n);
}

/*!
    Sets every character in the string to character \a ch. If \a size
    is different from -1 (the default), the string is resized to \a
    size beforehand.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::fillFunction()
    \skipto QString str
    \printuntil // str == "AA"

    \sa resize()
*/

QString& QString::fill(QChar ch, int size)
{
    resize(size < 0 ? d->size : size);
    if (d->size) {
        QChar *i = (QChar*)d->data + d->size;
        QChar *b = (QChar*)d->data;
        while (i != b)
           *--i = ch;
    }
    return *this;
}

/*!
    \fn int QString::length() const

    Returns the number of characters in this string.  Equivalent to
    size().

    \sa setLength()
*/

/*!
    \fn int QString::size() const

    Returns the number of characters in this string.

    The last character in the string is at position size() - 1. In
    addition, QString ensures that the character at position size()
    is always '\\0', so that you can use the return value of data()
    and constData() as arguments to functions that expect
    '\\0'-terminated strings.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::sizeFunction()
    \skipto QString str
    \printuntil str.data()[5]

    \sa isEmpty(), resize()
*/

/*! \fn bool QString::isNull() const

    Returns true if this string is null; otherwise returns false.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::isNullFunction()
    \skipto QString().isNull()
    \printuntil QString("abc").isNull()

    Qt makes a distinction between null strings and empty strings for
    historical reasons. For most applications, what matters is
    whether or not a string contains any data, and this can be
    determined using the isEmpty() function.

    \sa isEmpty()
*/

/*! \fn bool QString::isEmpty() const

    Returns true if the string has no characters; otherwise returns
    false.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::isEmptyFunction()
    \skipto QString().isEmpty()
    \printuntil QString("abc").isEmpty()

    \sa size()
*/

/*! \fn QString &QString::operator+=(const QString &other)

    Appends the string \a other onto the end of this string and
    returns a reference to this string.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::plusEqualOperator()
    \skipto QString x
    \printuntil // x == "freedom"

    This operation is typically very fast (\l{constant time}),
    because QString preallocates extra space at the end of the string
    data so it can grow without reallocating the entire string each
    time.

    \sa append(), prepend()
*/

/*! \fn QString &QString::operator+=(const QLatin1String &str)

    \overload

    Appends the Latin-1 string \a str to this string.
*/

/*! \fn QString &QString::operator+=(const QByteArray &ba)

    \overload

    Appends the byte array \a ba to this string. The byte array is
    converted to Unicode using the fromAscii() function.

    You can disable this function by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn QString &QString::operator+=(const char *str)

    \overload

    Appends the string \a str to this string. The const char pointer
    is converted to Unicode using the fromAscii() function.

    You can disable this function by defining \c QT_NO_CAST_FROM_ASCII
    when you compile your applications. This can be useful if you want
    to ensure that all user-visible strings go through QObject::tr(),
    for example.
*/

/*! \fn QString &QString::operator+=(char ch)

    \overload

    Appends the character \a ch to this string. The character is
    converted to Unicode using the fromAscii() function.

    You can disable this function by defining \c QT_NO_CAST_FROM_ASCII
    when you compile your applications. This can be useful if you want
    to ensure that all user-visible strings go through QObject::tr(),
    for example.
*/

/*! \fn QString &QString::operator+=(QChar ch)

    \overload

    Appends the character \a ch to the string.
*/

/*! \fn QString &QString::operator+=(QChar::SpecialCharacter c)

    \overload

    \internal
*/

/*!
    \fn bool operator==(const char *s1, const QString &s2)

    \overload
    \relates QString

    Returns true if \a s1 is equal to \a s2; otherwise returns false.
    Note that no string is equal to \a s1 being 0.

    Equivalent to \c {s1 != 0 && compare(s1, s2) == 0}.

    \sa QString::compare()
*/

/*!
    \fn bool operator!=(const char *s1, const QString &s2)

    \overload
    \relates QString

    Returns true if \a s1 is not equal to \a s2; otherwise returns false.

    For \a s1 != 0, this is equivalent to \c {compare(} \a s1, \a s2 \c {) !=
    0}. Note that no string is equal to \a s1 being 0.

    \sa QString::compare()
*/

/*!
    \fn bool operator<(const char *s1, const QString &s2)

    \overload
    \relates QString

    Returns true if \a s1 is lexically less than \a s2; otherwise
    returns false.  For \a s1 != 0, this is equivalent to \c
    {compare(s1, s2) < 0}.

    The comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would
    expect. Consider sorting user-interface strings using the
    QString::localeAwareCompare() function.

    \sa QString::compare()
*/

/*!
    \fn bool operator<=(const char *s1, const QString &s2)

    \overload
    \relates QString

    Returns true if \a s1 is lexically less than or equal to \a s2;
    otherwise returns false.  For \a s1 != 0, this is equivalent to \c
    {compare(s1, s2) <= 0}.

    The comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would
    expect. Consider sorting user-interface strings with
    QString::localeAwareCompare().

    \sa QString::compare()
*/

/*!
    \fn bool operator>(const char *s1, const QString &s2)

    \overload
    \relates QString

    Returns true if \a s1 is lexically greater than \a s2; otherwise
    returns false.  Equivalent to \c {compare(s1, s2) > 0}.

    The comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would
    expect. Consider sorting user-interface strings using the
    QString::localeAwareCompare() function.

    \sa QString::compare()
*/

/*!
    \fn bool operator>=(const char *s1, const QString &s2)

    \overload
    \relates QString

    Returns true if \a s1 is lexically greater than or equal to \a s2;
    otherwise returns false.  For \a s1 != 0, this is equivalent to \c
    {compare(s1, s2) >= 0}.

    The comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would
    expect. Consider sorting user-interface strings using the
    QString::localeAwareCompare() function.
*/

/*!
    \fn const QString operator+(const QString &s1, const QString &s2)
    \relates QString

    Returns a string which is the result of concatenating \a s1 and \a
    s2.
*/

/*!
    \fn const QString operator+(const QString &s1, const char *s2)

    \overload
    \relates QString

    Returns a string which is the result of concatenating \a s1 and \a
    s2 (\a s2 is converted to Unicode using the QString::fromAscii()
    function).

    \sa QString::fromAscii()
*/

/*!
    \fn const QString operator+(const char *s1, const QString &s2)

    \overload
    \relates QString

    Returns a string which is the result of concatenating \a s1 and \a
    s2 (\a s1 is converted to Unicode using the QString::fromAscii()
    function).

    \sa QString::fromAscii()
*/

/*!
    \fn const QString operator+(const QString &s, char ch)

    \overload
    \relates QString

    Returns a string which is the result of concatenating the string
    \a s and the character \a ch.
*/

/*!
    \fn const QString operator+(char ch, const QString &s)

    \overload
    \relates QString

    Returns a string which is the result of concatenating the
    character \a ch and the string \a s.
*/

/*!
    \fn int QString::compare(const QString & s1, const QString & s2, Qt::CaseSensitivity cs)
    \since 4.2

    Compares \a s1 with \a s2 and returns an integer less than, equal to, or greater
    than zero if \a s1 is less than, equal to, or greater than \a s2.

    If \a cs is Qt::CaseSensitive, the comparison is case sensitive;
    otherwise the comparison is case insensitive.

    Case sensitive comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would expect.
    Consider sorting user-visible strings with localeAwareCompare().

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::compareSensitiveFunction
    \skipto int x
    \printuntil int z

    \sa operator==(), operator<(), operator>()
*/

/*!
    \fn int QString::compare(const QString & s1, const QString & s2)

    \overload

    Performs a case sensitive compare of \a s1 and \a s2.
*/

/*!
    \fn int QString::compare(const QString &s1, const QLatin1String &s2,
                             Qt::CaseSensitivity cs)
    \since 4.2
    \overload

    Performs a comparison of \a s1 and \a s2, using the case
    sensitivity setting \a cs.
*/

/*!
    \fn int QString::compare(const QLatin1String &s1, const QString &s2,
                             Qt::CaseSensitivity cs = Qt::CaseSensitive)

    \since 4.2
    \overload

    Performs a comparison of \a s1 and \a s2, using the case
    sensitivity setting \a cs.
*/

/*!
    \overload

    Lexically compares this string with the \a other string and
    returns an integer less than, equal to, or greater than zero if
    this string is less than, equal to, or greater than the other
    string.

    Equivalent to \c {compare(*this, other)}.
*/
int QString::compare(const QString &other) const
{
    return ucstrcmp(*this, other);
}

/*!
    \overload
    \since 4.2

    Same as compare(*this, \a other, \a cs).
*/
int QString::compare(const QString &other, Qt::CaseSensitivity cs) const
{
    if (cs == Qt::CaseSensitive)
        return ucstrcmp(*this, other);
    return ucstricmp(d->data, d->data + d->size, other.d->data, other.d->data + other.d->size);
}

/*!
    \overload
    \since 4.2

    Same as compare(*this, \a other, \a cs).
*/
int QString::compare(const QLatin1String &other, Qt::CaseSensitivity cs) const
{
    const ushort *uc = d->data;
    const ushort *e = uc + d->size;
    const uchar *c = (uchar *)other.latin1();

    if (!c)
        return d->size;

    if (cs == Qt::CaseSensitive) {
        while (uc != e && *c && *uc == *c)
            uc++, c++;

        return *uc - *c;
    } else {
        return ucstricmp(d->data, d->data + d->size, c);
    }
}

/*!
    \fn int QString::localeAwareCompare(const QString & s1, const QString & s2)

    Compares \a s1 with \a s2 and returns an integer less than, equal
    to, or greater than zero if \a s1 is less than, equal to, or
    greater than \a s2.

    The comparison is performed in a locale- and also
    platform-dependent manner. Use this function to present sorted
    lists of strings to the user.

    On Mac OS X since Qt 4.3, this function compares according the
    "Order for sorted lists" setting in the International prefereces panel.

    \sa compare(), QTextCodec::locale()
*/

/*!
    \overload

    Compares this string with the \a other string and returns an
    integer less than, equal to, or greater than zero if this string
    is less than, equal to, or greater than the \a other string.

    Same as \c {localeAwareCompare(*this, other)}.
*/

#if !defined(CSTR_LESS_THAN)
#define CSTR_LESS_THAN    1
#define CSTR_EQUAL        2
#define CSTR_GREATER_THAN 3
#endif

int QString::localeAwareCompare(const QString &other) const
{
    // do the right thing for null and empty
    if (isEmpty() || other.isEmpty())
        return compare(other);

#if defined(Q_OS_WIN32)
    int res;
    QT_WA({
        const TCHAR* s1 = (TCHAR*)utf16();
        const TCHAR* s2 = (TCHAR*)other.utf16();
        res = CompareStringW(GetThreadLocale(), 0, s1, length(), s2, other.length());
    } , {
        QByteArray s1 = toLocal8Bit();
        QByteArray s2 = other.toLocal8Bit();
        res = CompareStringA(GetThreadLocale(), 0, s1.data(), s1.length(), s2.data(), s2.length());
    });

    switch (res) {
    case CSTR_LESS_THAN:
        return -1;
    case CSTR_GREATER_THAN:
        return 1;
    default:
        return 0;
    }
#elif defined (Q_OS_MAC)
    // Use CFStringCompare for comparing strings on Mac. This makes Qt order
    // strings the same way as native applications do, and also respects
    // the "Order for sorted lists" setting in the International preferences
    // panel.
    const CFStringRef thisString =
        CFStringCreateWithCharactersNoCopy(kCFAllocatorDefault,
            reinterpret_cast<const UniChar *>(this->unicode()), this->length(), kCFAllocatorNull);
    const CFStringRef otherString =
        CFStringCreateWithCharactersNoCopy(kCFAllocatorDefault,
            reinterpret_cast<const UniChar *>(other.unicode()), other.length(), kCFAllocatorNull);

    const int result = CFStringCompare(thisString, otherString, kCFCompareLocalized);
    CFRelease(thisString);
    CFRelease(otherString);
    return result;
#elif defined(Q_OS_UNIX)
    // declared in <string.h>
    int delta = strcoll(toLocal8Bit(), other.toLocal8Bit());
    if (delta == 0)
        delta = ucstrcmp(*this, other);
    return delta;
#else
    return ucstrcmp(*this, other);
#endif
}


/*!
    \fn const QChar *QString::unicode() const

    Returns a '\\0'-terminated Unicode representation of the string.
    The result remains valid until the string is modified.

    \sa utf16()
*/

/*!
    \fn const ushort *QString::utf16() const

    Returns the QString as a '\\0\'-terminated array of unsigned
    shorts. The result remains valid until the string is modified.

    \sa unicode()
*/

const ushort *QString::utf16() const
{
    if (d->data != d->array) {
        QString *that = const_cast<QString*>(this);
        that->realloc();   // ensure '\\0'-termination for ::fromRawData strings
        return that->d->data;
    }
    return d->array;
}

/*!
    Returns a string of size \a width that contains this string
    padded by the \a fill character.

    If \a truncate is false and the size() of the string is more than
    \a width, then the returned string is a copy of the string.

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::leftJustifiedFunction()
    \skipto QString s
    \printuntil QString t

    If \a truncate is true and the size() of the string is more than
    \a width, then any characters in a copy of the string after
    position \a width are removed, and the copy is returned.

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::leftJustifiedFunction()
    \skipto QString str
    \printuntil str.leftJustified(5, '.', true)

    \sa rightJustified()
*/

QString QString::leftJustified(int width, QChar fill, bool truncate) const
{
    QString result;
    int len = length();
    int padlen = width - len;
    if (padlen > 0) {
        result.resize(len+padlen);
        if (len)
            memcpy(result.d->data, d->data, sizeof(QChar)*len);
        QChar *uc = (QChar*)result.d->data + len;
        while (padlen--)
           * uc++ = fill;
    } else {
        if (truncate)
            result = left(width);
        else
            result = *this;
    }
    return result;
}

/*!
    Returns a string of size() \a width that contains the \a fill
    character followed by the string. For example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::rightJustifiedFunction()
    \skipto QString s
    \printuntil t == "...apple"

    If \a truncate is false and the size() of the string is more than
    \a width, then the returned string is a copy of the string.

    If \a truncate is true and the size() of the string is more than
    \a width, then the resulting string is truncated at position \a
    width.

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::rightJustifiedFunction()
    \skipto QString str
    \printuntil str == "Pinea"

    \sa leftJustified()
*/

QString QString::rightJustified(int width, QChar fill, bool truncate) const
{
    QString result;
    int len = length();
    int padlen = width - len;
    if (padlen > 0) {
        result.resize(len+padlen);
        QChar *uc = (QChar*)result.d->data;
        while (padlen--)
           * uc++ = fill;
        if (len)
            memcpy(uc, d->data, sizeof(QChar)*len);
    } else {
        if (truncate)
            result = left(width);
        else
            result = *this;
    }
    return result;
}

/*!
    Returns a lowercase copy of the string.

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::toLowerFunction()
    \skipto QString str
    \printuntil str.toLower()

    \sa toUpper()
*/

QString QString::toLower() const
{
    const ushort *p = d->data;
    if (!p)
        return *this;
    if (!d->size)
        return *this;

    const ushort *e = d->data + d->size;

    // this avoids one out of bounds check in the loop
    if (QChar(*p).isLowSurrogate())
        ++p;

    while (p != e) {
        uint c = *p;
        if (QChar(c).isLowSurrogate() && QChar(*(p - 1)).isHighSurrogate())
            c = QChar::surrogateToUcs4(*(p - 1), c);
        const QUnicodeTables::Properties *prop = qGetProp(c);
        if (prop->lowerCaseDiff || prop->lowerCaseSpecial) {
            QString s;
            s.resize(d->size);
            memcpy(s.d->data, d->data, (p - d->data)*sizeof(ushort));
            ushort *pp = s.d->data + (p - d->data);
            while (p < e) {
                uint c = *p;
                if (QChar(c).isLowSurrogate() && QChar(*(p - 1)).isHighSurrogate())
                    c = QChar::surrogateToUcs4(*(p - 1), c);
                prop = qGetProp(c);
                if (prop->lowerCaseSpecial) {
                    int pos = pp - s.d->data;
                    s.resize(s.d->size + SPECIAL_CASE_MAX_LEN);
                    pp = s.d->data + pos;
                    const ushort *specialCase = specialCaseMap + prop->lowerCaseDiff;
                    while (*specialCase)
                        *pp++ = *specialCase++;
                } else {
                    *pp++ = *p + prop->lowerCaseDiff;
                }
                ++p;
            }
            s.truncate(pp - s.d->data);
            return s;
        }
        ++p;
    }
    return *this;
}

/*!
Returns the case folded equivalent of the string. For most Unicode characters this
is the same as toLowerCase().
*/
QString QString::toCaseFolded() const
{
    if (!d->size)
        return *this;

    const ushort *p = d->data;
    if (!p)
        return *this;

    const ushort *e = d->data + d->size;

    uint last = 0;
    while (p < e) {
        ushort folded = foldCase(*p, last);
        if (folded != *p) {
            QString s(*this);
            s.detach();
            ushort *pp = s.d->data + (p - d->data);
            const ushort *ppe = s.d->data + s.d->size;
            last = pp > s.d->data ? *(pp - 1) : 0;
            while (pp < ppe) {
                *pp = foldCase(*pp, last);
                ++pp;
            }
            return s;
        }
        p++;
    }
    return *this;
}

/*!
    Returns an uppercase copy of the string.

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::toUpperFunction()
    \skipto QString str
    \printuntil str.toUpper()

    \sa toLower()
*/

QString QString::toUpper() const
{
    const ushort *p = d->data;
    if (!p)
        return *this;
    if (!d->size)
        return *this;

    const ushort *e = d->data + d->size;

    // this avoids one out of bounds check in the loop
    if (QChar(*p).isLowSurrogate())
        ++p;

    while (p != e) {
        uint c = *p;
        if (QChar(c).isLowSurrogate() && QChar(*(p - 1)).isHighSurrogate())
            c = QChar::surrogateToUcs4(*(p - 1), c);
        const QUnicodeTables::Properties *prop = qGetProp(c);
        if (prop->upperCaseDiff || prop->upperCaseSpecial) {
            QString s;
            s.resize(d->size);
            memcpy(s.d->data, d->data, (p - d->data)*sizeof(ushort));
            ushort *pp = s.d->data + (p - d->data);
            while (p < e) {
                uint c = *p;
                if (QChar(c).isLowSurrogate() && QChar(*(p - 1)).isHighSurrogate())
                    c = QChar::surrogateToUcs4(*(p - 1), c);
                prop = qGetProp(c);
                if (prop->upperCaseSpecial) {
                    int pos = pp - s.d->data;
                    s.resize(s.d->size + SPECIAL_CASE_MAX_LEN);
                    pp = s.d->data + pos;
                    const ushort *specialCase = specialCaseMap + prop->upperCaseDiff;
                    while (*specialCase)
                        *pp++ = *specialCase++;
                } else {
                    *pp++ = *p + prop->upperCaseDiff;
                }
                ++p;
            }
            s.truncate(pp - s.d->data);
            return s;
        }
        ++p;
    }
    return *this;
}


/*!
    Safely builds a formatted string from the format string \a cformat
    and an arbitrary list of arguments.

    The %lc escape sequence expects a unicode character of type ushort
    (as returned by QChar::unicode()). The %ls escape sequence expects
    a pointer to a zero-terminated array of unicode characters of type
    ushort (as returned by QString::utf16()).

    The format string supports most of the conversion specifiers
    provided by printf() in the standard C++ library. It doesn't
    honor the length modifiers (e.g. \c h for \c short, \c ll for
    \c{long long}). If you need those, use the standard snprintf()
    function instead:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::splitFunction()
    \skipto size_t
    \printuntil QString str

    \warning We do not recommend using QString::sprintf() in new Qt
    code. Instead, consider using QTextStream or arg(), both of
    which support Unicode strings seamlessly and are type-safe.
    Here's an example that uses QTextStream:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::splitFunction()
    \skipto QString result
    \printuntil // result == "pi = 3.14"

    For \l {QObject::tr()}{translations}, especially if the strings
    contains more than one escape sequence, you should consider using
    the arg() function instead. This allows the order of the
    replacements to be controlled by the translator.

    \sa arg()
*/

QString &QString::sprintf(const char *cformat, ...)
{
    va_list ap;
    va_start(ap, cformat);
    QString &s = vsprintf(cformat, ap);
    va_end(ap);
    return s;
}

/*!
    Equivalent method to sprintf(), but takes a va_list \a ap
    instead a list of variable arguments. See the sprintf()
    documentation for an explanation of \a cformat.

    This method does not call the va_end macro, the caller
    is responsible to call va_end on \a ap.

    \sa sprintf()
*/

QString &QString::vsprintf(const char* cformat, va_list ap)
{
    QLocale locale(QLocale::C);

    if (!cformat || !*cformat) {
        // Qt 1.x compat
        *this = fromLatin1("");
        return *this;
    }

    // Parse cformat

    QString result;
    const char *c = cformat;
    for (;;) {
        // Copy non-escape chars to result
        while (*c != '\0' && *c != '%')
            result.append(QLatin1Char(*c++));

        if (*c == '\0')
            break;

        // Found '%'
        const char *escape_start = c;
        ++c;

        if (*c == '\0') {
            result.append(QLatin1Char('%')); // a % at the end of the string - treat as non-escape text
            break;
        }
        if (*c == '%') {
            result.append(QLatin1Char('%')); // %%
            ++c;
            continue;
        }

        // Parse flag characters
        uint flags = 0;
        bool no_more_flags = false;
        do {
            switch (*c) {
                case '#': flags |= QLocalePrivate::Alternate; break;
                case '0': flags |= QLocalePrivate::ZeroPadded; break;
                case '-': flags |= QLocalePrivate::LeftAdjusted; break;
                case ' ': flags |= QLocalePrivate::BlankBeforePositive; break;
                case '+': flags |= QLocalePrivate::AlwaysShowSign; break;
                case '\'': flags |= QLocalePrivate::ThousandsGroup; break;
                default: no_more_flags = true; break;
            }

            if (!no_more_flags)
                ++c;
        } while (!no_more_flags);

        if (*c == '\0') {
            result.append(QLatin1String(escape_start)); // incomplete escape, treat as non-escape text
            break;
        }

        // Parse field width
        int width = -1; // -1 means unspecified
        if (qIsDigit(*c)) {
            QString width_str;
            while (*c != '\0' && qIsDigit(*c))
                width_str.append(QLatin1Char(*c++));

            // can't be negative - started with a digit
            // contains at least one digit
            width = width_str.toInt();
        }
        else if (*c == '*') {
            width = va_arg(ap, int);
            if (width < 0)
                width = -1; // treat all negative numbers as unspecified
            ++c;
        }

        if (*c == '\0') {
            result.append(QLatin1String(escape_start)); // incomplete escape, treat as non-escape text
            break;
        }

        // Parse precision
        int precision = -1; // -1 means unspecified
        if (*c == '.') {
            ++c;
            if (qIsDigit(*c)) {
                QString precision_str;
                while (*c != '\0' && qIsDigit(*c))
                    precision_str.append(QLatin1Char(*c++));

                // can't be negative - started with a digit
                // contains at least one digit
                precision = precision_str.toInt();
            }
            else if (*c == '*') {
                precision = va_arg(ap, int);
                if (precision < 0)
                    precision = -1; // treat all negative numbers as unspecified
                ++c;
            }
        }

        if (*c == '\0') {
            result.append(QLatin1String(escape_start)); // incomplete escape, treat as non-escape text
            break;
        }

        // Parse the length modifier
        enum LengthMod { lm_none, lm_hh, lm_h, lm_l, lm_ll, lm_L, lm_j, lm_z, lm_t };
        LengthMod length_mod = lm_none;
        switch (*c) {
            case 'h':
                ++c;
                if (*c == 'h') {
                    length_mod = lm_hh;
                    ++c;
                }
                else
                    length_mod = lm_h;
                break;

            case 'l':
                ++c;
                if (*c == 'l') {
                    length_mod = lm_ll;
                    ++c;
                }
                else
                    length_mod = lm_l;
                break;

            case 'L':
                ++c;
                length_mod = lm_L;
                break;

            case 'j':
                ++c;
                length_mod = lm_j;
                break;

            case 'z':
            case 'Z':
                ++c;
                length_mod = lm_z;
                break;

            case 't':
                ++c;
                length_mod = lm_t;
                break;

            default: break;
        }

        if (*c == '\0') {
            result.append(QLatin1String(escape_start)); // incomplete escape, treat as non-escape text
            break;
        }

        // Parse the conversion specifier and do the conversion
        QString subst;
        switch (*c) {
            case 'd':
            case 'i': {
                qint64 i;
                switch (length_mod) {
                    case lm_none: i = va_arg(ap, int); break;
                    case lm_hh: i = va_arg(ap, int); break;
                    case lm_h: i = va_arg(ap, int); break;
                    case lm_l: i = va_arg(ap, long int); break;
                    case lm_ll: i = va_arg(ap, qint64); break;
                    case lm_j: i = va_arg(ap, long int); break;
                    case lm_z: i = va_arg(ap, size_t); break;
                    case lm_t: i = va_arg(ap, int); break;
                    default: i = 0; break;
                }
                subst = locale.d()->longLongToString(i, precision, 10, width, flags);
                ++c;
                break;
            }
            case 'o':
            case 'u':
            case 'x':
            case 'X': {
                quint64 u;
                switch (length_mod) {
                    case lm_none: u = va_arg(ap, uint); break;
                    case lm_hh: u = va_arg(ap, uint); break;
                    case lm_h: u = va_arg(ap, uint); break;
                    case lm_l: u = va_arg(ap, ulong); break;
                    case lm_ll: u = va_arg(ap, quint64); break;
                    default: u = 0; break;
                }

                if (qIsUpper(*c))
                    flags |= QLocalePrivate::CapitalEorX;

                int base = 10;
                switch (qToLower(*c)) {
                    case 'o':
                        base = 8; break;
                    case 'u':
                        base = 10; break;
                    case 'x':
                        base = 16; break;
                    default: break;
                }
                subst = locale.d()->unsLongLongToString(u, precision, base, width, flags);
                ++c;
                break;
            }
            case 'E':
            case 'e':
            case 'F':
            case 'f':
            case 'G':
            case 'g':
            case 'A':
            case 'a': {
                double d;
                if (length_mod == lm_L)
                    d = va_arg(ap, long double); // not supported - converted to a double
                else
                    d = va_arg(ap, double);

                if (qIsUpper(*c))
                    flags |= QLocalePrivate::CapitalEorX;

                QLocalePrivate::DoubleForm form = QLocalePrivate::DFDecimal;
                switch (qToLower(*c)) {
                    case 'e': form = QLocalePrivate::DFExponent; break;
                    case 'a':                             // not supported - decimal form used instead
                    case 'f': form = QLocalePrivate::DFDecimal; break;
                    case 'g': form = QLocalePrivate::DFSignificantDigits; break;
                    default: break;
                }
                subst = locale.d()->doubleToString(d, precision, form, width, flags);
                ++c;
                break;
            }
            case 'c': {
                if (length_mod == lm_l)
                    subst = QChar((ushort) va_arg(ap, int));
                else
                    subst = QLatin1Char((uchar) va_arg(ap, int));
                ++c;
                break;
            }
            case 's': {
                if (length_mod == lm_l) {
                    const ushort *buff = va_arg(ap, const ushort*);
                    const ushort *ch = buff;
                    while (*ch != 0)
                        ++ch;
                    subst.setUtf16(buff, ch - buff);
                } else
                    subst = QString::fromUtf8(va_arg(ap, const char*));
                if (precision != -1)
                    subst.truncate(precision);
                ++c;
                break;
            }
            case 'p': {
                void *arg = va_arg(ap, void*);
#ifdef Q_OS_WIN64
                quint64 i = reinterpret_cast<quint64>(arg);
#else
                quint64 i = reinterpret_cast<unsigned long>(arg);
#endif
                flags |= QLocalePrivate::Alternate;
                subst = locale.d()->unsLongLongToString(i, precision, 16, width, flags);
                ++c;
                break;
            }
            case 'n':
                switch (length_mod) {
                    case lm_hh: {
                        signed char *n = va_arg(ap, signed char*);
                        *n = result.length();
                        break;
                    }
                    case lm_h: {
                        short int *n = va_arg(ap, short int*);
                        *n = result.length();
                            break;
                    }
                    case lm_l: {
                        long int *n = va_arg(ap, long int*);
                        *n = result.length();
                        break;
                    }
                    case lm_ll: {
                        qint64 *n = va_arg(ap, qint64*);
                        volatile uint tmp = result.length(); // egcs-2.91.66 gets internal
                        *n = tmp;                             // compiler error without volatile
                        break;
                    }
                    default: {
                        int *n = va_arg(ap, int*);
                        *n = result.length();
                        break;
                    }
                }
                ++c;
                break;

            default: // bad escape, treat as non-escape text
                for (const char *cc = escape_start; cc != c; ++cc)
                    result.append(QLatin1Char(*cc));
                continue;
        }

        if (flags & QLocalePrivate::LeftAdjusted)
            result.append(subst.leftJustified(width));
        else
            result.append(subst.rightJustified(width));
    }

    *this = result;

    return *this;
}

/*!
    Returns the string converted to a \c{long long} using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If a conversion error occurs, *\a{ok} is set to false; otherwise
    *\a{ok} is set to true.

    If \a base is 0, the C language convention is used: If the string
    begins with "0x", base 16 is used; if the string begins with "0",
    base 8 is used; otherwise, base 10 is used.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::toLongLongFunction()
    \skipto QString str
    \printuntil qint64 dec

    \sa number(), toULongLong(), toInt()
*/

qint64 QString::toLongLong(bool *ok, int base) const
{
#if defined(QT_CHECK_RANGE)
    if (base != 0 && (base < 2 || base > 36)) {
        qWarning("QString::toLongLong: Invalid base (%d)", base);
        base = 10;
    }
#endif

    bool my_ok;
    QLocale def_locale;
    qint64 result = def_locale.d()->stringToLongLong(*this, base, &my_ok, QLocalePrivate::FailOnGroupSeparators);
    if (my_ok) {
        if (ok != 0)
            *ok = true;
        return result;
    }

    QLocale c_locale(QLocale::C);
    return c_locale.d()->stringToLongLong(*this, base, ok, QLocalePrivate::FailOnGroupSeparators);
}

/*!
    Returns the string converted to an \c{unsigned long long} using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If a conversion error occurs, *\a{ok} is set to false; otherwise
    *\a{ok} is set to true.

    If \a base is 0, the C language convention is used: If the string
    begins with "0x", base 16 is used; if the string begins with "0",
    base 8 is used; otherwise, base 10 is used.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::toULongLongFunction()
    \skipto QString str
    \printuntil quint64 dec

    \sa number(), toLongLong()
*/

quint64 QString::toULongLong(bool *ok, int base) const
{
#if defined(QT_CHECK_RANGE)
    if (base != 0 && (base < 2 || base > 36)) {
        qWarning("QString::toULongLong: Invalid base (%d)", base);
        base = 10;
    }
#endif

    bool my_ok;
    QLocale def_locale;
    quint64 result = def_locale.d()->stringToUnsLongLong(*this, base, &my_ok, QLocalePrivate::FailOnGroupSeparators);
    if (my_ok) {
        if (ok != 0)
            *ok = true;
        return result;
    }

    QLocale c_locale(QLocale::C);
    return c_locale.d()->stringToUnsLongLong(*this, base, ok, QLocalePrivate::FailOnGroupSeparators);
}

/*!
    \fn long QString::toLong(bool *ok, int base) const

    Returns the string converted to a \c long using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If a conversion error occurs, *\a{ok} is set to false; otherwise
    *\a{ok} is set to true.

    If \a base is 0, the C language convention is used: If the string
    begins with "0x", base 16 is used; if the string begins with "0",
    base 8 is used; otherwise, base 10 is used.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::toLongFunction()
    \skipto QString str
    \printuntil long dec

    \sa number(), toULong(), toInt()
*/

long QString::toLong(bool *ok, int base) const
{
    qint64 v = toLongLong(ok, base);
    if (v < LONG_MIN || v > LONG_MAX) {
        if (ok)
            *ok = false;
        v = 0;
    }
    return (long)v;
}

/*!
    \fn ulong QString::toULong(bool *ok, int base) const

    Returns the string converted to an \c{unsigned long} using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If a conversion error occurs, *\a{ok} is set to false; otherwise
    *\a{ok} is set to true.

    If \a base is 0, the C language convention is used: If the string
    begins with "0x", base 16 is used; if the string begins with "0",
    base 8 is used; otherwise, base 10 is used.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::toULongFunction()
    \skipto QString str
    \printuntil ulong dec

    \sa number()
*/

ulong QString::toULong(bool *ok, int base) const
{
    quint64 v = toULongLong(ok, base);
    if (v > ULONG_MAX) {
        if (ok)
            *ok = false;
        v = 0;
    }
    return (ulong)v;
}


/*!
    Returns the string converted to an \c int using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If a conversion error occurs, *\a{ok} is set to false; otherwise
    *\a{ok} is set to true.

    If \a base is 0, the C language convention is used: If the string
    begins with "0x", base 16 is used; if the string begins with "0",
    base 8 is used; otherwise, base 10 is used.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::toIntFunction()
    \skipto QString str
    \printuntil int dec

    \sa number(), toUInt(), toDouble()
*/

int QString::toInt(bool *ok, int base) const
{
    qint64 v = toLongLong(ok, base);
    if (v < INT_MIN || v > INT_MAX) {
        if (ok)
            *ok = false;
        v = 0;
    }
    return v;
}

/*!
    Returns the string converted to an \c{unsigned int} using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If a conversion error occurs, *\a{ok} is set to false; otherwise
    *\a{ok} is set to true.

    If \a base is 0, the C language convention is used: If the string
    begins with "0x", base 16 is used; if the string begins with "0",
    base 8 is used; otherwise, base 10 is used.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::toUIntFunction()
    \skipto QString str
    \printuntil uint dec

    \sa number(), toInt()
*/

uint QString::toUInt(bool *ok, int base) const
{
    quint64 v = toULongLong(ok, base);
    if (v > UINT_MAX) {
        if (ok)
            *ok = false;
        v = 0;
    }
    return (uint)v;
}

/*!
    Returns the string converted to a \c short using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If a conversion error occurs, *\a{ok} is set to false; otherwise
    *\a{ok} is set to true.

    If \a base is 0, the C language convention is used: If the string
    begins with "0x", base 16 is used; if the string begins with "0",
    base 8 is used; otherwise, base 10 is used.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::toShortFunction()
    \skipto QString str
    \printuntil short dec

    \sa number(), toUShort(), toInt()
*/

short QString::toShort(bool *ok, int base) const
{
    long v = toLongLong(ok, base);
    if (v < SHRT_MIN || v > SHRT_MAX) {
        if (ok)
            *ok = false;
        v = 0;
    }
    return (short)v;
}

/*!
    Returns the string converted to an \c{unsigned short} using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If a conversion error occurs, *\a{ok} is set to false; otherwise
    *\a{ok} is set to true.

    If \a base is 0, the C language convention is used: If the string
    begins with "0x", base 16 is used; if the string begins with "0",
    base 8 is used; otherwise, base 10 is used.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::toUShortFunction()
    \skipto QString str
    \printuntil ushort dec

    \sa number(), toShort()
*/

ushort QString::toUShort(bool *ok, int base) const
{
    ulong v = toULongLong(ok, base);
    if (v > USHRT_MAX) {
        if (ok)
            *ok = false;
        v = 0;
    }
    return (ushort)v;
}


/*!
    Returns the string converted to a \c double value.

    Returns 0.0 if the conversion fails.

    If a conversion error occurs, \c{*}\a{ok} is set to false;
    otherwise \c{*}\a{ok} is set to true.

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::toDoubleFunction()
    \skipto QString str
    \printuntil double val

    Various string formats for floating point numbers can be converted
    to double values:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::toDoubleFunction()
    \skipto bool ok
    \printuntil toDouble

    This function tries to interpret the string according to the
    current locale. The current locale is determined from the
    system at application startup and can be changed by calling
    QLocale::setDefault(). If the string cannot be interpreted
    according to the current locale, this function falls back
    on the "C" locale.

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::toDoubleFunction()
    \skipto QLocale::setDefault(QLocale::C);
    \printto QLocale::setDefault(QLocale::German);
    \printto QLocale::setDefault(QLocale::C);

    Due to the ambiguity between the decimal point and thousands group
    separator in various locales, this function does not handle
    thousands group separators. If you need to convert such numbers,
    see QLocale::toDouble().

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::toDoubleFunction()
    \skipto QLocale::setDefault(QLocale::C);
    \printuntil d =

    \sa number() QLocale::setDefault() QLocale::toDouble() trimmed()
*/

double QString::toDouble(bool *ok) const
{
    bool my_ok;
    QLocale def_locale;
    double result = def_locale.d()->stringToDouble(*this, &my_ok, QLocalePrivate::FailOnGroupSeparators);
    if (my_ok) {
        if (ok != 0)
            *ok = true;
        return result;
    }

    QLocale c_locale(QLocale::C);
    return c_locale.d()->stringToDouble(*this, ok, QLocalePrivate::FailOnGroupSeparators);
}

/*!
    Returns the string converted to a \c float value.

    If a conversion error occurs, *\a{ok} is set to false; otherwise
    *\a{ok} is set to true. Returns 0.0 if the conversion fails.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::toFloatFunction()
    \skipto QString str1
    \printuntil str2.toFloat(&ok)

    \sa number(), toDouble(), toInt()
*/

#define QT_MAX_FLOAT 3.4028234663852886e+38

float QString::toFloat(bool *ok) const
{
    bool myOk;
    double d = toDouble(&myOk);
    if (!myOk || d > QT_MAX_FLOAT || d < -QT_MAX_FLOAT) {
        if (ok != 0)
            *ok = false;
        return 0.0;
    }
    if (ok != 0)
        *ok = true;
    return (float) d;
}

/*! \fn QString &QString::setNum(int n, int base)

    Sets the string to the printed value of \a n in the specified \a
    base, and returns a reference to the string.

    The base is 10 by default and must be between 2 and 36. For bases
    other than 10, \a n is treated as an unsigned integer.

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::setNumFunction
    \skipto QString str
    \printuntil  str.setNum(1234)
*/

/*! \fn QString &QString::setNum(uint n, int base)

    \overload
*/

/*! \fn QString &QString::setNum(long n, int base)

    \overload
*/

/*! \fn QString &QString::setNum(ulong n, int base)

    \overload
*/

/*!
    \overload
*/
QString &QString::setNum(qlonglong n, int base)
{
#if defined(QT_CHECK_RANGE)
    if (base < 2 || base > 36) {
        qWarning("QString::setNum: Invalid base (%d)", base);
        base = 10;
    }
#endif
    QLocale locale(QLocale::C);
    *this = locale.d()->longLongToString(n, -1, base);
    return *this;
}

/*!
    \overload
*/
QString &QString::setNum(qulonglong n, int base)
{
#if defined(QT_CHECK_RANGE)
    if (base < 2 || base > 36) {
        qWarning("QString::setNum: Invalid base (%d)", base);
        base = 10;
    }
#endif
    QLocale locale(QLocale::C);
    *this = locale.d()->unsLongLongToString(n, -1, base);
    return *this;
}

/*! \fn QString &QString::setNum(short n, int base)

    \overload
*/

/*! \fn QString &QString::setNum(ushort n, int base)

    \overload
*/

/*!
    \fn QString &QString::setNum(double n, char format, int precision)
    \overload

    Sets the string to the printed value of \a n, formatted according
    to the given \a format and \a precision, and returns a reference
    to the string.

    The \a format can be 'f', 'F', 'e', 'E', 'g' or 'G' (see the
    arg() function documentation for an explanation of the formats).

    Unlike QLocale::toString(), this function doesn't honor the
    user's locale settings.
*/

QString &QString::setNum(double n, char f, int prec)
{
    QLocalePrivate::DoubleForm form = QLocalePrivate::DFDecimal;
    uint flags = 0;

    if (qIsUpper(f))
        flags = QLocalePrivate::CapitalEorX;
    f = qToLower(f);

    switch (f) {
        case 'f':
            form = QLocalePrivate::DFDecimal;
            break;
        case 'e':
            form = QLocalePrivate::DFExponent;
            break;
        case 'g':
            form = QLocalePrivate::DFSignificantDigits;
            break;
        default:
#if defined(QT_CHECK_RANGE)
            qWarning("QString::setNum: Invalid format char '%c'", f);
#endif
            break;
    }

    QLocale locale(QLocale::C);
    *this = locale.d()->doubleToString(n, prec, form, -1, flags);
    return *this;
}

/*!
    \fn QString &QString::setNum(float n, char format, int precision)
    \overload

    Sets the string to the printed value of \a n, formatted according
    to the given \a format and \a precision, and returns a reference
    to the string.
*/


/*!
    \fn QString QString::number(long n, int base)

    Returns a string equivalent of the number \a n according to the
    specified \a base.

    The base is 10 by default and must be between 2
    and 36. For bases other than 10, \a n is treated as an
    unsigned integer.

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::numberFunction()
    \skipto long a
    \printuntil QString::number(a, 16).toUpper()

    \sa setNum()
*/

QString QString::number(long n, int base)
{
    QString s;
    s.setNum(n, base);
    return s;
}

/*!
  \fn QString QString::number(ulong n, int base)

    \overload
*/
QString QString::number(ulong n, int base)
{
    QString s;
    s.setNum(n, base);
    return s;
}

/*!
    \overload
*/
QString QString::number(int n, int base)
{
    QString s;
    s.setNum(n, base);
    return s;
}

/*!
    \overload
*/
QString QString::number(uint n, int base)
{
    QString s;
    s.setNum(n, base);
    return s;
}

/*!
    \overload
*/
QString QString::number(qlonglong n, int base)
{
    QString s;
    s.setNum(n, base);
    return s;
}

/*!
    \overload
*/
QString QString::number(qulonglong n, int base)
{
    QString s;
    s.setNum(n, base);
    return s;
}


/*!
    \fn QString QString::number(double n, char format, int precision)
    \overload

    Returns a string equivalent of the number \a n, formatted
    according to the specified \a format and \a precision. The \a
    format can be 'f', 'F', 'e', 'E', 'g' or 'G' (see the
    arg() function documentation for an explanation of the formats).

    Unlike QLocale::toString(), this function does not honor the
    user's locale settings.

    \sa setNum(), QLocale::toString()
*/
QString QString::number(double n, char f, int prec)
{
    QString s;
    s.setNum(n, f, prec);
    return s;
}

/*!
    Splits the string into substrings wherever \a sep occurs, and
    returns the list of those strings. If \a sep does not match
    anywhere in the string, split() returns a single-element list
    containing this string.

    \a cs specifies whether \a sep should be matched case
    sensitively or case insensitively.

    If \a behavior is QString::SkipEmptyParts, empty entries don't
    appear in the result. By default, empty entries are kept.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::splitCaseSensitiveFunction()
    \skipto QString str
    \printuntil // list2: [ "a", "b", "c" ]

    \sa QStringList::join(), section()
*/
QStringList QString::split(const QString &sep, SplitBehavior behavior, Qt::CaseSensitivity cs) const
{
    QStringList list;
    int start = 0;
    int extra = 0;
    int end;
    while ((end = indexOf(sep, start + extra, cs)) != -1) {
        if (start != end || behavior == KeepEmptyParts)
            list.append(mid(start, end - start));
        start = end + sep.size();
        extra = (sep.size() == 0 ? 1 : 0);
    }
    if (start != size() || behavior == KeepEmptyParts)
        list.append(mid(start));
    return list;
}

/*!
    \overload
*/
QStringList QString::split(const QChar &sep, SplitBehavior behavior, Qt::CaseSensitivity cs) const
{
    QStringList list;
    int start = 0;
    int end;
    while ((end = indexOf(sep, start, cs)) != -1) {
        if (start != end || behavior == KeepEmptyParts)
            list.append(mid(start, end - start));
        start = end + 1;
    }
    if (start != size() || behavior == KeepEmptyParts)
        list.append(mid(start));
    return list;
}

#ifndef QT_NO_REGEXP
/*!
    \overload

    Splits the string into substrings wherever the regular expression
    \a rx matches, and returns the list of those strings. If \a rx
    does not match anywhere in the string, split() returns a
    single-element list containing this string.

    Here's an example where we extract the words in a sentence
    using one or more whitespace characters as the separator:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::splitFunction()
    \skipto QString str
    \printuntil // list: [ "Some", "text", "with", "strange", "whitespace." ]

    Here's a similar example, but this time we use any sequence of
    non-word characters as the separator:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::splitFunction()
    \skipto str = "This time
    \printuntil // list: [ "This", "time", "a", "normal", "English", "sentence" ]

    Here's a third example where we use a zero-length assertion,
    \bold{\\b} (word boundary), to split the string into an
    alternating sequence of non-word and word tokens:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::splitFunction()
    \skipto str = "Now
    \printuntil // list: [ "", "Now", ": ", "this", " ", "sentence", " ", "fragment", "." ]

    \sa QStringList::join(), section()
*/
QStringList QString::split(const QRegExp &rx, SplitBehavior behavior) const
{
    QStringList list;
    int start = 0;
    int extra = 0;
    int end;
    while ((end = indexOf(rx, start + extra)) != -1) {
        int matchedLen = rx.matchedLength();
        if (start != end || behavior == KeepEmptyParts)
            list.append(mid(start, end - start));
        start = end + matchedLen;
        extra = (matchedLen == 0) ? 1 : 0;
    }
    if (start != size() || behavior == KeepEmptyParts)
        list.append(mid(start));
    return list;
}
#endif

/*!
    \enum QString::NormalizationForm

    This enum describes the various normalized forms of Unicode text.

    \value NormalizationForm_D  Canonical Decomposition
    \value NormalizationForm_C  Canonical Decomposition followed by Canonical Composition
    \value NormalizationForm_KD  Compatibility Decomposition
    \value NormalizationForm_KC  Compatibility Decomposition followed by Canonical Composition

    \sa normalized(),
        {http://www.unicode.org/reports/tr15/}{Unicode Standard Annex #15}
*/

/*!
    \fn QString QString::normalized(NormalizationForm mode) const
    Returns the string in the given Unicode normalization \a mode.
*/
QString QString::normalized(QString::NormalizationForm mode) const
{
    return normalized(mode, CURRENT_VERSION);
}

/*!
    \overload
    \fn QString QString::normalized(NormalizationForm mode, QChar::UnicodeVersion version) const

    Returns the string in the given Unicode normalization \a mode,
    according to the given \a version of the Unicode standard.
*/
QString QString::normalized(QString::NormalizationForm mode, QChar::UnicodeVersion version) const
{
    QString s = *this;
    if (version != CURRENT_VERSION) {
        for (int i = 0; i < NumNormalizationCorrections; ++i) {
            const NormalizationCorrection n = uc_normalization_corrections[i];
            if (n.version > version) {
                QString orig;
                orig.append(QChar::highSurrogate(n.ucs4));
                orig.append(QChar::lowSurrogate(n.ucs4));
                QString replacement;
                replacement.append(QChar::highSurrogate(n.old_mapping));
                replacement.append(QChar::lowSurrogate(n.old_mapping));
                s.replace(orig, replacement);
            }
        }
    }
    s = decomposeHelper(s, mode < QString::NormalizationForm_KD, version);

    s = canonicalOrderHelper(s, version);

    if (mode == QString::NormalizationForm_D || mode == QString::NormalizationForm_KD)
        return s;

    return composeHelper(s);

}


struct ArgEscapeData
{
    int min_escape;            // lowest escape sequence number
    int occurrences;           // number of occurrences of the lowest escape sequence number
    int locale_occurrences;    // number of occurrences of the lowest escape sequence number that
                               // contain 'L'
    int escape_len;            // total length of escape sequences which will be replaced
};

static ArgEscapeData findArgEscapes(const QString &s)
{
    const QChar *uc_begin = s.unicode();
    const QChar *uc_end = uc_begin + s.length();

    ArgEscapeData d;

    d.min_escape = INT_MAX;
    d.occurrences = 0;
    d.escape_len = 0;
    d.locale_occurrences = 0;

    const QChar *c = uc_begin;
    while (c != uc_end) {
        while (c != uc_end && c->unicode() != '%')
            ++c;

        if (c == uc_end)
            break;
        const QChar *escape_start = c;
        if (++c == uc_end)
            break;

        bool locale_arg = false;
        if (c->unicode() == 'L') {
            locale_arg = true;
            if (++c == uc_end)
                break;
        }

        if (c->digitValue() == -1)
            continue;

        int escape = c->digitValue();
        ++c;

        if (c != uc_end && c->digitValue() != -1) {
            escape = (10 * escape) + c->digitValue();
            ++c;
        }

        if (escape > d.min_escape)
            continue;

        if (escape < d.min_escape) {
            d.min_escape = escape;
            d.occurrences = 0;
            d.escape_len = 0;
            d.locale_occurrences = 0;
        }

        ++d.occurrences;
        if (locale_arg)
            ++d.locale_occurrences;
        d.escape_len += c - escape_start;
    }
    return d;
}

static QString replaceArgEscapes(const QString &s, const ArgEscapeData &d, int field_width,
                                 const QString &arg, const QString &larg, const QChar &fillChar = QLatin1Char(' '))
{
    const QChar *uc_begin = s.unicode();
    const QChar *uc_end = uc_begin + s.length();

    int abs_field_width = qAbs(field_width);
    int result_len = s.length()
                     - d.escape_len
                     + (d.occurrences - d.locale_occurrences)
                     *qMax(abs_field_width, arg.length())
                     + d.locale_occurrences
                     *qMax(abs_field_width, larg.length());

    QString result;
    result.resize(result_len);
    QChar *result_buff = (QChar*) result.unicode();

    QChar *rc = result_buff;
    const QChar *c = uc_begin;
    int repl_cnt = 0;
    while (c != uc_end) {
        /* We don't have to check if we run off the end of the string with c,
           because as long as d.occurrences > 0 we KNOW there are valid escape
           sequences. */

        const QChar *text_start = c;

        while (c->unicode() != '%')
            ++c;

        const QChar *escape_start = c++;

        bool locale_arg = false;
        if (c->unicode() == 'L') {
            locale_arg = true;
            ++c;
        }

        int escape = c->digitValue();
        if (escape != -1) {
            if (c + 1 != uc_end && (c + 1)->digitValue() != -1) {
                escape = (10 * escape) + (c + 1)->digitValue();
                ++c;
            }
        }

        if (escape != d.min_escape) {
            memcpy(rc, text_start, (c - text_start)*sizeof(QChar));
            rc += c - text_start;
        }
        else {
            ++c;

            memcpy(rc, text_start, (escape_start - text_start)*sizeof(QChar));
            rc += escape_start - text_start;

            uint pad_chars;
            if (locale_arg)
                pad_chars = qMax(abs_field_width, larg.length()) - larg.length();
            else
                pad_chars = qMax(abs_field_width, arg.length()) - arg.length();

            if (field_width > 0) { // left padded
                for (uint i = 0; i < pad_chars; ++i)
                    (rc++)->unicode() = fillChar.unicode();
            }

            if (locale_arg) {
                memcpy(rc, larg.unicode(), larg.length()*sizeof(QChar));
                rc += larg.length();
            }
            else {
                memcpy(rc, arg.unicode(), arg.length()*sizeof(QChar));
                rc += arg.length();
            }

            if (field_width < 0) { // right padded
                for (uint i = 0; i < pad_chars; ++i)
                    (rc++)->unicode() = fillChar.unicode();
            }

            if (++repl_cnt == d.occurrences) {
                memcpy(rc, c, (uc_end - c)*sizeof(QChar));
                rc += uc_end - c;
                Q_ASSERT(rc - result_buff == result_len);
                c = uc_end;
            }
        }
    }
    Q_ASSERT(rc == result_buff + result_len);

    return result;
}

/*!
    This function returns a copy of this string where \a a
    replaces the lowest numbered occurrence of \c %1, \c %2, ..., \c
    %99.

    The \a fieldWidth value specifies the minimum amount of space that
    \a a is padded to and filled with the character \a
    fillChar. A positive value will produce right-aligned text,
    whereas a negative value will produce left-aligned text.

    The following example shows how we could create a 'status' string
    when processing a list of files:

    \quotefromfile snippets/qstring/main.cpp
    \skipto argFunction
    \skipto QString i
    \printuntil .arg(i).arg(total).arg(fileName)

    One advantage of using arg() over sprintf() is that the order of arguments
    may need to change in other languages, when the application is translated.

    If there is no place marker (\c %1, \c %2, etc.), a warning
    message is output and the result is undefined.  Note that only
    placeholders between \c %1 and \c %99 are supported.
*/
QString QString::arg(const QString &a, int fieldWidth, const QChar &fillChar) const
{
    ArgEscapeData d = findArgEscapes(*this);

    if (d.occurrences == 0) {
        qWarning("QString::arg: Argument missing: %s, %s", toLocal8Bit().data(),
                  a.toLocal8Bit().data());
        return *this;
    }
    return replaceArgEscapes(*this, d, fieldWidth, a, a, fillChar);
}

/*!
    \fn QString QString::arg(const QString& a1, const QString& a2) const
    \overload

    This is the same as \c {str.arg(a1).arg(a2)}, except that the
    strings \a a1 and \a a2 are replaced in one pass. This can make a
    difference if \a a1 contains e.g. \c{%1}:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::argFunction()
    \skipto QString str
    \printuntil .arg("Hello")
*/

/*!
    \fn QString QString::arg(const QString& a1, const QString& a2, const QString& a3) const
    \overload

    This is the same as calling \c str.arg(a1).arg(a2).arg(a3), except
    that the strings \a a1, \a a2 and \a a3 are replaced in one pass.
*/

/*!
    \fn QString QString::arg(const QString& a1, const QString& a2, const QString& a3, const QString& a4) const
    \overload

    This is the same as calling \c
    {str.arg(a1).arg(a2).arg(a3).arg(a4)}, except that the strings \a
    a1, \a a2, \a a3 and \a a4 are replaced in one pass.
*/

/*!
    \fn QString QString::arg(const QString& a1, const QString& a2, const QString& a3, const QString& a4, const QString& a5) const
    \overload

    This is the same as calling \c
    {str.arg(a1).arg(a2).arg(a3).arg(a4).arg(a5)}, except that the strings
    \a a1, \a a2, \a a3, \a a4, and \a a5 are replaced in one pass.
*/

/*!
    \fn QString QString::arg(const QString& a1, const QString& a2, const QString& a3, const QString& a4, const QString& a5, const QString& a6) const
    \overload

    This is the same as calling \c
    {str.arg(a1).arg(a2).arg(a3).arg(a4).arg(a5).arg(a6))}, except that the strings
    \a a1, \a a2, \a a3, \a a4, \a a5, and \a a6 are replaced in one pass.
*/

/*!
    \fn QString QString::arg(const QString& a1, const QString& a2, const QString& a3, const QString& a4, const QString& a5, const QString& a6, const QString& a7) const
    \overload

    This is the same as calling \c
    {str.arg(a1).arg(a2).arg(a3).arg(a4).arg(a5).arg(a6).arg(a7)}, except that the strings
    \a a1, \a a2, \a a3, \a a4, \a a5, \a a6, and \a a7 are replaced in one pass.
*/

/*!
    \fn QString QString::arg(const QString& a1, const QString& a2, const QString& a3, const QString& a4, const QString& a5, const QString& a6, const QString& a7, const QString& a8) const
    \overload

    This is the same as calling \c
    {str.arg(a1).arg(a2).arg(a3).arg(a4).arg(a5).arg(a6).arg(a7).arg(a8)}, except that the strings
    \a a1, \a a2, \a a3, \a a4, \a a5, \a a6, \a a7, and \a a8 are replaced in one pass.
*/

/*!
    \fn QString QString::arg(const QString& a1, const QString& a2, const QString& a3, const QString& a4, const QString& a5, const QString& a6, const QString& a7, const QString& a8, const QString& a9) const
    \overload

    This is the same as calling \c
    {str.arg(a1).arg(a2).arg(a3).arg(a4).arg(a5).arg(a6).arg(a7).arg(a8).arg(a9)}, except that the strings
    \a a1, \a a2, \a a3, \a a4, \a a5, \a a6, \a a7, \a a8, and \a a9 are replaced in one pass.
*/

/*! \fn QString QString::arg(int a, int fieldWidth, int base, const QChar &fillChar) const

    \overload

    The \a a argument is expressed in base \a base, which is 10 by
    default and must be between 2 and 36. For bases other than 10,
    \a a is treated as an unsigned integer.

    The \a fieldWidth value specifies the minimum amount of space that
    \a a is padded to and filled with the character \a fillChar. A
    positive value will produce a right-aligned number, whereas a
    negative value will produce a left-aligned number.

    The '%' can be followed by an 'L', in which case the sequence is
    replaced with a localized representation of \a a. The conversion
    uses the default locale, set by QLocale::setDefault(). If no
    default locale was specified, the "C" locale is used. The 'L' flag
    is ignored if \a base is not 10.

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::argFunction()
    \skipto QString str
    \printline QString str
    \skipto Decimal
    \printuntil   // str == "12345 12,345 3039"

*/

/*! \fn QString QString::arg(uint a, int fieldWidth, int base, const QChar &fillChar) const

    \overload

    The \a base argument specifies the base to use when converting the
    integer \a a into a string. The base must be between 2 and 36.
*/

/*! \fn QString QString::arg(long a, int fieldWidth, int base, const QChar &fillChar) const

    \overload

    The \a fieldWidth value specifies the minimum amount of space that
    \a a is padded to and filled with the character \a fillChar. A
    positive value will produce a right-aligned number, whereas a
    negative value will produce a left-aligned number.

    The \a a argument is expressed in the given \a base, which is 10
    by default and must be between 2 and 36.

    The '%' can be followed by an 'L', in which case the sequence is
    replaced with a localized representation of \a a. The conversion
    uses the default locale. The default locale is determined from the
    system's locale settings at application startup. It can be changed
    using QLocale::setDefault(). The 'L' flag is ignored if \a base is
    not 10.

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::argFunction()
    \skipto QString str
    \printline QString str
    \skipto Decimal
    \printuntil   // str == "12345 12,345 3039"

*/

/*! \fn QString QString::arg(ulong a, int fieldWidth, int base, const QChar &fillChar) const

    \overload

    The \a base argument specifies the base to use when converting the
    integer \a a into a string. The base must be between 2 and 36,
    with 8 giving octal, 10 decimal, and 16 hexadecimal numbers.
*/

/*!
    \overload

    The \a base argument specifies the base to use when converting the
    integer \a a into a string. The base must be between 2 and 36,
    with 8 giving octal, 10 decimal, and 16 hexadecimal numbers.
*/
QString QString::arg(qlonglong a, int fieldWidth, int base, const QChar &fillChar) const
{
    ArgEscapeData d = findArgEscapes(*this);

    if (d.occurrences == 0) {
        qWarning("QString::arg: Argument missing: %s, %lld", toLocal8Bit().data(), a);
        return *this;
    }

    QString arg;
    if (d.occurrences > d.locale_occurrences)
        arg = number(a, base);

    QString locale_arg;
    if (d.locale_occurrences > 0) {
        QLocale locale;
        locale_arg = locale.d()->longLongToString(a, -1, base, -1, QLocalePrivate::ThousandsGroup);
    }

    return replaceArgEscapes(*this, d, fieldWidth, arg, locale_arg, fillChar);
}

/*!
    \overload

    The \a base argument specifies the base to use when converting the
    integer \a a into a string. \a base must be between 2 and 36, with
    8 giving octal, 10 decimal, and 16 hexadecimal numbers.
*/
QString QString::arg(qulonglong a, int fieldWidth, int base, const QChar &fillChar) const
{
    ArgEscapeData d = findArgEscapes(*this);

    if (d.occurrences == 0) {
        qWarning("QString::arg: Argument missing: %s, %llu", toLocal8Bit().data(), a);
        return *this;
    }

    QString arg;
    if (d.occurrences > d.locale_occurrences)
        arg = number(a, base);

    QString locale_arg;
    if (d.locale_occurrences > 0) {
        QLocale locale;
        locale_arg = locale.d()->unsLongLongToString(a, -1, base, -1, QLocalePrivate::ThousandsGroup);
    }

    return replaceArgEscapes(*this, d, fieldWidth, arg, locale_arg, fillChar);
}

/*!
    \fn QString QString::arg(short a, int fieldWidth, int base, const QChar &fillChar) const

    \overload

    The \a base argument specifies the base to use when converting the
    integer \a a into a string. The base must be between 2 and 36,
    with 8 giving octal, 10 decimal, and 16 hexadecimal numbers.
*/

/*!
    \fn QString QString::arg(ushort a, int fieldWidth, int base, const QChar &fillChar) const

    \overload

    The \a base argument specifies the base to use when converting the
    integer \a a into a string. The base must be between 2 and 36,
    with 8 giving octal, 10 decimal, and 16 hexadecimal numbers.
*/

/*!
    \overload
*/
QString QString::arg(QChar a, int fieldWidth, const QChar &fillChar) const
{
    QString c;
    c += a;
    return arg(c, fieldWidth, fillChar);
}

/*!
    \overload

    The \a a argument is interpreted as a Latin-1 character.
*/
QString QString::arg(char a, int fieldWidth, const QChar &fillChar) const
{
    QString c;
    c += QLatin1Char(a);
    return arg(c, fieldWidth, fillChar);
}

/*!
    \fn QString QString::arg(double a, int fieldWidth, char format, int precision, const QChar &fillChar) const
    \overload

    \target arg-formats

    Argument \a a is formatted according to the specified \a format,
    which is 'g' by default and can be any of the following:

    \table
    \header \o Format \o Meaning
    \row \o \c e \o format as [-]9.9e[+|-]999
    \row \o \c E \o format as [-]9.9E[+|-]999
    \row \o \c f \o format as [-]9.9
    \row \o \c g \o use \c e or \c f format, whichever is the most concise
    \row \o \c G \o use \c E or \c f format, whichever is the most concise
    \endtable

    With 'e', 'E', and 'f', \a precision is the number of digits after
    the decimal point. With 'g' and 'G', \a precision is the maximum
    number of significant digits (trailing zeroes are omitted).

    \code
        double d = 12.34;
        QString str = QString("delta: %1").arg(d, 0, 'E', 3);
        // str == "delta: 1.234E+01"
    \endcode

    The '%' can be followed by an 'L', in which case the sequence is
    replaced with a localized representation of \a a. The conversion uses
    the default locale, set by QLocale::setDefaultLocale(). If no default
    locale was specified, the "C" locale is used.

    \sa QLocale::toString()
*/
QString QString::arg(double a, int fieldWidth, char fmt, int prec, const QChar &fillChar) const
{
    ArgEscapeData d = findArgEscapes(*this);

    if (d.occurrences == 0) {
        qWarning("QString::arg: Argument missing: %s, %g", toLocal8Bit().data(), a);
        return *this;
    }

    QString arg;
    if (d.occurrences > d.locale_occurrences)
        arg = number(a, fmt, prec);

    QString locale_arg;
    if (d.locale_occurrences > 0) {
        QLocale locale;

        QLocalePrivate::DoubleForm form = QLocalePrivate::DFDecimal;
        uint flags = 0;

        if (qIsUpper(fmt))
            flags = QLocalePrivate::CapitalEorX;
        fmt = qToLower(fmt);

        switch (fmt) {
            case 'f':
                form = QLocalePrivate::DFDecimal;
                break;
            case 'e':
                form = QLocalePrivate::DFExponent;
                break;
            case 'g':
                form = QLocalePrivate::DFSignificantDigits;
                break;
            default:
#if defined(QT_CHECK_RANGE)
                qWarning("QString::arg: Invalid format char '%c'", fmt);
#endif
                break;
        }

        flags |= QLocalePrivate::ThousandsGroup;

        locale_arg = locale.d()->doubleToString(a, prec, form, -1, flags);
    }

    return replaceArgEscapes(*this, d, fieldWidth, arg, locale_arg, fillChar);
}


QString QString::multiArg(int numArgs, const QString **args) const
{
    QString result;
    union {
        int digitUsed[10];
        int argForDigit[10];
    };
    const QChar *uc = (const QChar*) d->data;
    const int len = d->size;
    const int end = len - 1;
    int lastDigit = -1;
    int i;

    memset(digitUsed, 0, sizeof(digitUsed));

    for (i = 0; i < end; i++) {
        if (uc[i] == QLatin1Char('%')) {
            int digit = uc[i + 1].unicode() - '0';
            if (digit >= 0 && digit <= 9)
                digitUsed[digit]++;
        }
    }

    for (i = 0; i < numArgs; i++) {
        do {
            ++lastDigit;
        } while (lastDigit < 10 && digitUsed[lastDigit] == 0);

        if (lastDigit == 10) {
            qWarning("QString::arg: Argument missing: %s, %s", toLocal8Bit().data(), args[i]->toLocal8Bit().data());
            numArgs = i;
            lastDigit = 9;
            break;
        }
        argForDigit[lastDigit] = i;
    }

    i = 0;
    while (i < len) {
        if (uc[i] == QLatin1Char('%') && i != end) {
            int digit = uc[i + 1].unicode() - '0';
            if (digit >= 0 && digit <= lastDigit) {
                result += *args[argForDigit[digit]];
                i += 2;
                continue;
            }
        }
        result += uc[i++];
    }
    return result;
}



/*! \internal
 */
void QString::updateProperties() const
{
    ushort *p = d->data;
    ushort *end = p + d->size;
    d->simpletext = true;
    while (p < end) {
        ushort uc = *p;
        // sort out regions of complex text formatting
        if (uc > 0x058f && (uc < 0x1100 || uc > 0xfb0f)) {
            d->simpletext = false;
        }
        p++;
    }

    p = d->data;
    d->righttoleft = false;
    while (p < end) {
        switch(QChar::direction(*p))
        {
        case QChar::DirL:
        case QChar::DirLRO:
        case QChar::DirLRE:
            goto end;
        case QChar::DirR:
        case QChar::DirAL:
        case QChar::DirRLO:
        case QChar::DirRLE:
            d->righttoleft = true;
            goto end;
        default:
            break;
        }
        ++p;
    }
 end:
    d->clean = true;
    return;
}

/*! \fn bool QString::isSimpleText() const

    \internal
*/

/*! \fn bool QString::isRightToLeft() const

    \internal
*/


/*! \fn QString &QString::inline_append(QChar ch)
    \internal

    An inlined version of append().
*/

/*! \fn QChar *QString::data()

    Returns a pointer to the data stored in the QString. The pointer
    can be used to access and modify the characters that compose the
    string. For convenience, the data is '\\0'-terminated.

    Example:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::dataFunction()
    \skipto QString str
    \printuntil }

    Note that the pointer remains valid only as long as the string is
    not modified by other means. For read-only access, constData() is
    faster because it never causes a \l{deep copy} to occur.

    \sa constData(), operator[]()
*/

/*! \fn const QChar *QString::data() const

    \overload
*/

/*! \fn const QChar *QString::constData() const

    Returns a pointer to the data stored in the QString. The pointer
    can be used to access the characters that compose the string. For
    convenience, the data is '\\0'-terminated.

    Note that the pointer remains valid only as long as the string is
    not modified.

    \sa data(), operator[]()
*/

/*! \fn void QString::push_front(const QString &other)

    This function is provided for STL compatibility, prepending the
    given \a other string to the beginning of this string. It is
    equivalent to \c prepend(other).

    \sa prepend()
*/

/*! \fn void QString::push_front(QChar ch)

    \overload

    Prepends the given \a ch character to the beginning of this string.
*/

/*! \fn void QString::push_back(const QString &other)

    This function is provided for STL compatibility, appending the
    given \a other string onto the end of this string. It is
    equivalent to \c append(other).

    \sa append()
*/

/*! \fn void QString::push_back(QChar ch)

    \overload

    Appends the given \a ch character onto the end of this string.
*/

/*! \fn std::string QString::toStdString() const

    Returns a std::string object with the data contained in this
    QString. The Unicode data is converted into 8-bit characters using
    the toAscii() function.

    This operator is mostly useful to pass a QString to a function
    that accepts a std::string object.

    If the QString contains non-ASCII Unicode characters, using this
    operator can lead to loss of information. You can disable this
    operator by defining \c QT_NO_CAST_TO_ASCII when you compile your
    applications. You then need to call toAscii() (or toLatin1() or
    toUtf8() or toLocal8Bit()) explicitly if you want to convert the data
    to \c{const char *} and pass the return value on to the
    std::string constructor.

    This operator is only available if Qt is configured with STL
    compatibility enabled.

    \sa toAscii(), toLatin1(), toUtf8(), toLocal8Bit()
*/

/*!
    Constructs a QString that uses the first \a size Unicode characters
    in the array \a unicode. The data in \a unicode is \e not
    copied. The caller must be able to guarantee that \a unicode will
    not be deleted or modified as long as the QString (or an
    unmodified copy of it) exists.

    Any attempts to modify the QString or copies of it will cause it
    to create a deep copy of the data, ensuring that the raw data
    isn't modified.

    Here's an example of how we can use a QRegExp on raw data in
    memory without requiring to copy the data into a QString:

    \quotefromfile snippets/qstring/main.cpp
    \skipto Widget::fromRawDataFunction()
    \skipto QRegExp
    \printuntil // ...
    \printline }

    \warning A string created with fromRawData() is \e not
    '\\0'-terminated, unless the raw data contains a '\\0' character
    at position \a size. This means unicode() will \e not return a
    '\\0'-terminated string (although utf16() does, at the cost of
    copying the raw data).

    \sa fromUtf16()
*/
QString QString::fromRawData(const QChar *unicode, int size)
{
    Data *x = static_cast<Data *>(qMalloc(sizeof(Data)));
    if (unicode) {
        x->data = (ushort *)unicode;
    } else {
        x->data = x->array;
        size = 0;
    }
    x->ref = 1;
    x->alloc = x->size = size;
    *x->array = '\0';
    x->clean = x->asciiCache = x->simpletext = x->righttoleft = x->capacity = 0;
    return QString(x, 0);
}

/*! \class QLatin1String
    \brief The QLatin1String class provides a thin wrapper around an ASCII/Latin-1 encoded string literal.

    \ingroup text
    \reentrant

    Many of QString's member functions are overloaded to accept
    \c{const char *} instead of QString. This includes the copy
    constructor, the assignment operator, the comparison operators,
    and various other functions such as \link QString::insert()
    insert() \endlink, \link QString::replace() replace()\endlink,
    and \link QString::indexOf() indexOf()\endlink. These functions
    are usually optimized to avoid constructing a QString object for
    the \c{const char *} data. For example, assuming \c str is a
    QString,

    \code
        if (str == "auto" || str == "extern"
                || str == "static" || str == "register") {
            ...
        }
    \endcode

    is much faster than

    \code
        if (str == QString("auto") || str == QString("extern")
                || str == QString("static") || str == QString("register")) {
            ...
        }
    \endcode

    because it doesn't construct four temporary QString objects and
    make a deep copy of the character data.

    Applications that define \c QT_NO_CAST_FROM_ASCII (as explained
    in the QString documentation) don't have access to QString's
    \c{const char *} API. To provide an efficient way of specifying
    constant Latin-1 strings, Qt provides the QLatin1String, which is
    just a very thin wrapper around a \c{const char *}. Using
    QLatin1String, the example code above becomes

    \code
        if (str == QLatin1String("auto")
                || str == QLatin1String("extern")
                || str == QLatin1String("static")
                || str == QLatin1String("register") {
            ...
        }
    \endcode

    This is a bit longer to type, but it provides exactly the same
    benefits as the first version of the code, and is faster than
    converting the Latin-1 strings using QString::fromLatin1().

    Thanks to the QString(const QLatin1String &) constructor,
    QLatin1String can be used everywhere a QString is expected. For example:

    \code
        QLabel *label = new QLabel(QLatin1String("MOD"), this);
    \endcode

    \sa QString, QLatin1Char
*/

/*! \fn QLatin1String::QLatin1String(const char *str)

    Constructs a QLatin1String object that stores \a str.

    The string data is \e not copied. The caller must be able to
    guarantee that \a str will not be deleted or modified as long as
    the QLatin1String object exists.

    \sa latin1()
*/

/*!
    \since 4.1
    \fn QLatin1String &QLatin1String::operator=(const QLatin1String &other)

    Constructs a copy of \a other.
*/

/*! \fn const char *QLatin1String::latin1() const

    Returns the Latin-1 string stored in this object.
*/

/*! \fn bool QLatin1String::operator==(const QString &other) const

    Returns true if this string is equal to string \a other;
    otherwise returns false.

    The comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would
    expect. Consider sorting user-interface strings with
    QString::localeAwareCompare().
*/

/*!
    \fn bool QLatin1String::operator==(const char *other) const
    \since 4.3
    \overload

    The \a other const char pointer is converted to a QLatin1String using
    the QString::fromAscii() function.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn bool QLatin1String::operator!=(const QString &other) const

    Returns true if this string is not equal to string \a other;
    otherwise returns false.

    The comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would
    expect. Consider sorting user-interface strings with
    QString::localeAwareCompare().
*/

/*!
    \fn bool QLatin1String::operator!=(const char *other) const
    \since 4.3
    \overload

    The \a other const char pointer is converted to a QLatin1String using
    the QString::fromAscii() function.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*!
    \fn bool QLatin1String::operator>(const QString &other) const

    Returns true if this string is lexically greater than string \a
    other; otherwise returns false.

    The comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would
    expect. Consider sorting user-interface strings with
    QString::localeAwareCompare().
*/

/*!
    \fn bool QLatin1String::operator>(const char *other) const
    \since 4.3
    \overload

    The \a other const char pointer is converted to a QLatin1String using
    the QString::fromAscii() function.

    You can disable this operator by defining \c QT_NO_CAST_FROM_ASCII
    when you compile your applications. This can be useful if you want
    to ensure that all user-visible strings go through QObject::tr(),
    for example.
*/

/*!
    \fn bool QLatin1String::operator<(const QString &other) const

    Returns true if this string is lexically less than the \a other
    string; otherwise returns false.

    The comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would
    expect. Consider sorting user-interface strings using the
    QString::localeAwareCompare() function.
*/

/*!
    \fn bool QLatin1String::operator<(const char *other) const
    \since 4.3
    \overload

    The \a other const char pointer is converted to a QLatin1String using
    the QString::fromAscii() function.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*!
    \fn bool QLatin1String::operator>=(const QString &other) const

    Returns true if this string is lexically greater than or equal
    to string \a other; otherwise returns false.

    The comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would
    expect. Consider sorting user-interface strings with
    QString::localeAwareCompare().
*/

/*!
    \fn bool QLatin1String::operator>=(const char *other) const
    \since 4.3
    \overload

    The \a other const char pointer is converted to a QLatin1String using
    the QString::fromAscii() function.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/

/*! \fn bool QLatin1String::operator<=(const QString &other) const

    Returns true if this string is lexically less than or equal
    to string \a other; otherwise returns false.

    The comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would
    expect. Consider sorting user-interface strings with
    QString::localeAwareCompare().
*/

/*!
    \fn bool QLatin1String::operator<=(const char *other) const
    \since 4.3
    \overload

    The \a other const char pointer is converted to a QString using
    the QString::fromAscii() function.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.
*/



/* \fn bool operator==(const QLatin1String &s1, const QLatin1String &s2)
   \relates QLatin1String

   Returns true if string \a s1 is lexically equal to string \a s2; otherwise
   returns false.
*/
/* \fn bool operator!=(const QLatin1String &s1, const QLatin1String &s2)
   \relates QLatin1String

   Returns true if string \a s1 is lexically unequal to string \a s2; otherwise
   returns false.
*/
/* \fn bool operator<(const QLatin1String &s1, const QLatin1String &s2)
   \relates QLatin1String

   Returns true if string \a s1 is lexically smaller than string \a s2; otherwise
   returns false.
*/
/* \fn bool operator<=(const QLatin1String &s1, const QLatin1String &s2)
   \relates QLatin1String

   Returns true if string \a s1 is lexically smaller than or equal to string \a s2; otherwise
   returns false.
*/
/* \fn bool operator>(const QLatin1String &s1, const QLatin1String &s2)
   \relates QLatin1String

   Returns true if string \a s1 is lexically greater than string \a s2; otherwise
   returns false.
*/
/* \fn bool operator>=(const QLatin1String &s1, const QLatin1String &s2)
   \relates QLatin1String

   Returns true if string \a s1 is lexically greater than or equal to
   string \a s2; otherwise returns false.
*/


#ifndef QT_NO_DATASTREAM
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QString &string)
    \relates QString

    Writes the given \a string to the specified \a stream.

    \sa {Format of the QDataStream Operators}
*/

QDataStream &operator<<(QDataStream &out, const QString &str)
{
    if (out.version() == 1) {
        out << str.toLatin1();
    } else {
        if (!str.isNull() || out.version() < 3) {
            int byteOrder = out.byteOrder();
            const QChar* ub = str.unicode();
            static const uint auto_size = 1024;
            char t[auto_size];
            char *b;
            if (str.length()*sizeof(QChar) > auto_size) {
                b = new char[str.length()*sizeof(QChar)];
            } else {
                b = t;
            }
            int l = str.length();
            char *c=b;
            while (l--) {
                if (byteOrder == QDataStream::BigEndian) {
                    *c++ = (char)ub->row();
                    *c++ = (char)ub->cell();
                } else {
                    *c++ = (char)ub->cell();
                    *c++ = (char)ub->row();
                }
                ub++;
            }
            out.writeBytes(b, sizeof(QChar)*str.length());
            if (str.length()*sizeof(QChar) > auto_size)
                delete [] b;
        } else {
            // write null marker
            out << (quint32)0xffffffff;
        }
    }
    return out;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QString &string)
    \relates QString

    Reads a string from the specified \a stream into the given \a string.

    \sa {Format of the QDataStream Operators}
*/

QDataStream &operator>>(QDataStream &in, QString &str)
{
#ifdef QT_QSTRING_UCS_4
#if defined(Q_CC_GNU)
#warning "operator>> not working properly"
#endif
#endif

    if (in.version() == 1) {
        QByteArray l;
        in >> l;
        str = QString::fromLatin1(l);
    } else {
        quint32 bytes = 0;
        in >> bytes;                                  // read size of string
        if (bytes == 0xffffffff) {                    // null string
            str.clear();
        } else if (bytes > 0) {                       // not empty
            if (bytes & 0x1) {
                str.clear();
                in.setStatus(QDataStream::ReadCorruptData);
                return in;
            }

            const quint32 Step = 1024 * 1024;
            quint32 len = bytes / 2;
            quint32 allocated = 0;

            while (allocated < len) {
                int blockSize = qMin(Step, len - allocated);
                str.resize(allocated + blockSize);
                if (in.readRawData(reinterpret_cast<char *>(str.data()) + allocated * 2,
                                   blockSize * 2) != blockSize * 2) {
                    str.clear();
                    in.setStatus(QDataStream::ReadPastEnd);
                    return in;
                }
                allocated += blockSize;
            }

            if ((in.byteOrder() == QDataStream::BigEndian)
                    != (QSysInfo::ByteOrder == QSysInfo::BigEndian)) {
                ushort *data = reinterpret_cast<ushort *>(str.data());
                while (len--) {
                    *data = (*data >> 8) | (*data << 8);
                    ++data;
                }
            }
        } else {
            str = QLatin1String("");
        }
    }
    return in;
}
#endif // QT_NO_DATASTREAM

/*!
    \fn void QString::setLength(int nl)

    Use resize() instead.
*/

/*!
    \fn QString QString::copy() const

    Use simple assignment instead. QString is implicitly shared so if
    a copy is modified only the copy is changed.
*/

/*!
    \fn QString &QString::remove(QChar c, bool cs)

    Use the remove(QChar, Qt::CaseSensitive) overload instead.
*/

/*!
    \fn QString &QString::remove(const QString  &s, bool cs)

    Use the remove(QString, Qt::CaseSensitive) overload instead.
*/

/*!
    \fn QString &QString::replace(QChar c, const QString  &after, bool cs)

    Use the replace(QChar, QString, Qt::CaseSensitive) overload instead.
*/

/*!
    \fn QString &QString::replace(const QString &before, const QString &after, bool cs)

    Use the replace(QString, QString, Qt::CaseSensitive) overload instead.
*/

/*!
    \fn QString &QString::replace(char c, const QString &after, bool cs)

    Use the replace(QChar, QString, Qt::CaseSensitive) overload instead.
*/

/*!
    \fn QString &QString::replace(char c, const QString &after, Qt::CaseSensitivity cs)

    Use the replace(QChar, QString, Qt::CaseSensitive) overload instead.
*/

/*!
    \fn int QString::find(QChar c, int i = 0, bool cs = true) const

    Use indexOf() instead.
*/

/*!
    \fn int QString::find(const QString &s, int i = 0, bool cs = true) const

    Use indexOf() instead.
*/

/*!
    \fn int QString::findRev(QChar c, int i = -1, bool cs = true) const

    Use lastIndexOf() instead.
*/

/*!
    \fn int QString::findRev(const QString &s, int i = -1, bool cs = true) const

    Use lastIndexOf() instead.
*/

/*!
    \fn int QString::find(const QRegExp &rx, int i=0) const

    Use indexOf() instead.
*/

/*!
    \fn int QString::findRev(const QRegExp &rx, int i=-1) const

    Use lastIndexOf() instead.
*/

/*!
    \fn QBool QString::contains(QChar c, bool cs) const

    Use the contains(QChar, Qt::CaseSensitive) overload instead.
*/

/*!
    \fn QBool QString::contains(const QString &s, bool cs) const

    Use the contains(QString, Qt::CaseSensitive) overload instead.
*/

/*!
    \fn bool QString::startsWith(const QString &s, bool cs) const

    Use the startsWith(QString, Qt::CaseSensitive) overload instead.
*/

/*!
    \fn bool QString::endsWith(const QString &s, bool cs) const

    Use the endsWith(QString, Qt::CaseSensitive) overload instead.
*/

/*!
    \fn QString QString::leftJustify(int width, QChar fill = QLatin1Char(' '), bool trunc=false) const

    Use leftJustified() instead.
*/

/*!
    \fn QString QString::rightJustify(int width, QChar fill = QLatin1Char(' '), bool trunc=false) const

    Use rightJustified() instead.
*/

/*!
    \fn QString QString::lower() const

    Use toLower() instead.
*/

/*!
    \fn QString QString::upper() const

    Use toUpper() instead.
*/

/*!
    \fn QString QString::stripWhiteSpace() const

    Use trimmed() instead.
*/

/*!
    \fn QString QString::simplifyWhiteSpace() const

    Use simplified() instead.
*/

/*!
    \fn QString &QString::setUnicodeCodes(const ushort *unicode_as_ushorts, int size)

    Use setUtf16() instead.
*/

/*!
    \fn ushort *QString::ucs2() const

    Use utf16() instead.
*/

/*!
    \fn QString QString::fromUcs2(const ushort *unicode, int size = -1)

    Use fromUtf16() instead.
*/

/*!
    \fn QString &QString::setAscii(const char *str, int len = -1)

    Use fromAscii() instead.
*/

/*!
    \fn QString &QString::setLatin1(const char *str, int len = -1)

    Use fromLatin1() instead.
*/

/*!
    \fn QChar QString::constref(uint i) const

    Use at() instead.
*/

/*!
    \fn QChar &QString::ref(uint i);

    Use operator[]() instead.
*/

/*!
    \fn QString::operator const char *() const

    Use toAscii().constData() instead.
*/

/*!
    \class QConstString
    \brief The QConstString class is a wrapper for constant Unicode string data.
    \compat

    In Qt 4, QConstString is replaced by QString::fromRawData(), a
    static function that constructs a QString object based on Unicode
    string data.

    Because QString::fromRawData() has slightly more stringent
    constranints than QConstString had in Qt 3, the new QConstString
    class takes a deep copy of the string data.

    \sa QString::fromRawData()
*/

/*!
    \fn QConstString::QConstString(const QChar *unicode, int size)

    Use QString(\a unicode, \a size) or
    QString::fromRawData(\a unicode, \a size) instead.
*/

/*!
    \fn const QString &QConstString::string() const

    Returns \c *this. Not necessary in Qt 4.
*/



/*!
    \class QStringRef
    \since 4.3
    \brief The QStringRef class provides a thin wrapper around QString substrings.
    \reentrant
    \ingroup tools
    \ingroup text

    QStringRef provides a read-only subset of the QString API.

    A string reference explicitly references a portion of a string()
    with a given size(), starting at a specific position(). Calling
    toString() returns a copy of the data as a real QString instance.

    This class is designed to improve the performance of substring
    handling when manipulating substrings obtained from existing QString
    instances. QStringRef avoids the memory allocation and reference
    counting overhead of a standard QString by simply referencing a
    part of the original string. This can prove to be advantageous in
    low level code, such as that used in a parser, at the expense of
    potentially more complex code.

    For most users, there are no semantic benefits to using QStringRef
    instead of QString since QStringRef requires attention to be paid
    to memory management issues, potentially making code more complex
    to write and maintain.

    \warning A QStringRef is only valid as long as the referenced
    string exists. If the original string is deleted, the string
    reference points to an invalid memory location.

    We suggest that you only use this class in stable code where profiling
    has clearly identified that performance improvements can be made by
    replacing standard string operations with the optimized substring
    handling provided by this class.

    \sa {Implicitly Shared Classes}
*/


/*!
 \fn QStringRef::QStringRef()

 Constructs an empty string reference.
*/

/*! \fn QStringRef::QStringRef(const QString *string, int position, int length)

Constructs a string reference to the range of characters in the given
\a string specified by the starting \a position and \a length in characters.

\warning This function exists to improve performance as much as possible,
and performs no bounds checking. For program correctness, \a position and
\a length must describe a valid substring of \a string.

This means that the starting \a position must be positive or 0 and smaller
than \a string's length, and \a length must be positive or 0 but smaller than
the string's length minus the starting \a position;
i.e, 0 <= position < string->length() and
0 <= length <= string->length() - position must both be satisfied.
*/

/*! \fn QStringRef::QStringRef(const QString *string)

Constructs a string reference to the given \a string.
*/

/*! \fn QStringRef::QStringRef(const QStringRef &other)

Constructs a copy of the \a other string reference.
 */
/*!
\fn QStringRef::~QStringRef()

Destroys the string reference.

Since this class is only used to refer to string data, and does not take
ownership of it, no memory is freed when instances are destroyed.
*/


/*!
    \fn int QStringRef::position() const

    Returns the starting position in the referenced string that is referred to
    by the string reference.

    \sa size(), string()
*/

/*!
    \fn int QStringRef::size() const

    Returns the number of characters referred to by the string reference.

    \sa position(), string()
*/
/*!
    \fn int QStringRef::count() const
    Returns the number of characters in this string.

    \sa position()
*/
/*!
    \fn int QStringRef::length() const
    Returns the number of characters in this substring.

    \sa position()
*/


/*!
    \fn bool QStringRef::isEmpty() const

    Returns true if the string reference has no characters; otherwise returns
    false.

    A string reference is empty if its size is zero.

    \sa size()
*/

/*!
    \fn bool QStringRef::isNull() const

    Returns true if string() returns a null pointer or a pointer to a
    null string; otherwise returns true.

    \sa size()
*/

/*!
    \fn const QString *QStringRef::string() const

    Returns a pointer to the string referred to by the string reference, or
    0 if it does not reference a string.

    \sa unicode()
*/


/*!
    \fn const QChar *QStringRef::unicode() const

    Returns a Unicode representation of the string reference. Since
    the data stems directly from the referenced string, it is not
    null-terminated unless the string reference includes the string's
    null terminator.

    \sa string()
*/

/*!
    \fn const QChar *QStringRef::data() const

    Same as unicode().
*/

/*!
    \fn const QChar *QStringRef::constData() const

    Same as unicode().
*/

/*!
    Returns a copy of the string reference as a QString object.

    If the string reference is not a complete reference of the string
    (meaning that position() is 0 and size() equals string()->size()),
    this function will allocate a new string to return.

    \sa string()
*/

QString QStringRef::toString() const {
    if (!m_string)
        return QString();
    if (m_size && m_position == 0 && m_size == m_string->size())
        return *m_string;
    return QString::fromUtf16(reinterpret_cast<const ushort*>(m_string->unicode() + m_position), m_size);
}


/*! \relates QStringRef

   Returns true if string reference \a s1 is lexically equal to string reference \a s2; otherwise
   returns false.
*/
bool operator==(const QStringRef &s1,const QStringRef &s2)
{ return (s1.size() == s2.size() &&
          (memcmp((char*)s1.unicode(), (char*)s2.unicode(), s1.size()*sizeof(QChar))==0)); }

/*! \relates QStringRef

   Returns true if string \a s1 is lexically equal to string reference \a s2; otherwise
   returns false.
*/
bool operator==(const QString &s1,const QStringRef &s2)
{ return (s1.size() == s2.size() &&
          (memcmp((char*)s1.unicode(), (char*)s2.unicode(), s1.size()*sizeof(QChar))==0)); }

/*! \relates QStringRef

   Returns true if string  \a s1 is lexically equal to string reference \a s2; otherwise
   returns false.
*/
bool operator==(const QLatin1String &s1, const QStringRef &s2)
{
    const ushort *uc = reinterpret_cast<const ushort *>(s2.unicode());
    const ushort *e = uc + s2.size();
    const uchar *c = reinterpret_cast<const uchar *>(s1.latin1());
    if (!c)
        return s2.isEmpty();

    while (*c) {
        if (uc == e || *uc != *c)
            return false;
        ++uc;
        ++c;
    }
    return (uc == e);
}

/*!
   \relates QStringRef

    Returns true if string reference \a s1 is lexically less than
    string reference \a s2; otherwise returns false.

    The comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would
    expect. Consider sorting user-interface strings using the
    QString::localeAwareCompare() function.
*/
bool operator<(const QStringRef &s1,const QStringRef &s2)
{
    return ucstrcmp(s1, s2) < 0;
}

/*!\fn bool operator<=(const QStringRef &s1,const QStringRef &s2)

   \relates QStringRef

    Returns true if string reference \a s1 is lexically less than
    or equal to string reference \a s2; otherwise returns false.

    The comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would
    expect. Consider sorting user-interface strings using the
    QString::localeAwareCompare() function.
*/

/*!\fn bool operator>=(const QStringRef &s1,const QStringRef &s2)

   \relates QStringRef

    Returns true if string reference \a s1 is lexically greater than
    or equal to string reference \a s2; otherwise returns false.

    The comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would
    expect. Consider sorting user-interface strings using the
    QString::localeAwareCompare() function.
*/

/*!\fn bool operator>(const QStringRef &s1,const QStringRef &s2)

   \relates QStringRef

    Returns true if string reference \a s1 is lexically greater than
    string reference \a s2; otherwise returns false.

    The comparison is based exclusively on the numeric Unicode values
    of the characters and is very fast, but is not what a human would
    expect. Consider sorting user-interface strings using the
    QString::localeAwareCompare() function.
*/


/*!
    \fn const QChar QStringRef::at(int position) const

    Returns the character at the given index \a position in the
    string reference.

    The \a position must be a valid index position in the string
    (i.e., 0 <= \a position < size()).
*/

/*!
    \fn void QStringRef::clear()

    Clears the contents of the string reference by making it null and empty.

    \sa isEmpty(), isNull()
*/

/*!
    \fn QStringRef &QStringRef::operator=(const QStringRef &other)

    Assigns the \a other string reference to this string reference, and
    returns the result.
*/

/*!
    \fn QStringRef &QStringRef::operator=(const QString *string)

    Constructs a string reference to the given \a string and assigns it to
    this string reference, returning the result.
*/

/*!
    \typedef QString::DataPtr
    \internal
*/

/*!
    \fn DataPtr & QString::data_ptr()
    \internal
*/



/*!  Appends the string reference to \a string, and returns a new
reference to the combined string data.
 */
QStringRef QStringRef::appendTo(QString *string) const
{
    if (!string)
        return QStringRef();
    int pos = string->size();
    string->insert(pos, unicode(), size());
    return QStringRef(string, pos, size());
}


QT_END_NAMESPACE
