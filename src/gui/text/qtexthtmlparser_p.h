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

#ifndef QTEXTHTMLPARSER_P_H
#define QTEXTHTMLPARSER_P_H

#include <qvector.h>
#include <qcolor.h>
#include <qfont.h>
#include <private/qtextformat_p.h>
#include <private/qtextdocument_p.h>

#include "qtextdocument.h"
#include "qtextcursor.h"

enum QTextHTMLElements {
    Html_qt,
    Html_body,

    Html_a,
    Html_em,
    Html_i,
    Html_big,
    Html_small,
    Html_strong,
    Html_b,

    Html_h1,
    Html_h2,
    Html_h3,
    Html_h4,
    Html_h5,
    Html_h6,
    Html_p,
    Html_center,

    Html_font,

    Html_ul,
    Html_ol,
    Html_li,

    Html_code,
    Html_tt,

    Html_img,
    Html_br,
    Html_hr,

    Html_sub,
    Html_sup,

    Html_pre,
    Html_blockquote,
    Html_head,
    Html_div,
    Html_span,
    Html_dl,
    Html_dt,
    Html_dd,
    Html_u,
    Html_s,
    Html_nobr,

    // tables
    Html_table,
    Html_tr,
    Html_td,
    Html_th,
    Html_html,

    // misc...
    Html_style,
    Html_title,

    Html_NumElements
};

class QTextHtmlParser;
struct QTextHtmlParserAttribute {
    enum {
        Add,
        Attributes,
        And,
        Styles,
        Here,
        That,
        The,
        Parser,
        Does,
        Not,
        Resolve,
        Eg,
        Tables
    } id;
    QString value;
};
Q_DECLARE_TYPEINFO(QTextHtmlParserAttribute, Q_MOVABLE_TYPE);

struct QTextHtmlParserNode {
    enum WhiteSpaceMode {
        WhiteSpaceNormal,
        WhiteSpacePre,
        WhiteSpaceNoWrap,
        WhiteSpaceModeUndefined = -1
    };

    QTextHtmlParserNode();
    QString tag;
    QString text;
    QVector<QTextHtmlParserAttribute> attributes;
    int parent;
    QVector<int> children;
    int id;
    uint isBlock : 1;
    uint isListItem : 1;
    uint isListStart : 1;
    uint isTableCell : 1;
    uint isAnchor : 1;
    uint fontItalic : 1;
    uint fontUnderline : 1;
    uint fontOverline : 1;
    uint fontStrikeOut : 1;
    uint fontFixedPitch : 1;
    QTextFrameFormat::Position cssFloat : 2;
    uint hasOwnListStyle : 1;
    QString fontFamily;
    uint hasFontPointSize : 1;
    int fontPointSize;
    int fontWeight;
    QColor color;
    QColor bgColor;
    Qt::Alignment alignment;
    QTextListFormat::Style listStyle;
    QString anchorHref;
    QString anchorName;
    QString imageName;
    int imageWidth;
    int imageHeight;
    QTextLength width;
    int tableBorder;
    int tableCellRowSpan;
    int tableCellColSpan;
    int tableCellSpacing;
    int tableCellPadding;

    int cssBlockIndent;
    uint hasCssBlockIndent : 1;
    int cssListIndent;
    uint hasCssListIndent : 1;

    QTextCharFormat charFormat() const;

    WhiteSpaceMode wsm;

    inline bool isNotSelfNesting() const
    { return id == Html_p || id == Html_li; }

    inline bool allowedInContext(int parentId) const
    {
        switch (id) {
            case Html_dd: return (parentId == Html_dt || parentId == Html_dl);
            case Html_dt: return (parentId == Html_dl);
            case Html_tr: return (parentId == Html_table);
            case Html_th:
            case Html_td: return (parentId == Html_tr);
            /*
               The end tag for li may be omitted, so turn
               <ul>
                <li>Blah
                 <ul>
               Into
               <ul>
                <li>Blah</li>
                <ul>
             */
            case Html_ul:
            case Html_ol: return (parentId != Html_li);
            default: break;
        }
        return true;
    }

    inline bool mayNotHaveChildren() const
    { return id == Html_img; }

    void initializeProperties(const QTextHtmlParserNode *parent, const QTextHtmlParser *parser);

private:
    bool isNestedList(const QTextHtmlParser *parser) const;

    int margin[5];
    friend class QTextHtmlParser;
};
Q_DECLARE_TYPEINFO(QTextHtmlParserNode, Q_MOVABLE_TYPE);


class QTextHtmlParser
{
public:
    enum Margin {
        MarginLeft,
        MarginRight,
        MarginTop,
        MarginBottom,
        MarginFirstLine
    };

    inline const QTextHtmlParserNode &at(int i) const { return nodes.at(i); }
    inline QTextHtmlParserNode &operator[](int i) { return nodes[i]; }
    inline int count() const { return nodes.count(); }
    inline int last() const { return nodes.count()-1; }
    int depth(int i) const;
    int topMargin(int i) const;
    int bottomMargin(int i) const;
    inline int leftMargin(int i) const { return margin(i, MarginLeft); }
    inline int rightMargin(int i) const { return margin(i, MarginRight); }
    inline int firstLineMargin(int i) const { return margin(i, MarginFirstLine); }

    void dumpHtml();

    void parse(const QString &text);

    static int lookupElement(const QString &element);
protected:
    QTextHtmlParserNode *newNode(int parent);
    QVector<QTextHtmlParserNode> nodes;
    QString txt;
    int pos, len;

    void parse();
    void parseTag();
    void parseCloseTag();
    void parseExclamationTag();
    QChar parseEntity();
    QString parseWord();
    void resolveParent();
    void resolveNode();
    void parseAttributes();
    void eatSpace();
    inline bool hasPrefix(QChar c, int lookahead = 0) const
        {return pos + lookahead < len && txt.at(pos) == c; }
    int margin(int i, int mar) const;
};

#endif // QTEXTHTMLPARSER_P_H
