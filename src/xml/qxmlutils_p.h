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

#ifndef QXMLUTILS_P_H
#define QXMLUTILS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp and qfiledialog.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

class QString;
class QChar;
class QXmlCharRange;

/*!
  \internal
  \short This class contains helper functions related to XML, for validating character classes,
         productions in the XML specification, and so on.

 This class is intentionally Not exported.
 */
class QXmlUtils
{
public:
    static bool isEncName(const QString &encName);
    static bool isChar(const QChar c);
    static bool isNameChar(const QChar c);
    static bool isLetter(const QChar c);
    static bool isPubidChar(const QChar c);
    static bool isNCName(const QString &ncName);

private:
    typedef const QXmlCharRange *RangeIter;
    static bool rangeContains(RangeIter begin, RangeIter end, const QChar c);
    static bool isBaseChar(const QChar c);
    static bool isDigit(const QChar c);
    static bool isExtender(const QChar c);
    static bool isIdeographic(const QChar c);
    static bool isCombiningChar(const QChar c);
};

#endif