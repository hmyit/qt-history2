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

#include "domtool.h"

#include <qsizepolicy.h>
#include <qcolor.h>
#include <qcursor.h>
#include <qdatetime.h>
#include <qrect.h>
#include <qsize.h>
#include <qfont.h>
#include <qdom.h>
#include <qbytearray.h>
#include <qdebug.h>

/*
  \class DomTool domtool.h
  \brief The DomTool class provides static functions for Qt Designer
  and uic.

  A collection of static functions used by Resource (part of the
  designer) and Uic.

*/

/*
  Returns the contents of property \a name of object \a e as
  a variant or the variant passed as \a defValue if the property does
  not exist. The \a comment is passed on to the elementToVariant()
  function.

  \sa hasProperty()
*/
QCoreVariant DomTool::readProperty(const QDomElement& e, const QString& name, const QCoreVariant& defValue, QString& comment)
{
    QDomElement n;
    for (n = e.firstChild().toElement(); !n.isNull(); n = n.nextSibling().toElement()) {
        if (n.tagName() == QLatin1String("property")) {
            if (n.attribute(QLatin1String("name")) != name)
                continue;
            return elementToVariant(n.firstChild().toElement(), defValue, comment);
        }
    }
    return defValue;
}


/*
  \overload
 */
QCoreVariant DomTool::readProperty(const QDomElement& e, const QString& name, const QCoreVariant& defValue)
{
    QString comment;
    return readProperty(e, name, defValue, comment);
}

/*
  Returns wheter object \a e defines property \a name or not.

  \sa readProperty()
 */
bool DomTool::hasProperty(const QDomElement& e, const QString& name)
{
    QDomElement n;
    for (n = e.firstChild().toElement(); !n.isNull(); n = n.nextSibling().toElement()) {
        if (n.tagName() == QLatin1String("property")) {
            if (n.attribute(QLatin1String("name")) != name)
                continue;
            return TRUE;
        }
    }
    return FALSE;
}

/*
    Returns a list of the names of the properties of the given \a type
    found in the element \a e.
*/
QStringList DomTool::propertiesOfType(const QDomElement& e, const QString& type)
{
    QStringList result;
    QDomElement n;
    for (n = e.firstChild().toElement(); !n.isNull(); n = n.nextSibling().toElement()) {
        if (n.tagName() == QLatin1String("property")) {
            QDomElement n2 = n.firstChild().toElement();
            if (n2.tagName() == type)
                result += n.attribute(QLatin1String("name"));
        }
    }
    return result;
}


/*
    \overload
*/
QCoreVariant DomTool::elementToVariant(const QDomElement& e, const QCoreVariant& defValue)
{
    QString dummy;
    return elementToVariant(e, defValue, dummy);
}

/*
  Interprets element \a e as a variant and returns the result of the
  interpretation, extracting the data as a text element is the \a
  comment matches the tag name. If the interpretation fails the \a
  defValue is returned instead.
 */
QCoreVariant DomTool::elementToVariant(const QDomElement& e, const QCoreVariant& defValue, QString &comment)
{
    Q_UNUSED(defValue);

    QCoreVariant v;
    Variant var;

    if (e.tagName() == QLatin1String("rect")) {
        QDomElement n3 = e.firstChild().toElement();
        int x = 0, y = 0, w = 0, h = 0;
        while (!n3.isNull()) {
            if (n3.tagName() == QLatin1String("x"))
                x = n3.firstChild().toText().data().toInt();
            else if (n3.tagName() == QLatin1String("y"))
                y = n3.firstChild().toText().data().toInt();
            else if (n3.tagName() == QLatin1String("width"))
                w = n3.firstChild().toText().data().toInt();
            else if (n3.tagName() == QLatin1String("height"))
                h = n3.firstChild().toText().data().toInt();
            n3 = n3.nextSibling().toElement();
        }
        var.createRect(x, y, w, h);
        qVariantSet(v, var, "Variant");
    } else if (e.tagName() == QLatin1String("point")) {
        QDomElement n3 = e.firstChild().toElement();
        int x = 0, y = 0;
        while (!n3.isNull()) {
            if (n3.tagName() == QLatin1String("x"))
                x = n3.firstChild().toText().data().toInt();
            else if (n3.tagName() == QLatin1String("y"))
                y = n3.firstChild().toText().data().toInt();
            n3 = n3.nextSibling().toElement();
        }
        var.createPoint(x,y);
        qVariantSet(v, var, "Variant");
    } else if (e.tagName() == QLatin1String("size")) {
        QDomElement n3 = e.firstChild().toElement();
        int w = 0, h = 0;
        while (!n3.isNull()) {
            if (n3.tagName() == QLatin1String("width"))
                w = n3.firstChild().toText().data().toInt();
            else if (n3.tagName() == QLatin1String("height"))
                h = n3.firstChild().toText().data().toInt();
            n3 = n3.nextSibling().toElement();
        }
        var.createSize(w, h);
        qVariantSet(v, var, "Variant");
    } else if (e.tagName() == QLatin1String("color")) {
        var.color = readColor(e);
        qVariantSet(v, var, "Variant");
    } else if (e.tagName() == QLatin1String("font")) {
        QDomElement n3 = e.firstChild().toElement();
        Font f;
        f.init();
        while (!n3.isNull()) {
            if (n3.tagName() == QLatin1String("family"))
                f.family = qstrdup(n3.firstChild().toText().data());
            else if (n3.tagName() == QLatin1String("pointsize"))
                f.pointsize = n3.firstChild().toText().data().toInt();
            else if (n3.tagName() == QLatin1String("bold"))
                f.bold = n3.firstChild().toText().data().toInt();
            else if (n3.tagName() == QLatin1String("italic"))
                f.italic = n3.firstChild().toText().data().toInt();
            else if (n3.tagName() == QLatin1String("underline"))
                f.underline = n3.firstChild().toText().data().toInt();
            else if (n3.tagName() == QLatin1String("strikeout"))
                f.strikeout = n3.firstChild().toText().data().toInt();
            n3 = n3.nextSibling().toElement();
        }
        var.font = f;
        qVariantSet(v, var, "Variant");
    } else if (e.tagName() == QLatin1String("string")) {
        v = QCoreVariant(e.firstChild().toText().data());
        QDomElement n = e;
        n = n.nextSibling().toElement();
        if (n.tagName() == QLatin1String("comment"))
            comment = n.firstChild().toText().data();
    } else if (e.tagName() == QLatin1String("cstring")) {
        v = QCoreVariant(e.firstChild().toText().data().toAscii());
    } else if (e.tagName() == QLatin1String("number")) {
        bool ok = TRUE;
        v = QCoreVariant(e.firstChild().toText().data().toInt(&ok));
        if (!ok)
            v = QCoreVariant(e.firstChild().toText().data().toDouble());
    } else if (e.tagName() == QLatin1String("bool")) {
        QString t = e.firstChild().toText().data();
        v = QCoreVariant(t == QLatin1String("true") || t == QLatin1String("1"));
    } else if (e.tagName() == QLatin1String("pixmap")) {
        v = QCoreVariant(e.firstChild().toText().data());
    } else if (e.tagName() == QLatin1String("iconset")) {
        v = QCoreVariant(e.firstChild().toText().data());
    } else if (e.tagName() == QLatin1String("image")) {
        v = QCoreVariant(e.firstChild().toText().data());
    } else if (e.tagName() == QLatin1String("enum")) {
        v = QCoreVariant(e.firstChild().toText().data());
    } else if (e.tagName() == QLatin1String("set")) {
        v = QCoreVariant(e.firstChild().toText().data());
    } else if (e.tagName() == QLatin1String("sizepolicy")) {
        QDomElement n3 = e.firstChild().toElement();
        var.createSizePolicy();
        while (!n3.isNull()) {
            if (n3.tagName() == QLatin1String("hsizetype"))
                var.sizePolicy.hsizetype = n3.firstChild().toText().data().toInt();
            else if (n3.tagName() == QLatin1String("vsizetype"))
                var.sizePolicy.vsizetype = n3.firstChild().toText().data().toInt();
            else if (n3.tagName() == QLatin1String("horstretch"))
                var.sizePolicy.horstretch = n3.firstChild().toText().data().toInt();
            else if (n3.tagName() == QLatin1String("verstretch"))
                var.sizePolicy.verstretch = n3.firstChild().toText().data().toInt();
            n3 = n3.nextSibling().toElement();
        }
        qVariantSet(v, var, "Variant");
    } else if (e.tagName() == QLatin1String("cursor")) {
        var.createCursor(e.firstChild().toText().data().toInt());
        qVariantSet(v, var, "Variant");
    } else if (e.tagName() == QLatin1String("stringlist")) {
        QStringList lst;
        QDomElement n;
        for (n = e.firstChild().toElement(); !n.isNull(); n = n.nextSibling().toElement())
            lst << n.firstChild().toText().data();
        v = QCoreVariant(lst);
    } else if (e.tagName() == QLatin1String("date")) {
        QDomElement n3 = e.firstChild().toElement();
        int y, m, d;
        y = m = d = 0;
        while (!n3.isNull()) {
            if (n3.tagName() == QLatin1String("year"))
                y = n3.firstChild().toText().data().toInt();
            else if (n3.tagName() == QLatin1String("month"))
                m = n3.firstChild().toText().data().toInt();
            else if (n3.tagName() == QLatin1String("day"))
                d = n3.firstChild().toText().data().toInt();
            n3 = n3.nextSibling().toElement();
        }
        v = QCoreVariant(QDate(y, m, d));
    } else if (e.tagName() == QLatin1String("time")) {
        QDomElement n3 = e.firstChild().toElement();
        int h, m, s;
        h = m = s = 0;
        while (!n3.isNull()) {
            if (n3.tagName() == QLatin1String("hour"))
                h = n3.firstChild().toText().data().toInt();
            else if (n3.tagName() == QLatin1String("minute"))
                m = n3.firstChild().toText().data().toInt();
            else if (n3.tagName() == QLatin1String("second"))
                s = n3.firstChild().toText().data().toInt();
            n3 = n3.nextSibling().toElement();
        }
        v = QCoreVariant(QTime(h, m, s));
    } else if (e.tagName() == QLatin1String("datetime")) {
        QDomElement n3 = e.firstChild().toElement();
        int h, mi, s, y, mo, d ;
        h = mi = s = y = mo = d = 0;
        while (!n3.isNull()) {
            if (n3.tagName() == QLatin1String("hour"))
                h = n3.firstChild().toText().data().toInt();
            else if (n3.tagName() == QLatin1String("minute"))
                mi = n3.firstChild().toText().data().toInt();
            else if (n3.tagName() == QLatin1String("second"))
                s = n3.firstChild().toText().data().toInt();
            else if (n3.tagName() == QLatin1String("year"))
                y = n3.firstChild().toText().data().toInt();
            else if (n3.tagName() == QLatin1String("month"))
                mo = n3.firstChild().toText().data().toInt();
            else if (n3.tagName() == QLatin1String("day"))
                d = n3.firstChild().toText().data().toInt();
            n3 = n3.nextSibling().toElement();
        }
        v = QCoreVariant(QDateTime(QDate(y, mo, d), QTime(h, mi, s)));
    }

    return v;
}


/*  Returns the color which is returned in the dom element \a e.
 */

Color DomTool::readColor(const QDomElement &e)
{
    QDomElement n = e.firstChild().toElement();
    int r= 0, g = 0, b = 0;
    while (!n.isNull()) {
        if (n.tagName() == QLatin1String("red"))
            r = n.firstChild().toText().data().toInt();
        else if (n.tagName() == QLatin1String("green"))
            g = n.firstChild().toText().data().toInt();
        else if (n.tagName() == QLatin1String("blue"))
            b = n.firstChild().toText().data().toInt();
        n = n.nextSibling().toElement();
    }

    Color c;
    c.init(r, g, b);
    return c;
}

/*
  Returns the contents of attribute \a name of object \a e as
  a variant or the variant passed as \a defValue if the attribute does
  not exist. The \a comment is passed to the elementToVariant()
  function.

  \sa hasAttribute()
 */
QCoreVariant DomTool::readAttribute(const QDomElement& e, const QString& name, const QCoreVariant& defValue, QString& comment)
{
    QDomElement n;
    for (n = e.firstChild().toElement(); !n.isNull(); n = n.nextSibling().toElement()) {
        if (n.tagName() == QLatin1String("attribute")) {
            if (n.attribute(QLatin1String("name")) != name)
                continue;
            return elementToVariant(n.firstChild().toElement(), defValue, comment);
        }
    }
    return defValue;
}

/*
  \overload
*/
QCoreVariant DomTool::readAttribute(const QDomElement& e, const QString& name, const QCoreVariant& defValue)
{
    QString comment;
    return readAttribute(e, name, defValue, comment);
}

/*
  Returns wheter object \a e defines attribute \a name or not.

  \sa readAttribute()
 */
bool DomTool::hasAttribute(const QDomElement& e, const QString& name)
{
    QDomElement n;
    for (n = e.firstChild().toElement(); !n.isNull(); n = n.nextSibling().toElement()) {
        if (n.tagName() == QLatin1String("attribute")) {
            if (n.attribute(QLatin1String("name")) != name)
                continue;
            return TRUE;
        }
    }
    return FALSE;
}

static bool toBool(const QString& s)
{
    return s == QLatin1String("true") || s.toInt() != 0;
}

/*
    \internal

    Convert Qt 2.x format to Qt 3.x format if necessary.
*/
void DomTool::fixDocument(QDomDocument& doc)
{
    QDomElement e;
    QDomNode n;
    QDomNodeList nl;
    int i = 0;

    e = doc.firstChild().toElement();
    if (e.tagName() != QLatin1String("UI"))
        return;

    // rename classes and properties
    double version = e.hasAttribute(QLatin1String("version")) 
        ? e.attribute(QLatin1String("version")).toDouble() : 0.0;
        
    nl = e.childNodes();
    fixAttributes(nl, version);

    // 3.x don't do anything more
    if (e.hasAttribute(QLatin1String("version")) 
            && e.attribute(QLatin1String("version")).toDouble() >= 3.0)
        return;

    // in versions smaller than 3.0 we need to change more

    e.setAttribute(QLatin1String("version"), 3.0);
    e.setAttribute(QLatin1String("stdsetdef"), 1);
    nl = e.elementsByTagName(QLatin1String("property"));
    for (i = 0; i <  (int) nl.length(); i++) {
        e = nl.item(i).toElement();
        QString name;
        QDomElement n2 = e.firstChild().toElement();
        if (n2.tagName() == QLatin1String("name")) {
            name = n2.firstChild().toText().data();
            if (name == QLatin1String("resizeable"))
                e.setAttribute(QLatin1String("name"), QLatin1String("resizable"));
            else
                e.setAttribute(QLatin1String("name"), name);
            e.removeChild(n2);
        }
        bool stdset = toBool(e.attribute(QLatin1String("stdset")));
        if (stdset || name == QLatin1String("toolTip") || name == QLatin1String("whatsThis") ||
             name == QLatin1String("buddy") ||
             e.parentNode().toElement().tagName() == QLatin1String("item") ||
             e.parentNode().toElement().tagName() == QLatin1String("spacer") ||
             e.parentNode().toElement().tagName() == QLatin1String("column")
            )
            e.removeAttribute(QLatin1String("stdset"));
        else
            e.setAttribute(QLatin1String("stdset"), 0);
    }

    nl = doc.elementsByTagName(QLatin1String("attribute"));
    for (i = 0; i <  (int) nl.length(); i++) {
        e = nl.item(i).toElement();
        QString name;
        QDomElement n2 = e.firstChild().toElement();
        if (n2.tagName() == QLatin1String("name")) {
            name = n2.firstChild().toText().data();
            e.setAttribute(QLatin1String("name"), name);
            e.removeChild(n2);
        }
    }

    nl = doc.elementsByTagName(QLatin1String("image"));
    for (i = 0; i <  (int) nl.length(); i++) {
        e = nl.item(i).toElement();
        QString name;
        QDomElement n2 = e.firstChild().toElement();
        if (n2.tagName() == QLatin1String("name")) {
            name = n2.firstChild().toText().data();
            e.setAttribute(QLatin1String("name"), name);
            e.removeChild(n2);
        }
    }

    nl = doc.elementsByTagName(QLatin1String("widget"));
    for (i = 0; i <  (int) nl.length(); i++) {
        e = nl.item(i).toElement();
        QString name;
        QDomElement n2 = e.firstChild().toElement();
        if (n2.tagName() == QLatin1String("class")) {
            name = n2.firstChild().toText().data();
            e.setAttribute(QLatin1String("class"), name);
            e.removeChild(n2);
        }
    }

}

struct widgetName {
    widgetName(double v, QString b, QString a)
        : version(v), before(b), after(a) {}
    double version;
    QString before;
    QString after;
};

struct propertyName : public widgetName {
    propertyName(double v, QString b, QString a, QString c = QString::null)
        : widgetName(v, b, a), clss(c) {}
    QString clss;
};

const int widgs = 1;
widgetName widgetTable[1] = {
    widgetName(3.3, QLatin1String("before"), QLatin1String("after")),
};

const int props = 1;
propertyName propertyTable[1] = {
    propertyName(3.0, QLatin1String("resizeable"), QLatin1String("resizable")), // we need to fix a spelling error in 3.0
};

/*
    \internal
*/
void DomTool::fixAttributes(QDomNodeList &nodes, double version)
{
    QDomNode n;
    QDomNodeList nl;
    for (int i = 0; i < (int) nodes.count(); ++i) {
        n = nodes.item(i);
        fixAttribute(n, version);
        nl = n.childNodes();
        fixAttributes(nl, version);
    }
}

/*
    \internal
*/
void DomTool::fixAttribute(QDomNode &node, double version)
{
    QString tagName =  node.toElement().tagName();
    if (tagName == QLatin1String("widget")) {
        QString clss = node.toElement().attribute(QLatin1String("class"));
        for (int i = 0; i < widgs; ++i)
            if ((version < widgetTable[i].version)
                 && (clss == widgetTable[i].before)) {
                node.toElement().setAttribute(QLatin1String("class"), propertyTable[i].after);
                return;
            }
        return;
    }
    if (tagName == QLatin1String("property")) {
        QDomElement e = node.parentNode().toElement();
        QString clss = e.attribute(QLatin1String("class"));
        QString name = node.toElement().attribute(QLatin1String("name"), QLatin1String(""));
        for (int i = 0; i < props; ++i)
            if ((version < propertyTable[i].version)
                 && (clss == propertyTable[i].clss)
                 && (propertyTable[i].before.isNull()
                      || name == propertyTable[i].before)) {
                node.toElement().setAttribute(QLatin1String("name"), propertyTable[i].after);
                return;
            }
    }
}
