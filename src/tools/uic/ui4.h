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

#ifndef UI4_H
#define UI4_H

#include <qstring.h>
#include <qdom.h>
#include <qlist.h>
#include <qalgorithms.h>
#include <qstringlist.h>

class DomUI;
class DomIncludes;
class DomInclude;
class DomActionGroup;
class DomAction;
class DomActionRef;
class DomImages;
class DomImage;
class DomImageData;
class DomCustomWidgets;
class DomHeader;
class DomCustomWidget;
class DomProperties;
class DomPropertyData;
class DomSizePolicyData;
class DomLayoutDefault;
class DomLayoutFunction;
class DomTabStops;
class DomLayout;
class DomLayoutItem;
class DomRow;
class DomColumn;
class DomItem;
class DomWidget;
class DomSpacer;
class DomColor;
class DomColorGroup;
class DomPalette;
class DomFont;
class DomPoint;
class DomRect;
class DomSizePolicy;
class DomSize;
class DomDate;
class DomTime;
class DomDateTime;
class DomStringList;
class DomString;
class DomProperty;


class DomUI
{
    DomUI(const DomUI &other);
    void operator = (const DomUI &other);

public:
    DomUI();
    ~DomUI();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QString attributeVersion() const { return m_aVersion; }
    inline bool hasAttributeVersion() const { return m_hasVersion; }
    inline void setAttributeVersion(const QString & a) { m_aVersion = a; m_hasVersion = true; }
    inline void clearAttributeVersion() { m_hasVersion = false; }

    inline int attributeStdSetDef() const { return m_aStdSetDef; }
    inline bool hasAttributeStdSetDef() const { return m_hasStdSetDef; }
    inline void setAttributeStdSetDef(int a) { m_aStdSetDef = a; m_hasStdSetDef = true; }
    inline void clearAttributeStdSetDef() { m_hasStdSetDef = false; }

    inline QString elementAuthor() const { return m_eAuthor; }
    inline void setElementAuthor(const QString & a) { m_eAuthor = a; };

    inline QString elementComment() const { return m_eComment; }
    inline void setElementComment(const QString & a) { m_eComment = a; };

    inline QString elementExportMacro() const { return m_eExportMacro; }
    inline void setElementExportMacro(const QString & a) { m_eExportMacro = a; };

    inline QString elementClass() const { return m_eClass; }
    inline void setElementClass(const QString & a) { m_eClass = a; };

    inline DomWidget * elementWidget() const { return m_eWidget; }
    inline void setElementWidget(DomWidget * a) { m_eWidget = a; };

    inline DomLayoutDefault * elementLayoutDefault() const { return m_eLayoutDefault; }
    inline void setElementLayoutDefault(DomLayoutDefault * a) { m_eLayoutDefault = a; };

    inline DomLayoutFunction * elementLayoutFunction() const { return m_eLayoutFunction; }
    inline void setElementLayoutFunction(DomLayoutFunction * a) { m_eLayoutFunction = a; };

    inline QString elementPixmapFunction() const { return m_ePixmapFunction; }
    inline void setElementPixmapFunction(const QString & a) { m_ePixmapFunction = a; };

    inline DomCustomWidgets * elementCustomWidgets() const { return m_eCustomWidgets; }
    inline void setElementCustomWidgets(DomCustomWidgets * a) { m_eCustomWidgets = a; };

    inline DomTabStops * elementTabStops() const { return m_eTabStops; }
    inline void setElementTabStops(DomTabStops * a) { m_eTabStops = a; };

    inline DomImages * elementImages() const { return m_eImages; }
    inline void setElementImages(DomImages * a) { m_eImages = a; };

    inline DomIncludes * elementIncludes() const { return m_eIncludes; }
    inline void setElementIncludes(DomIncludes * a) { m_eIncludes = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes
    QString m_aVersion;
    bool m_hasVersion;
    int m_aStdSetDef;
    bool m_hasStdSetDef;

    // elements
    QString m_eAuthor;
    QString m_eComment;
    QString m_eExportMacro;
    QString m_eClass;
    DomWidget * m_eWidget;
    DomLayoutDefault * m_eLayoutDefault;
    DomLayoutFunction * m_eLayoutFunction;
    QString m_ePixmapFunction;
    DomCustomWidgets * m_eCustomWidgets;
    DomTabStops * m_eTabStops;
    DomImages * m_eImages;
    DomIncludes * m_eIncludes;
};

class DomIncludes
{
    DomIncludes(const DomIncludes &other);
    void operator = (const DomIncludes &other);

public:
    DomIncludes();
    ~DomIncludes();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QList<DomInclude *> elementInclude() const { return m_eInclude; }
    inline void setElementInclude(const QList<DomInclude *> & a) { m_eInclude = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    QList<DomInclude *> m_eInclude;
};

class DomInclude
{
    DomInclude(const DomInclude &other);
    void operator = (const DomInclude &other);

public:
    DomInclude();
    ~DomInclude();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QString attributeLocation() const { return m_aLocation; }
    inline bool hasAttributeLocation() const { return m_hasLocation; }
    inline void setAttributeLocation(const QString & a) { m_aLocation = a; m_hasLocation = true; }
    inline void clearAttributeLocation() { m_hasLocation = false; }

    inline QString attributeImpldecl() const { return m_aImpldecl; }
    inline bool hasAttributeImpldecl() const { return m_hasImpldecl; }
    inline void setAttributeImpldecl(const QString & a) { m_aImpldecl = a; m_hasImpldecl = true; }
    inline void clearAttributeImpldecl() { m_hasImpldecl = false; }

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes
    QString m_aLocation;
    bool m_hasLocation;
    QString m_aImpldecl;
    bool m_hasImpldecl;

    // elements
};

class DomActionGroup
{
    DomActionGroup(const DomActionGroup &other);
    void operator = (const DomActionGroup &other);

public:
    DomActionGroup();
    ~DomActionGroup();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QString attributeName() const { return m_aName; }
    inline bool hasAttributeName() const { return m_hasName; }
    inline void setAttributeName(const QString & a) { m_aName = a; m_hasName = true; }
    inline void clearAttributeName() { m_hasName = false; }

    inline QList<DomAction *> elementAction() const { return m_eAction; }
    inline void setElementAction(const QList<DomAction *> & a) { m_eAction = a; };

    inline QList<DomActionGroup *> elementActionGroup() const { return m_eActionGroup; }
    inline void setElementActionGroup(const QList<DomActionGroup *> & a) { m_eActionGroup = a; };

    inline QList<DomProperty *> elementProperty() const { return m_eProperty; }
    inline void setElementProperty(const QList<DomProperty *> & a) { m_eProperty = a; };

    inline QList<DomProperty *> elementAttribute() const { return m_eAttribute; }
    inline void setElementAttribute(const QList<DomProperty *> & a) { m_eAttribute = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes
    QString m_aName;
    bool m_hasName;

    // elements
    QList<DomAction *> m_eAction;
    QList<DomActionGroup *> m_eActionGroup;
    QList<DomProperty *> m_eProperty;
    QList<DomProperty *> m_eAttribute;
};

class DomAction
{
    DomAction(const DomAction &other);
    void operator = (const DomAction &other);

public:
    DomAction();
    ~DomAction();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QString attributeName() const { return m_aName; }
    inline bool hasAttributeName() const { return m_hasName; }
    inline void setAttributeName(const QString & a) { m_aName = a; m_hasName = true; }
    inline void clearAttributeName() { m_hasName = false; }

    inline QString attributeMenu() const { return m_aMenu; }
    inline bool hasAttributeMenu() const { return m_hasMenu; }
    inline void setAttributeMenu(const QString & a) { m_aMenu = a; m_hasMenu = true; }
    inline void clearAttributeMenu() { m_hasMenu = false; }

    inline QList<DomProperty *> elementProperty() const { return m_eProperty; }
    inline void setElementProperty(const QList<DomProperty *> & a) { m_eProperty = a; };

    inline QList<DomProperty *> elementAttribute() const { return m_eAttribute; }
    inline void setElementAttribute(const QList<DomProperty *> & a) { m_eAttribute = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes
    QString m_aName;
    bool m_hasName;
    QString m_aMenu;
    bool m_hasMenu;

    // elements
    QList<DomProperty *> m_eProperty;
    QList<DomProperty *> m_eAttribute;
};

class DomActionRef
{
    DomActionRef(const DomActionRef &other);
    void operator = (const DomActionRef &other);

public:
    DomActionRef();
    ~DomActionRef();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QString attributeName() const { return m_aName; }
    inline bool hasAttributeName() const { return m_hasName; }
    inline void setAttributeName(const QString & a) { m_aName = a; m_hasName = true; }
    inline void clearAttributeName() { m_hasName = false; }

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes
    QString m_aName;
    bool m_hasName;

    // elements
};

class DomImages
{
    DomImages(const DomImages &other);
    void operator = (const DomImages &other);

public:
    DomImages();
    ~DomImages();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QList<DomImage *> elementImage() const { return m_eImage; }
    inline void setElementImage(const QList<DomImage *> & a) { m_eImage = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    QList<DomImage *> m_eImage;
};

class DomImage
{
    DomImage(const DomImage &other);
    void operator = (const DomImage &other);

public:
    DomImage();
    ~DomImage();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QString attributeName() const { return m_aName; }
    inline bool hasAttributeName() const { return m_hasName; }
    inline void setAttributeName(const QString & a) { m_aName = a; m_hasName = true; }
    inline void clearAttributeName() { m_hasName = false; }

    inline DomImageData * elementData() const { return m_eData; }
    inline void setElementData(DomImageData * a) { m_eData = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes
    QString m_aName;
    bool m_hasName;

    // elements
    DomImageData * m_eData;
};

class DomImageData
{
    DomImageData(const DomImageData &other);
    void operator = (const DomImageData &other);

public:
    DomImageData();
    ~DomImageData();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QString attributeFormat() const { return m_aFormat; }
    inline bool hasAttributeFormat() const { return m_hasFormat; }
    inline void setAttributeFormat(const QString & a) { m_aFormat = a; m_hasFormat = true; }
    inline void clearAttributeFormat() { m_hasFormat = false; }

    inline int attributeLength() const { return m_aLength; }
    inline bool hasAttributeLength() const { return m_hasLength; }
    inline void setAttributeLength(int a) { m_aLength = a; m_hasLength = true; }
    inline void clearAttributeLength() { m_hasLength = false; }

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes
    QString m_aFormat;
    bool m_hasFormat;
    int m_aLength;
    bool m_hasLength;

    // elements
};

class DomCustomWidgets
{
    DomCustomWidgets(const DomCustomWidgets &other);
    void operator = (const DomCustomWidgets &other);

public:
    DomCustomWidgets();
    ~DomCustomWidgets();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QList<DomCustomWidget *> elementCustomWidget() const { return m_eCustomWidget; }
    inline void setElementCustomWidget(const QList<DomCustomWidget *> & a) { m_eCustomWidget = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    QList<DomCustomWidget *> m_eCustomWidget;
};

class DomHeader
{
    DomHeader(const DomHeader &other);
    void operator = (const DomHeader &other);

public:
    DomHeader();
    ~DomHeader();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QString attributeLocation() const { return m_aLocation; }
    inline bool hasAttributeLocation() const { return m_hasLocation; }
    inline void setAttributeLocation(const QString & a) { m_aLocation = a; m_hasLocation = true; }
    inline void clearAttributeLocation() { m_hasLocation = false; }

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes
    QString m_aLocation;
    bool m_hasLocation;

    // elements
};

class DomCustomWidget
{
    DomCustomWidget(const DomCustomWidget &other);
    void operator = (const DomCustomWidget &other);

public:
    DomCustomWidget();
    ~DomCustomWidget();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QString elementClass() const { return m_eClass; }
    inline void setElementClass(const QString & a) { m_eClass = a; };

    inline QString elementExtends() const { return m_eExtends; }
    inline void setElementExtends(const QString & a) { m_eExtends = a; };

    inline DomHeader * elementHeader() const { return m_eHeader; }
    inline void setElementHeader(DomHeader * a) { m_eHeader = a; };

    inline DomSize * elementSizeHint() const { return m_eSizeHint; }
    inline void setElementSizeHint(DomSize * a) { m_eSizeHint = a; };

    inline int elementContainer() const { return m_eContainer; }
    inline void setElementContainer(int a) { m_eContainer = a; };

    inline DomSizePolicyData * elementSizePolicy() const { return m_eSizePolicy; }
    inline void setElementSizePolicy(DomSizePolicyData * a) { m_eSizePolicy = a; };

    inline QString elementPixmap() const { return m_ePixmap; }
    inline void setElementPixmap(const QString & a) { m_ePixmap = a; };

    inline DomProperties * elementProperties() const { return m_eProperties; }
    inline void setElementProperties(DomProperties * a) { m_eProperties = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    QString m_eClass;
    QString m_eExtends;
    DomHeader * m_eHeader;
    DomSize * m_eSizeHint;
    int m_eContainer;
    DomSizePolicyData * m_eSizePolicy;
    QString m_ePixmap;
    DomProperties * m_eProperties;
};

class DomProperties
{
    DomProperties(const DomProperties &other);
    void operator = (const DomProperties &other);

public:
    DomProperties();
    ~DomProperties();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QList<DomPropertyData *> elementProperty() const { return m_eProperty; }
    inline void setElementProperty(const QList<DomPropertyData *> & a) { m_eProperty = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    QList<DomPropertyData *> m_eProperty;
};

class DomPropertyData
{
    DomPropertyData(const DomPropertyData &other);
    void operator = (const DomPropertyData &other);

public:
    DomPropertyData();
    ~DomPropertyData();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QString attributeType() const { return m_aType; }
    inline bool hasAttributeType() const { return m_hasType; }
    inline void setAttributeType(const QString & a) { m_aType = a; m_hasType = true; }
    inline void clearAttributeType() { m_hasType = false; }

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes
    QString m_aType;
    bool m_hasType;

    // elements
};

class DomSizePolicyData
{
    DomSizePolicyData(const DomSizePolicyData &other);
    void operator = (const DomSizePolicyData &other);

public:
    DomSizePolicyData();
    ~DomSizePolicyData();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline int elementHorData() const { return m_eHorData; }
    inline void setElementHorData(int a) { m_eHorData = a; };

    inline int elementVerData() const { return m_eVerData; }
    inline void setElementVerData(int a) { m_eVerData = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    int m_eHorData;
    int m_eVerData;
};

class DomLayoutDefault
{
    DomLayoutDefault(const DomLayoutDefault &other);
    void operator = (const DomLayoutDefault &other);

public:
    DomLayoutDefault();
    ~DomLayoutDefault();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline int attributeSpacing() const { return m_aSpacing; }
    inline bool hasAttributeSpacing() const { return m_hasSpacing; }
    inline void setAttributeSpacing(int a) { m_aSpacing = a; m_hasSpacing = true; }
    inline void clearAttributeSpacing() { m_hasSpacing = false; }

    inline int attributeMargin() const { return m_aMargin; }
    inline bool hasAttributeMargin() const { return m_hasMargin; }
    inline void setAttributeMargin(int a) { m_aMargin = a; m_hasMargin = true; }
    inline void clearAttributeMargin() { m_hasMargin = false; }

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes
    int m_aSpacing;
    bool m_hasSpacing;
    int m_aMargin;
    bool m_hasMargin;

    // elements
};

class DomLayoutFunction
{
    DomLayoutFunction(const DomLayoutFunction &other);
    void operator = (const DomLayoutFunction &other);

public:
    DomLayoutFunction();
    ~DomLayoutFunction();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QString attributeSpacing() const { return m_aSpacing; }
    inline bool hasAttributeSpacing() const { return m_hasSpacing; }
    inline void setAttributeSpacing(const QString & a) { m_aSpacing = a; m_hasSpacing = true; }
    inline void clearAttributeSpacing() { m_hasSpacing = false; }

    inline QString attributeMargin() const { return m_aMargin; }
    inline bool hasAttributeMargin() const { return m_hasMargin; }
    inline void setAttributeMargin(const QString & a) { m_aMargin = a; m_hasMargin = true; }
    inline void clearAttributeMargin() { m_hasMargin = false; }

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes
    QString m_aSpacing;
    bool m_hasSpacing;
    QString m_aMargin;
    bool m_hasMargin;

    // elements
};

class DomTabStops
{
    DomTabStops(const DomTabStops &other);
    void operator = (const DomTabStops &other);

public:
    DomTabStops();
    ~DomTabStops();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QStringList elementTabStop() const { return m_eTabStop; }
    inline void setElementTabStop(const QStringList & a) { m_eTabStop = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    QStringList m_eTabStop;
};

class DomLayout
{
    DomLayout(const DomLayout &other);
    void operator = (const DomLayout &other);

public:
    DomLayout();
    ~DomLayout();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QString attributeClass() const { return m_aClass; }
    inline bool hasAttributeClass() const { return m_hasClass; }
    inline void setAttributeClass(const QString & a) { m_aClass = a; m_hasClass = true; }
    inline void clearAttributeClass() { m_hasClass = false; }

    inline QList<DomProperty *> elementProperty() const { return m_eProperty; }
    inline void setElementProperty(const QList<DomProperty *> & a) { m_eProperty = a; };

    inline QList<DomProperty *> elementAttribute() const { return m_eAttribute; }
    inline void setElementAttribute(const QList<DomProperty *> & a) { m_eAttribute = a; };

    inline QList<DomLayoutItem *> elementItem() const { return m_eItem; }
    inline void setElementItem(const QList<DomLayoutItem *> & a) { m_eItem = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes
    QString m_aClass;
    bool m_hasClass;

    // elements
    QList<DomProperty *> m_eProperty;
    QList<DomProperty *> m_eAttribute;
    QList<DomLayoutItem *> m_eItem;
};

class DomLayoutItem
{
    DomLayoutItem(const DomLayoutItem &other);
    void operator = (const DomLayoutItem &other);

public:
    DomLayoutItem();
    ~DomLayoutItem();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    enum Kind {
        Unknown = 0,
        Widget,
        Layout,
        Spacer
    };

    inline Kind kind() const { return m_kind; }

    inline int attributeRow() const { return m_aRow; }
    inline bool hasAttributeRow() const { return m_hasRow; }
    inline void setAttributeRow(int a) { m_aRow = a; m_hasRow = true; }
    inline void clearAttributeRow() { m_hasRow = false; }

    inline int attributeColumn() const { return m_aColumn; }
    inline bool hasAttributeColumn() const { return m_hasColumn; }
    inline void setAttributeColumn(int a) { m_aColumn = a; m_hasColumn = true; }
    inline void clearAttributeColumn() { m_hasColumn = false; }

    inline int attributeRowSpan() const { return m_aRowSpan; }
    inline bool hasAttributeRowSpan() const { return m_hasRowSpan; }
    inline void setAttributeRowSpan(int a) { m_aRowSpan = a; m_hasRowSpan = true; }
    inline void clearAttributeRowSpan() { m_hasRowSpan = false; }

    inline int attributeColSpan() const { return m_aColSpan; }
    inline bool hasAttributeColSpan() const { return m_hasColSpan; }
    inline void setAttributeColSpan(int a) { m_aColSpan = a; m_hasColSpan = true; }
    inline void clearAttributeColSpan() { m_hasColSpan = false; }

    inline DomWidget * elementWidget() const { return m_eWidget; }
    inline void setElementWidget(DomWidget * a) { reset(false); m_eWidget = a; m_kind = Widget; };

    inline DomLayout * elementLayout() const { return m_eLayout; }
    inline void setElementLayout(DomLayout * a) { reset(false); m_eLayout = a; m_kind = Layout; };

    inline DomSpacer * elementSpacer() const { return m_eSpacer; }
    inline void setElementSpacer(DomSpacer * a) { reset(false); m_eSpacer = a; m_kind = Spacer; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);
    Kind m_kind;

    // attributes
    int m_aRow;
    bool m_hasRow;
    int m_aColumn;
    bool m_hasColumn;
    int m_aRowSpan;
    bool m_hasRowSpan;
    int m_aColSpan;
    bool m_hasColSpan;

    // elements
    DomWidget * m_eWidget;
    DomLayout * m_eLayout;
    DomSpacer * m_eSpacer;
};

class DomRow
{
    DomRow(const DomRow &other);
    void operator = (const DomRow &other);

public:
    DomRow();
    ~DomRow();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QList<DomProperty *> elementProperty() const { return m_eProperty; }
    inline void setElementProperty(const QList<DomProperty *> & a) { m_eProperty = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    QList<DomProperty *> m_eProperty;
};

class DomColumn
{
    DomColumn(const DomColumn &other);
    void operator = (const DomColumn &other);

public:
    DomColumn();
    ~DomColumn();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QList<DomProperty *> elementProperty() const { return m_eProperty; }
    inline void setElementProperty(const QList<DomProperty *> & a) { m_eProperty = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    QList<DomProperty *> m_eProperty;
};

class DomItem
{
    DomItem(const DomItem &other);
    void operator = (const DomItem &other);

public:
    DomItem();
    ~DomItem();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QList<DomProperty *> elementProperty() const { return m_eProperty; }
    inline void setElementProperty(const QList<DomProperty *> & a) { m_eProperty = a; };

    inline QList<DomItem *> elementItem() const { return m_eItem; }
    inline void setElementItem(const QList<DomItem *> & a) { m_eItem = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    QList<DomProperty *> m_eProperty;
    QList<DomItem *> m_eItem;
};

class DomWidget
{
    DomWidget(const DomWidget &other);
    void operator = (const DomWidget &other);

public:
    DomWidget();
    ~DomWidget();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QString attributeClass() const { return m_aClass; }
    inline bool hasAttributeClass() const { return m_hasClass; }
    inline void setAttributeClass(const QString & a) { m_aClass = a; m_hasClass = true; }
    inline void clearAttributeClass() { m_hasClass = false; }

    inline QString attributeName() const { return m_aName; }
    inline bool hasAttributeName() const { return m_hasName; }
    inline void setAttributeName(const QString & a) { m_aName = a; m_hasName = true; }
    inline void clearAttributeName() { m_hasName = false; }

    inline QStringList elementClass() const { return m_eClass; }
    inline void setElementClass(const QStringList & a) { m_eClass = a; };

    inline QList<DomProperty *> elementProperty() const { return m_eProperty; }
    inline void setElementProperty(const QList<DomProperty *> & a) { m_eProperty = a; };

    inline QList<DomProperty *> elementAttribute() const { return m_eAttribute; }
    inline void setElementAttribute(const QList<DomProperty *> & a) { m_eAttribute = a; };

    inline QList<DomRow *> elementRow() const { return m_eRow; }
    inline void setElementRow(const QList<DomRow *> & a) { m_eRow = a; };

    inline QList<DomColumn *> elementColumn() const { return m_eColumn; }
    inline void setElementColumn(const QList<DomColumn *> & a) { m_eColumn = a; };

    inline QList<DomItem *> elementItem() const { return m_eItem; }
    inline void setElementItem(const QList<DomItem *> & a) { m_eItem = a; };

    inline QList<DomLayout *> elementLayout() const { return m_eLayout; }
    inline void setElementLayout(const QList<DomLayout *> & a) { m_eLayout = a; };

    inline QList<DomWidget *> elementWidget() const { return m_eWidget; }
    inline void setElementWidget(const QList<DomWidget *> & a) { m_eWidget = a; };

    inline QList<DomAction *> elementAction() const { return m_eAction; }
    inline void setElementAction(const QList<DomAction *> & a) { m_eAction = a; };

    inline QList<DomActionGroup *> elementActionGroup() const { return m_eActionGroup; }
    inline void setElementActionGroup(const QList<DomActionGroup *> & a) { m_eActionGroup = a; };

    inline QList<DomActionRef *> elementAddAction() const { return m_eAddAction; }
    inline void setElementAddAction(const QList<DomActionRef *> & a) { m_eAddAction = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes
    QString m_aClass;
    bool m_hasClass;
    QString m_aName;
    bool m_hasName;

    // elements
    QStringList m_eClass;
    QList<DomProperty *> m_eProperty;
    QList<DomProperty *> m_eAttribute;
    QList<DomRow *> m_eRow;
    QList<DomColumn *> m_eColumn;
    QList<DomItem *> m_eItem;
    QList<DomLayout *> m_eLayout;
    QList<DomWidget *> m_eWidget;
    QList<DomAction *> m_eAction;
    QList<DomActionGroup *> m_eActionGroup;
    QList<DomActionRef *> m_eAddAction;
};

class DomSpacer
{
    DomSpacer(const DomSpacer &other);
    void operator = (const DomSpacer &other);

public:
    DomSpacer();
    ~DomSpacer();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QString attributeName() const { return m_aName; }
    inline bool hasAttributeName() const { return m_hasName; }
    inline void setAttributeName(const QString & a) { m_aName = a; m_hasName = true; }
    inline void clearAttributeName() { m_hasName = false; }

    inline QList<DomProperty *> elementProperty() const { return m_eProperty; }
    inline void setElementProperty(const QList<DomProperty *> & a) { m_eProperty = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes
    QString m_aName;
    bool m_hasName;

    // elements
    QList<DomProperty *> m_eProperty;
};

class DomColor
{
    DomColor(const DomColor &other);
    void operator = (const DomColor &other);

public:
    DomColor();
    ~DomColor();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline int elementRed() const { return m_eRed; }
    inline void setElementRed(int a) { m_eRed = a; };

    inline int elementGreen() const { return m_eGreen; }
    inline void setElementGreen(int a) { m_eGreen = a; };

    inline int elementBlue() const { return m_eBlue; }
    inline void setElementBlue(int a) { m_eBlue = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    int m_eRed;
    int m_eGreen;
    int m_eBlue;
};

class DomColorGroup
{
    DomColorGroup(const DomColorGroup &other);
    void operator = (const DomColorGroup &other);

public:
    DomColorGroup();
    ~DomColorGroup();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QList<DomColor *> elementColor() const { return m_eColor; }
    inline void setElementColor(const QList<DomColor *> & a) { m_eColor = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    QList<DomColor *> m_eColor;
};

class DomPalette
{
    DomPalette(const DomPalette &other);
    void operator = (const DomPalette &other);

public:
    DomPalette();
    ~DomPalette();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline DomColorGroup * elementActive() const { return m_eActive; }
    inline void setElementActive(DomColorGroup * a) { m_eActive = a; };

    inline DomColorGroup * elementInactive() const { return m_eInactive; }
    inline void setElementInactive(DomColorGroup * a) { m_eInactive = a; };

    inline DomColorGroup * elementDisabled() const { return m_eDisabled; }
    inline void setElementDisabled(DomColorGroup * a) { m_eDisabled = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    DomColorGroup * m_eActive;
    DomColorGroup * m_eInactive;
    DomColorGroup * m_eDisabled;
};

class DomFont
{
    DomFont(const DomFont &other);
    void operator = (const DomFont &other);

public:
    DomFont();
    ~DomFont();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QString elementFamily() const { return m_eFamily; }
    inline void setElementFamily(const QString & a) { m_eFamily = a; };

    inline int elementPointSize() const { return m_ePointSize; }
    inline void setElementPointSize(int a) { m_ePointSize = a; };

    inline int elementWeight() const { return m_eWeight; }
    inline void setElementWeight(int a) { m_eWeight = a; };

    inline bool elementItalic() const { return m_eItalic; }
    inline void setElementItalic(bool a) { m_eItalic = a; };

    inline bool elementBold() const { return m_eBold; }
    inline void setElementBold(bool a) { m_eBold = a; };

    inline bool elementUnderline() const { return m_eUnderline; }
    inline void setElementUnderline(bool a) { m_eUnderline = a; };

    inline bool elementStrikeOut() const { return m_eStrikeOut; }
    inline void setElementStrikeOut(bool a) { m_eStrikeOut = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    QString m_eFamily;
    int m_ePointSize;
    int m_eWeight;
    bool m_eItalic;
    bool m_eBold;
    bool m_eUnderline;
    bool m_eStrikeOut;
};

class DomPoint
{
    DomPoint(const DomPoint &other);
    void operator = (const DomPoint &other);

public:
    DomPoint();
    ~DomPoint();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline int elementX() const { return m_eX; }
    inline void setElementX(int a) { m_eX = a; };

    inline int elementY() const { return m_eY; }
    inline void setElementY(int a) { m_eY = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    int m_eX;
    int m_eY;
};

class DomRect
{
    DomRect(const DomRect &other);
    void operator = (const DomRect &other);

public:
    DomRect();
    ~DomRect();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline int elementX() const { return m_eX; }
    inline void setElementX(int a) { m_eX = a; };

    inline int elementY() const { return m_eY; }
    inline void setElementY(int a) { m_eY = a; };

    inline int elementWidth() const { return m_eWidth; }
    inline void setElementWidth(int a) { m_eWidth = a; };

    inline int elementHeight() const { return m_eHeight; }
    inline void setElementHeight(int a) { m_eHeight = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    int m_eX;
    int m_eY;
    int m_eWidth;
    int m_eHeight;
};

class DomSizePolicy
{
    DomSizePolicy(const DomSizePolicy &other);
    void operator = (const DomSizePolicy &other);

public:
    DomSizePolicy();
    ~DomSizePolicy();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline int elementHSizeType() const { return m_eHSizeType; }
    inline void setElementHSizeType(int a) { m_eHSizeType = a; };

    inline int elementVSizeType() const { return m_eVSizeType; }
    inline void setElementVSizeType(int a) { m_eVSizeType = a; };

    inline int elementHorStretch() const { return m_eHorStretch; }
    inline void setElementHorStretch(int a) { m_eHorStretch = a; };

    inline int elementVerStretch() const { return m_eVerStretch; }
    inline void setElementVerStretch(int a) { m_eVerStretch = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    int m_eHSizeType;
    int m_eVSizeType;
    int m_eHorStretch;
    int m_eVerStretch;
};

class DomSize
{
    DomSize(const DomSize &other);
    void operator = (const DomSize &other);

public:
    DomSize();
    ~DomSize();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline int elementWidth() const { return m_eWidth; }
    inline void setElementWidth(int a) { m_eWidth = a; };

    inline int elementHeight() const { return m_eHeight; }
    inline void setElementHeight(int a) { m_eHeight = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    int m_eWidth;
    int m_eHeight;
};

class DomDate
{
    DomDate(const DomDate &other);
    void operator = (const DomDate &other);

public:
    DomDate();
    ~DomDate();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline int elementYear() const { return m_eYear; }
    inline void setElementYear(int a) { m_eYear = a; };

    inline int elementMonth() const { return m_eMonth; }
    inline void setElementMonth(int a) { m_eMonth = a; };

    inline int elementDay() const { return m_eDay; }
    inline void setElementDay(int a) { m_eDay = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    int m_eYear;
    int m_eMonth;
    int m_eDay;
};

class DomTime
{
    DomTime(const DomTime &other);
    void operator = (const DomTime &other);

public:
    DomTime();
    ~DomTime();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline int elementHour() const { return m_eHour; }
    inline void setElementHour(int a) { m_eHour = a; };

    inline int elementMinute() const { return m_eMinute; }
    inline void setElementMinute(int a) { m_eMinute = a; };

    inline int elementSecond() const { return m_eSecond; }
    inline void setElementSecond(int a) { m_eSecond = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    int m_eHour;
    int m_eMinute;
    int m_eSecond;
};

class DomDateTime
{
    DomDateTime(const DomDateTime &other);
    void operator = (const DomDateTime &other);

public:
    DomDateTime();
    ~DomDateTime();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline int elementHour() const { return m_eHour; }
    inline void setElementHour(int a) { m_eHour = a; };

    inline int elementMinute() const { return m_eMinute; }
    inline void setElementMinute(int a) { m_eMinute = a; };

    inline int elementSecond() const { return m_eSecond; }
    inline void setElementSecond(int a) { m_eSecond = a; };

    inline int elementYear() const { return m_eYear; }
    inline void setElementYear(int a) { m_eYear = a; };

    inline int elementMonth() const { return m_eMonth; }
    inline void setElementMonth(int a) { m_eMonth = a; };

    inline int elementDay() const { return m_eDay; }
    inline void setElementDay(int a) { m_eDay = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    int m_eHour;
    int m_eMinute;
    int m_eSecond;
    int m_eYear;
    int m_eMonth;
    int m_eDay;
};

class DomStringList
{
    DomStringList(const DomStringList &other);
    void operator = (const DomStringList &other);

public:
    DomStringList();
    ~DomStringList();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QStringList elementString() const { return m_eString; }
    inline void setElementString(const QStringList & a) { m_eString = a; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes

    // elements
    QStringList m_eString;
};

class DomString
{
    DomString(const DomString &other);
    void operator = (const DomString &other);

public:
    DomString();
    ~DomString();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    inline QString attributeNotr() const { return m_aNotr; }
    inline bool hasAttributeNotr() const { return m_hasNotr; }
    inline void setAttributeNotr(const QString & a) { m_aNotr = a; m_hasNotr = true; }
    inline void clearAttributeNotr() { m_hasNotr = false; }

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);

    // attributes
    QString m_aNotr;
    bool m_hasNotr;

    // elements
};

class DomProperty
{
    DomProperty(const DomProperty &other);
    void operator = (const DomProperty &other);

public:
    DomProperty();
    ~DomProperty();

    void read(const QDomElement &node);
    QDomElement write(QDomDocument &doc, const QString &tagName = QString::null);

    enum Kind {
        Unknown = 0,
        Bool,
        Color,
        Cstring,
        Cursor,
        Enum,
        Font,
        IconSet,
        Pixmap,
        Palette,
        Point,
        Rect,
        Set,
        SizePolicy,
        Size,
        String,
        StringList,
        Number,
        Date,
        Time,
        DateTime
    };

    inline Kind kind() const { return m_kind; }

    inline QString attributeName() const { return m_aName; }
    inline bool hasAttributeName() const { return m_hasName; }
    inline void setAttributeName(const QString & a) { m_aName = a; m_hasName = true; }
    inline void clearAttributeName() { m_hasName = false; }

    inline int attributeStdset() const { return m_aStdset; }
    inline bool hasAttributeStdset() const { return m_hasStdset; }
    inline void setAttributeStdset(int a) { m_aStdset = a; m_hasStdset = true; }
    inline void clearAttributeStdset() { m_hasStdset = false; }

    inline QString elementBool() const { return m_eBool; }
    inline void setElementBool(const QString & a) { reset(false); m_eBool = a; m_kind = Bool; };

    inline DomColor * elementColor() const { return m_eColor; }
    inline void setElementColor(DomColor * a) { reset(false); m_eColor = a; m_kind = Color; };

    inline QString elementCstring() const { return m_eCstring; }
    inline void setElementCstring(const QString & a) { reset(false); m_eCstring = a; m_kind = Cstring; };

    inline int elementCursor() const { return m_eCursor; }
    inline void setElementCursor(int a) { reset(false); m_eCursor = a; m_kind = Cursor; };

    inline QString elementEnum() const { return m_eEnum; }
    inline void setElementEnum(const QString & a) { reset(false); m_eEnum = a; m_kind = Enum; };

    inline DomFont * elementFont() const { return m_eFont; }
    inline void setElementFont(DomFont * a) { reset(false); m_eFont = a; m_kind = Font; };

    inline QString elementIconSet() const { return m_eIconSet; }
    inline void setElementIconSet(const QString & a) { reset(false); m_eIconSet = a; m_kind = IconSet; };

    inline QString elementPixmap() const { return m_ePixmap; }
    inline void setElementPixmap(const QString & a) { reset(false); m_ePixmap = a; m_kind = Pixmap; };

    inline DomPalette * elementPalette() const { return m_ePalette; }
    inline void setElementPalette(DomPalette * a) { reset(false); m_ePalette = a; m_kind = Palette; };

    inline DomPoint * elementPoint() const { return m_ePoint; }
    inline void setElementPoint(DomPoint * a) { reset(false); m_ePoint = a; m_kind = Point; };

    inline DomRect * elementRect() const { return m_eRect; }
    inline void setElementRect(DomRect * a) { reset(false); m_eRect = a; m_kind = Rect; };

    inline QString elementSet() const { return m_eSet; }
    inline void setElementSet(const QString & a) { reset(false); m_eSet = a; m_kind = Set; };

    inline DomSizePolicy * elementSizePolicy() const { return m_eSizePolicy; }
    inline void setElementSizePolicy(DomSizePolicy * a) { reset(false); m_eSizePolicy = a; m_kind = SizePolicy; };

    inline DomSize * elementSize() const { return m_eSize; }
    inline void setElementSize(DomSize * a) { reset(false); m_eSize = a; m_kind = Size; };

    inline DomString * elementString() const { return m_eString; }
    inline void setElementString(DomString * a) { reset(false); m_eString = a; m_kind = String; };

    inline DomStringList * elementStringList() const { return m_eStringList; }
    inline void setElementStringList(DomStringList * a) { reset(false); m_eStringList = a; m_kind = StringList; };

    inline int elementNumber() const { return m_eNumber; }
    inline void setElementNumber(int a) { reset(false); m_eNumber = a; m_kind = Number; };

    inline DomDate * elementDate() const { return m_eDate; }
    inline void setElementDate(DomDate * a) { reset(false); m_eDate = a; m_kind = Date; };

    inline DomTime * elementTime() const { return m_eTime; }
    inline void setElementTime(DomTime * a) { reset(false); m_eTime = a; m_kind = Time; };

    inline DomDateTime * elementDateTime() const { return m_eDateTime; }
    inline void setElementDateTime(DomDateTime * a) { reset(false); m_eDateTime = a; m_kind = DateTime; };

    inline QString text() const { return m_text; }
    inline void setText(const QString &text) { m_text = text; }
private:
    QString m_text;

    inline void reset(bool full=true);
    Kind m_kind;

    // attributes
    QString m_aName;
    bool m_hasName;
    int m_aStdset;
    bool m_hasStdset;

    // elements
    QString m_eBool;
    DomColor * m_eColor;
    QString m_eCstring;
    int m_eCursor;
    QString m_eEnum;
    DomFont * m_eFont;
    QString m_eIconSet;
    QString m_ePixmap;
    DomPalette * m_ePalette;
    DomPoint * m_ePoint;
    DomRect * m_eRect;
    QString m_eSet;
    DomSizePolicy * m_eSizePolicy;
    DomSize * m_eSize;
    DomString * m_eString;
    DomStringList * m_eStringList;
    int m_eNumber;
    DomDate * m_eDate;
    DomTime * m_eTime;
    DomDateTime * m_eDateTime;
};



inline DomUI::DomUI()
{
    m_hasVersion = false;
    m_hasStdSetDef = false;
    m_eWidget = 0;
    m_eLayoutDefault = 0;
    m_eLayoutFunction = 0;
    m_eCustomWidgets = 0;
    m_eTabStops = 0;
    m_eImages = 0;
    m_eIncludes = 0;
}

inline DomIncludes::DomIncludes()
{
}

inline DomInclude::DomInclude()
{
    m_hasLocation = false;
    m_hasImpldecl = false;
}

inline DomActionGroup::DomActionGroup()
{
    m_hasName = false;
}

inline DomAction::DomAction()
{
    m_hasName = false;
    m_hasMenu = false;
}

inline DomActionRef::DomActionRef()
{
    m_hasName = false;
}

inline DomImages::DomImages()
{
}

inline DomImage::DomImage()
{
    m_hasName = false;
    m_eData = 0;
}

inline DomImageData::DomImageData()
{
    m_hasFormat = false;
    m_hasLength = false;
}

inline DomCustomWidgets::DomCustomWidgets()
{
}

inline DomHeader::DomHeader()
{
    m_hasLocation = false;
}

inline DomCustomWidget::DomCustomWidget()
{
    m_eHeader = 0;
    m_eSizeHint = 0;
    m_eContainer = 0;
    m_eSizePolicy = 0;
    m_eProperties = 0;
}

inline DomProperties::DomProperties()
{
}

inline DomPropertyData::DomPropertyData()
{
    m_hasType = false;
}

inline DomSizePolicyData::DomSizePolicyData()
{
    m_eHorData = 0;
    m_eVerData = 0;
}

inline DomLayoutDefault::DomLayoutDefault()
{
    m_hasSpacing = false;
    m_hasMargin = false;
}

inline DomLayoutFunction::DomLayoutFunction()
{
    m_hasSpacing = false;
    m_hasMargin = false;
}

inline DomTabStops::DomTabStops()
{
}

inline DomLayout::DomLayout()
{
    m_hasClass = false;
}

inline DomLayoutItem::DomLayoutItem()
{
    m_kind = Unknown;

    m_hasRow = false;
    m_hasColumn = false;
    m_hasRowSpan = false;
    m_hasColSpan = false;
    m_eWidget = 0;
    m_eLayout = 0;
    m_eSpacer = 0;
}

inline DomRow::DomRow()
{
}

inline DomColumn::DomColumn()
{
}

inline DomItem::DomItem()
{
}

inline DomWidget::DomWidget()
{
    m_hasClass = false;
    m_hasName = false;
}

inline DomSpacer::DomSpacer()
{
    m_hasName = false;
}

inline DomColor::DomColor()
{
    m_eRed = 0;
    m_eGreen = 0;
    m_eBlue = 0;
}

inline DomColorGroup::DomColorGroup()
{
}

inline DomPalette::DomPalette()
{
    m_eActive = 0;
    m_eInactive = 0;
    m_eDisabled = 0;
}

inline DomFont::DomFont()
{
    m_ePointSize = 0;
    m_eWeight = 0;
    m_eItalic = false;
    m_eBold = false;
    m_eUnderline = false;
    m_eStrikeOut = false;
}

inline DomPoint::DomPoint()
{
    m_eX = 0;
    m_eY = 0;
}

inline DomRect::DomRect()
{
    m_eX = 0;
    m_eY = 0;
    m_eWidth = 0;
    m_eHeight = 0;
}

inline DomSizePolicy::DomSizePolicy()
{
    m_eHSizeType = 0;
    m_eVSizeType = 0;
    m_eHorStretch = 0;
    m_eVerStretch = 0;
}

inline DomSize::DomSize()
{
    m_eWidth = 0;
    m_eHeight = 0;
}

inline DomDate::DomDate()
{
    m_eYear = 0;
    m_eMonth = 0;
    m_eDay = 0;
}

inline DomTime::DomTime()
{
    m_eHour = 0;
    m_eMinute = 0;
    m_eSecond = 0;
}

inline DomDateTime::DomDateTime()
{
    m_eHour = 0;
    m_eMinute = 0;
    m_eSecond = 0;
    m_eYear = 0;
    m_eMonth = 0;
    m_eDay = 0;
}

inline DomStringList::DomStringList()
{
}

inline DomString::DomString()
{
    m_hasNotr = false;
}

inline DomProperty::DomProperty()
{
    m_kind = Unknown;

    m_hasName = false;
    m_hasStdset = false;
    m_eColor = 0;
    m_eCursor = 0;
    m_eFont = 0;
    m_ePalette = 0;
    m_ePoint = 0;
    m_eRect = 0;
    m_eSizePolicy = 0;
    m_eSize = 0;
    m_eString = 0;
    m_eStringList = 0;
    m_eNumber = 0;
    m_eDate = 0;
    m_eTime = 0;
    m_eDateTime = 0;
}



inline DomUI::~DomUI()
{
    reset();
}

inline DomIncludes::~DomIncludes()
{
    reset();
}

inline DomInclude::~DomInclude()
{
    reset();
}

inline DomActionGroup::~DomActionGroup()
{
    reset();
}

inline DomAction::~DomAction()
{
    reset();
}

inline DomActionRef::~DomActionRef()
{
    reset();
}

inline DomImages::~DomImages()
{
    reset();
}

inline DomImage::~DomImage()
{
    reset();
}

inline DomImageData::~DomImageData()
{
    reset();
}

inline DomCustomWidgets::~DomCustomWidgets()
{
    reset();
}

inline DomHeader::~DomHeader()
{
    reset();
}

inline DomCustomWidget::~DomCustomWidget()
{
    reset();
}

inline DomProperties::~DomProperties()
{
    reset();
}

inline DomPropertyData::~DomPropertyData()
{
    reset();
}

inline DomSizePolicyData::~DomSizePolicyData()
{
    reset();
}

inline DomLayoutDefault::~DomLayoutDefault()
{
    reset();
}

inline DomLayoutFunction::~DomLayoutFunction()
{
    reset();
}

inline DomTabStops::~DomTabStops()
{
    reset();
}

inline DomLayout::~DomLayout()
{
    reset();
}

inline DomLayoutItem::~DomLayoutItem()
{
    reset();
}

inline DomRow::~DomRow()
{
    reset();
}

inline DomColumn::~DomColumn()
{
    reset();
}

inline DomItem::~DomItem()
{
    reset();
}

inline DomWidget::~DomWidget()
{
    reset();
}

inline DomSpacer::~DomSpacer()
{
    reset();
}

inline DomColor::~DomColor()
{
    reset();
}

inline DomColorGroup::~DomColorGroup()
{
    reset();
}

inline DomPalette::~DomPalette()
{
    reset();
}

inline DomFont::~DomFont()
{
    reset();
}

inline DomPoint::~DomPoint()
{
    reset();
}

inline DomRect::~DomRect()
{
    reset();
}

inline DomSizePolicy::~DomSizePolicy()
{
    reset();
}

inline DomSize::~DomSize()
{
    reset();
}

inline DomDate::~DomDate()
{
    reset();
}

inline DomTime::~DomTime()
{
    reset();
}

inline DomDateTime::~DomDateTime()
{
    reset();
}

inline DomStringList::~DomStringList()
{
    reset();
}

inline DomString::~DomString()
{
    reset();
}

inline DomProperty::~DomProperty()
{
    reset();
}



inline void DomUI::read(const QDomElement &node)
{
    // attributes
    if (node.hasAttribute(QString::fromLatin1("version").toLower())) setAttributeVersion(node.attribute(QString::fromLatin1("version").toLower()));
    if (node.hasAttribute(QString::fromLatin1("stdSetDef").toLower())) setAttributeStdSetDef(node.attribute(QString::fromLatin1("stdSetDef").toLower()).toInt());

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("author")) { m_eAuthor = e.firstChild().toText().data(); }
        else if (tag == QLatin1String("comment")) { m_eComment = e.firstChild().toText().data(); }
        else if (tag == QLatin1String("exportmacro")) { m_eExportMacro = e.firstChild().toText().data(); }
        else if (tag == QLatin1String("class")) { m_eClass = e.firstChild().toText().data(); }
        else if (tag == QLatin1String("widget")) { DomWidget* v = new DomWidget(); v->read(e); m_eWidget = v; }
        else if (tag == QLatin1String("layoutdefault")) { DomLayoutDefault* v = new DomLayoutDefault(); v->read(e); m_eLayoutDefault = v; }
        else if (tag == QLatin1String("layoutfunction")) { DomLayoutFunction* v = new DomLayoutFunction(); v->read(e); m_eLayoutFunction = v; }
        else if (tag == QLatin1String("pixmapfunction")) { m_ePixmapFunction = e.firstChild().toText().data(); }
        else if (tag == QLatin1String("customwidgets")) { DomCustomWidgets* v = new DomCustomWidgets(); v->read(e); m_eCustomWidgets = v; }
        else if (tag == QLatin1String("tabstops")) { DomTabStops* v = new DomTabStops(); v->read(e); m_eTabStops = v; }
        else if (tag == QLatin1String("images")) { DomImages* v = new DomImages(); v->read(e); m_eImages = v; }
        else if (tag == QLatin1String("includes")) { DomIncludes* v = new DomIncludes(); v->read(e); m_eIncludes = v; }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomIncludes::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("include")) { DomInclude* v = new DomInclude(); v->read(e); m_eInclude.append(v); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomInclude::read(const QDomElement &node)
{
    // attributes
    if (node.hasAttribute(QString::fromLatin1("location").toLower())) setAttributeLocation(node.attribute(QString::fromLatin1("location").toLower()));
    if (node.hasAttribute(QString::fromLatin1("impldecl").toLower())) setAttributeImpldecl(node.attribute(QString::fromLatin1("impldecl").toLower()));

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomActionGroup::read(const QDomElement &node)
{
    // attributes
    if (node.hasAttribute(QString::fromLatin1("name").toLower())) setAttributeName(node.attribute(QString::fromLatin1("name").toLower()));

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("action")) { DomAction* v = new DomAction(); v->read(e); m_eAction.append(v); }
        else if (tag == QLatin1String("actiongroup")) { DomActionGroup* v = new DomActionGroup(); v->read(e); m_eActionGroup.append(v); }
        else if (tag == QLatin1String("property")) { DomProperty* v = new DomProperty(); v->read(e); m_eProperty.append(v); }
        else if (tag == QLatin1String("attribute")) { DomProperty* v = new DomProperty(); v->read(e); m_eAttribute.append(v); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomAction::read(const QDomElement &node)
{
    // attributes
    if (node.hasAttribute(QString::fromLatin1("name").toLower())) setAttributeName(node.attribute(QString::fromLatin1("name").toLower()));
    if (node.hasAttribute(QString::fromLatin1("menu").toLower())) setAttributeMenu(node.attribute(QString::fromLatin1("menu").toLower()));

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("property")) { DomProperty* v = new DomProperty(); v->read(e); m_eProperty.append(v); }
        else if (tag == QLatin1String("attribute")) { DomProperty* v = new DomProperty(); v->read(e); m_eAttribute.append(v); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomActionRef::read(const QDomElement &node)
{
    // attributes
    if (node.hasAttribute(QString::fromLatin1("name").toLower())) setAttributeName(node.attribute(QString::fromLatin1("name").toLower()));

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomImages::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("image")) { DomImage* v = new DomImage(); v->read(e); m_eImage.append(v); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomImage::read(const QDomElement &node)
{
    // attributes
    if (node.hasAttribute(QString::fromLatin1("name").toLower())) setAttributeName(node.attribute(QString::fromLatin1("name").toLower()));

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("data")) { DomImageData* v = new DomImageData(); v->read(e); m_eData = v; }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomImageData::read(const QDomElement &node)
{
    // attributes
    if (node.hasAttribute(QString::fromLatin1("format").toLower())) setAttributeFormat(node.attribute(QString::fromLatin1("format").toLower()));
    if (node.hasAttribute(QString::fromLatin1("length").toLower())) setAttributeLength(node.attribute(QString::fromLatin1("length").toLower()).toInt());

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomCustomWidgets::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("customwidget")) { DomCustomWidget* v = new DomCustomWidget(); v->read(e); m_eCustomWidget.append(v); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomHeader::read(const QDomElement &node)
{
    // attributes
    if (node.hasAttribute(QString::fromLatin1("location").toLower())) setAttributeLocation(node.attribute(QString::fromLatin1("location").toLower()));

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomCustomWidget::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("class")) { m_eClass = e.firstChild().toText().data(); }
        else if (tag == QLatin1String("extends")) { m_eExtends = e.firstChild().toText().data(); }
        else if (tag == QLatin1String("header")) { DomHeader* v = new DomHeader(); v->read(e); m_eHeader = v; }
        else if (tag == QLatin1String("sizehint")) { DomSize* v = new DomSize(); v->read(e); m_eSizeHint = v; }
        else if (tag == QLatin1String("container")) { m_eContainer = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("sizepolicy")) { DomSizePolicyData* v = new DomSizePolicyData(); v->read(e); m_eSizePolicy = v; }
        else if (tag == QLatin1String("pixmap")) { m_ePixmap = e.firstChild().toText().data(); }
        else if (tag == QLatin1String("properties")) { DomProperties* v = new DomProperties(); v->read(e); m_eProperties = v; }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomProperties::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("property")) { DomPropertyData* v = new DomPropertyData(); v->read(e); m_eProperty.append(v); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomPropertyData::read(const QDomElement &node)
{
    // attributes
    if (node.hasAttribute(QString::fromLatin1("type").toLower())) setAttributeType(node.attribute(QString::fromLatin1("type").toLower()));

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomSizePolicyData::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("hordata")) { m_eHorData = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("verdata")) { m_eVerData = e.firstChild().toText().data().toInt(); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomLayoutDefault::read(const QDomElement &node)
{
    // attributes
    if (node.hasAttribute(QString::fromLatin1("spacing").toLower())) setAttributeSpacing(node.attribute(QString::fromLatin1("spacing").toLower()).toInt());
    if (node.hasAttribute(QString::fromLatin1("margin").toLower())) setAttributeMargin(node.attribute(QString::fromLatin1("margin").toLower()).toInt());

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomLayoutFunction::read(const QDomElement &node)
{
    // attributes
    if (node.hasAttribute(QString::fromLatin1("spacing").toLower())) setAttributeSpacing(node.attribute(QString::fromLatin1("spacing").toLower()));
    if (node.hasAttribute(QString::fromLatin1("margin").toLower())) setAttributeMargin(node.attribute(QString::fromLatin1("margin").toLower()));

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomTabStops::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("tabstop")) { m_eTabStop.append(e.firstChild().toText().data()); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomLayout::read(const QDomElement &node)
{
    // attributes
    if (node.hasAttribute(QString::fromLatin1("class").toLower())) setAttributeClass(node.attribute(QString::fromLatin1("class").toLower()));

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("property")) { DomProperty* v = new DomProperty(); v->read(e); m_eProperty.append(v); }
        else if (tag == QLatin1String("attribute")) { DomProperty* v = new DomProperty(); v->read(e); m_eAttribute.append(v); }
        else if (tag == QLatin1String("item")) { DomLayoutItem* v = new DomLayoutItem(); v->read(e); m_eItem.append(v); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomLayoutItem::read(const QDomElement &node)
{
    m_kind = Unknown;

    // attributes
    if (node.hasAttribute(QString::fromLatin1("row").toLower())) setAttributeRow(node.attribute(QString::fromLatin1("row").toLower()).toInt());
    if (node.hasAttribute(QString::fromLatin1("column").toLower())) setAttributeColumn(node.attribute(QString::fromLatin1("column").toLower()).toInt());
    if (node.hasAttribute(QString::fromLatin1("rowSpan").toLower())) setAttributeRowSpan(node.attribute(QString::fromLatin1("rowSpan").toLower()).toInt());
    if (node.hasAttribute(QString::fromLatin1("colSpan").toLower())) setAttributeColSpan(node.attribute(QString::fromLatin1("colSpan").toLower()).toInt());

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("widget")) { DomWidget* v = new DomWidget(); v->read(e); m_eWidget = v; m_kind = Widget; }
        else if (tag == QLatin1String("layout")) { DomLayout* v = new DomLayout(); v->read(e); m_eLayout = v; m_kind = Layout; }
        else if (tag == QLatin1String("spacer")) { DomSpacer* v = new DomSpacer(); v->read(e); m_eSpacer = v; m_kind = Spacer; }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomRow::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("property")) { DomProperty* v = new DomProperty(); v->read(e); m_eProperty.append(v); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomColumn::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("property")) { DomProperty* v = new DomProperty(); v->read(e); m_eProperty.append(v); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomItem::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("property")) { DomProperty* v = new DomProperty(); v->read(e); m_eProperty.append(v); }
        else if (tag == QLatin1String("item")) { DomItem* v = new DomItem(); v->read(e); m_eItem.append(v); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomWidget::read(const QDomElement &node)
{
    // attributes
    if (node.hasAttribute(QString::fromLatin1("class").toLower())) setAttributeClass(node.attribute(QString::fromLatin1("class").toLower()));
    if (node.hasAttribute(QString::fromLatin1("name").toLower())) setAttributeName(node.attribute(QString::fromLatin1("name").toLower()));

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("class")) { m_eClass.append(e.firstChild().toText().data()); }
        else if (tag == QLatin1String("property")) { DomProperty* v = new DomProperty(); v->read(e); m_eProperty.append(v); }
        else if (tag == QLatin1String("attribute")) { DomProperty* v = new DomProperty(); v->read(e); m_eAttribute.append(v); }
        else if (tag == QLatin1String("row")) { DomRow* v = new DomRow(); v->read(e); m_eRow.append(v); }
        else if (tag == QLatin1String("column")) { DomColumn* v = new DomColumn(); v->read(e); m_eColumn.append(v); }
        else if (tag == QLatin1String("item")) { DomItem* v = new DomItem(); v->read(e); m_eItem.append(v); }
        else if (tag == QLatin1String("layout")) { DomLayout* v = new DomLayout(); v->read(e); m_eLayout.append(v); }
        else if (tag == QLatin1String("widget")) { DomWidget* v = new DomWidget(); v->read(e); m_eWidget.append(v); }
        else if (tag == QLatin1String("action")) { DomAction* v = new DomAction(); v->read(e); m_eAction.append(v); }
        else if (tag == QLatin1String("actiongroup")) { DomActionGroup* v = new DomActionGroup(); v->read(e); m_eActionGroup.append(v); }
        else if (tag == QLatin1String("addaction")) { DomActionRef* v = new DomActionRef(); v->read(e); m_eAddAction.append(v); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomSpacer::read(const QDomElement &node)
{
    // attributes
    if (node.hasAttribute(QString::fromLatin1("name").toLower())) setAttributeName(node.attribute(QString::fromLatin1("name").toLower()));

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("property")) { DomProperty* v = new DomProperty(); v->read(e); m_eProperty.append(v); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomColor::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("red")) { m_eRed = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("green")) { m_eGreen = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("blue")) { m_eBlue = e.firstChild().toText().data().toInt(); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomColorGroup::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("color")) { DomColor* v = new DomColor(); v->read(e); m_eColor.append(v); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomPalette::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("active")) { DomColorGroup* v = new DomColorGroup(); v->read(e); m_eActive = v; }
        else if (tag == QLatin1String("inactive")) { DomColorGroup* v = new DomColorGroup(); v->read(e); m_eInactive = v; }
        else if (tag == QLatin1String("disabled")) { DomColorGroup* v = new DomColorGroup(); v->read(e); m_eDisabled = v; }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomFont::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("family")) { m_eFamily = e.firstChild().toText().data(); }
        else if (tag == QLatin1String("pointsize")) { m_ePointSize = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("weight")) { m_eWeight = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("italic")) { m_eItalic = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("bold")) { m_eBold = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("underline")) { m_eUnderline = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("strikeout")) { m_eStrikeOut = e.firstChild().toText().data().toInt(); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomPoint::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("x")) { m_eX = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("y")) { m_eY = e.firstChild().toText().data().toInt(); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomRect::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("x")) { m_eX = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("y")) { m_eY = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("width")) { m_eWidth = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("height")) { m_eHeight = e.firstChild().toText().data().toInt(); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomSizePolicy::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("hsizetype")) { m_eHSizeType = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("vsizetype")) { m_eVSizeType = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("horstretch")) { m_eHorStretch = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("verstretch")) { m_eVerStretch = e.firstChild().toText().data().toInt(); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomSize::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("width")) { m_eWidth = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("height")) { m_eHeight = e.firstChild().toText().data().toInt(); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomDate::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("year")) { m_eYear = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("month")) { m_eMonth = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("day")) { m_eDay = e.firstChild().toText().data().toInt(); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomTime::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("hour")) { m_eHour = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("minute")) { m_eMinute = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("second")) { m_eSecond = e.firstChild().toText().data().toInt(); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomDateTime::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("hour")) { m_eHour = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("minute")) { m_eMinute = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("second")) { m_eSecond = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("year")) { m_eYear = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("month")) { m_eMonth = e.firstChild().toText().data().toInt(); }
        else if (tag == QLatin1String("day")) { m_eDay = e.firstChild().toText().data().toInt(); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomStringList::read(const QDomElement &node)
{
    // attributes

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("string")) { m_eString.append(e.firstChild().toText().data()); }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomString::read(const QDomElement &node)
{
    // attributes
    if (node.hasAttribute(QString::fromLatin1("notr").toLower())) setAttributeNotr(node.attribute(QString::fromLatin1("notr").toLower()));

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}

inline void DomProperty::read(const QDomElement &node)
{
    m_kind = Unknown;

    // attributes
    if (node.hasAttribute(QString::fromLatin1("name").toLower())) setAttributeName(node.attribute(QString::fromLatin1("name").toLower()));
    if (node.hasAttribute(QString::fromLatin1("stdset").toLower())) setAttributeStdset(node.attribute(QString::fromLatin1("stdset").toLower()).toInt());

    // elements
    QDomElement e = node.firstChild().toElement();
    while (!e.isNull()) {
        QString tag = e.tagName().toLower();
        if (tag == QLatin1String("bool")) { m_eBool = e.firstChild().toText().data(); m_kind = Bool; }
        else if (tag == QLatin1String("color")) { DomColor* v = new DomColor(); v->read(e); m_eColor = v; m_kind = Color; }
        else if (tag == QLatin1String("cstring")) { m_eCstring = e.firstChild().toText().data(); m_kind = Cstring; }
        else if (tag == QLatin1String("cursor")) { m_eCursor = e.firstChild().toText().data().toInt(); m_kind = Cursor; }
        else if (tag == QLatin1String("enum")) { m_eEnum = e.firstChild().toText().data(); m_kind = Enum; }
        else if (tag == QLatin1String("font")) { DomFont* v = new DomFont(); v->read(e); m_eFont = v; m_kind = Font; }
        else if (tag == QLatin1String("iconset")) { m_eIconSet = e.firstChild().toText().data(); m_kind = IconSet; }
        else if (tag == QLatin1String("pixmap")) { m_ePixmap = e.firstChild().toText().data(); m_kind = Pixmap; }
        else if (tag == QLatin1String("palette")) { DomPalette* v = new DomPalette(); v->read(e); m_ePalette = v; m_kind = Palette; }
        else if (tag == QLatin1String("point")) { DomPoint* v = new DomPoint(); v->read(e); m_ePoint = v; m_kind = Point; }
        else if (tag == QLatin1String("rect")) { DomRect* v = new DomRect(); v->read(e); m_eRect = v; m_kind = Rect; }
        else if (tag == QLatin1String("set")) { m_eSet = e.firstChild().toText().data(); m_kind = Set; }
        else if (tag == QLatin1String("sizepolicy")) { DomSizePolicy* v = new DomSizePolicy(); v->read(e); m_eSizePolicy = v; m_kind = SizePolicy; }
        else if (tag == QLatin1String("size")) { DomSize* v = new DomSize(); v->read(e); m_eSize = v; m_kind = Size; }
        else if (tag == QLatin1String("string")) { DomString* v = new DomString(); v->read(e); m_eString = v; m_kind = String; }
        else if (tag == QLatin1String("stringlist")) { DomStringList* v = new DomStringList(); v->read(e); m_eStringList = v; m_kind = StringList; }
        else if (tag == QLatin1String("number")) { m_eNumber = e.firstChild().toText().data().toInt(); m_kind = Number; }
        else if (tag == QLatin1String("date")) { DomDate* v = new DomDate(); v->read(e); m_eDate = v; m_kind = Date; }
        else if (tag == QLatin1String("time")) { DomTime* v = new DomTime(); v->read(e); m_eTime = v; m_kind = Time; }
        else if (tag == QLatin1String("datetime")) { DomDateTime* v = new DomDateTime(); v->read(e); m_eDateTime = v; m_kind = DateTime; }

        e = e.nextSibling().toElement();
    }
    m_text = node.firstChild().toText().data();
}



inline QDomElement DomUI::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("UI").toLower());
    if (m_hasVersion)
        node.setAttribute(QString::fromLatin1("version").toLower(), m_aVersion);

    if (m_hasStdSetDef)
        node.setAttribute(QString::fromLatin1("stdSetDef").toLower(), m_aStdSetDef);


    QDomElement child;
    QDomText t;

    if (m_eAuthor.size()) {
        child = doc.createElement(QString::fromLatin1("author").toLower());
        t = doc.createTextNode(m_eAuthor);
        child.appendChild(t);
        node.appendChild(child);
    }

    if (m_eComment.size()) {
        child = doc.createElement(QString::fromLatin1("comment").toLower());
        t = doc.createTextNode(m_eComment);
        child.appendChild(t);
        node.appendChild(child);
    }

    if (m_eExportMacro.size()) {
        child = doc.createElement(QString::fromLatin1("exportMacro").toLower());
        t = doc.createTextNode(m_eExportMacro);
        child.appendChild(t);
        node.appendChild(child);
    }

    if (m_eClass.size()) {
        child = doc.createElement(QString::fromLatin1("class").toLower());
        t = doc.createTextNode(m_eClass);
        child.appendChild(t);
        node.appendChild(child);
    }

    if (m_eWidget)
        node.appendChild(m_eWidget->write(doc, QString::fromLatin1("widget").toLower()));

    if (m_eLayoutDefault)
        node.appendChild(m_eLayoutDefault->write(doc, QString::fromLatin1("layoutDefault").toLower()));

    if (m_eLayoutFunction)
        node.appendChild(m_eLayoutFunction->write(doc, QString::fromLatin1("layoutFunction").toLower()));

    if (m_ePixmapFunction.size()) {
        child = doc.createElement(QString::fromLatin1("pixmapFunction").toLower());
        t = doc.createTextNode(m_ePixmapFunction);
        child.appendChild(t);
        node.appendChild(child);
    }

    if (m_eCustomWidgets)
        node.appendChild(m_eCustomWidgets->write(doc, QString::fromLatin1("customWidgets").toLower()));

    if (m_eTabStops)
        node.appendChild(m_eTabStops->write(doc, QString::fromLatin1("tabStops").toLower()));

    if (m_eImages)
        node.appendChild(m_eImages->write(doc, QString::fromLatin1("images").toLower()));

    if (m_eIncludes)
        node.appendChild(m_eIncludes->write(doc, QString::fromLatin1("includes").toLower()));

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomIncludes::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Includes").toLower());

    QDomElement child;
    QDomText t;

    for (int i=0; i<m_eInclude.size(); ++i) {
        node.appendChild(m_eInclude.at(i)->write(doc, QString::fromLatin1("include").toLower()));
    }

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomInclude::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Include").toLower());
    if (m_hasLocation)
        node.setAttribute(QString::fromLatin1("location").toLower(), m_aLocation);

    if (m_hasImpldecl)
        node.setAttribute(QString::fromLatin1("impldecl").toLower(), m_aImpldecl);


    QDomElement child;
    QDomText t;

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomActionGroup::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("ActionGroup").toLower());
    if (m_hasName)
        node.setAttribute(QString::fromLatin1("name").toLower(), m_aName);


    QDomElement child;
    QDomText t;

    for (int i=0; i<m_eAction.size(); ++i) {
        node.appendChild(m_eAction.at(i)->write(doc, QString::fromLatin1("action").toLower()));
    }

    for (int i=0; i<m_eActionGroup.size(); ++i) {
        node.appendChild(m_eActionGroup.at(i)->write(doc, QString::fromLatin1("actionGroup").toLower()));
    }

    for (int i=0; i<m_eProperty.size(); ++i) {
        node.appendChild(m_eProperty.at(i)->write(doc, QString::fromLatin1("property").toLower()));
    }

    for (int i=0; i<m_eAttribute.size(); ++i) {
        node.appendChild(m_eAttribute.at(i)->write(doc, QString::fromLatin1("attribute").toLower()));
    }

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomAction::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Action").toLower());
    if (m_hasName)
        node.setAttribute(QString::fromLatin1("name").toLower(), m_aName);

    if (m_hasMenu)
        node.setAttribute(QString::fromLatin1("menu").toLower(), m_aMenu);


    QDomElement child;
    QDomText t;

    for (int i=0; i<m_eProperty.size(); ++i) {
        node.appendChild(m_eProperty.at(i)->write(doc, QString::fromLatin1("property").toLower()));
    }

    for (int i=0; i<m_eAttribute.size(); ++i) {
        node.appendChild(m_eAttribute.at(i)->write(doc, QString::fromLatin1("attribute").toLower()));
    }

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomActionRef::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("ActionRef").toLower());
    if (m_hasName)
        node.setAttribute(QString::fromLatin1("name").toLower(), m_aName);


    QDomElement child;
    QDomText t;

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomImages::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Images").toLower());

    QDomElement child;
    QDomText t;

    for (int i=0; i<m_eImage.size(); ++i) {
        node.appendChild(m_eImage.at(i)->write(doc, QString::fromLatin1("image").toLower()));
    }

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomImage::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Image").toLower());
    if (m_hasName)
        node.setAttribute(QString::fromLatin1("name").toLower(), m_aName);


    QDomElement child;
    QDomText t;

    if (m_eData)
        node.appendChild(m_eData->write(doc, QString::fromLatin1("data").toLower()));

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomImageData::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("ImageData").toLower());
    if (m_hasFormat)
        node.setAttribute(QString::fromLatin1("format").toLower(), m_aFormat);

    if (m_hasLength)
        node.setAttribute(QString::fromLatin1("length").toLower(), m_aLength);


    QDomElement child;
    QDomText t;

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomCustomWidgets::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("CustomWidgets").toLower());

    QDomElement child;
    QDomText t;

    for (int i=0; i<m_eCustomWidget.size(); ++i) {
        node.appendChild(m_eCustomWidget.at(i)->write(doc, QString::fromLatin1("customWidget").toLower()));
    }

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomHeader::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Header").toLower());
    if (m_hasLocation)
        node.setAttribute(QString::fromLatin1("location").toLower(), m_aLocation);


    QDomElement child;
    QDomText t;

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomCustomWidget::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("CustomWidget").toLower());

    QDomElement child;
    QDomText t;

    if (m_eClass.size()) {
        child = doc.createElement(QString::fromLatin1("class").toLower());
        t = doc.createTextNode(m_eClass);
        child.appendChild(t);
        node.appendChild(child);
    }

    if (m_eExtends.size()) {
        child = doc.createElement(QString::fromLatin1("extends").toLower());
        t = doc.createTextNode(m_eExtends);
        child.appendChild(t);
        node.appendChild(child);
    }

    if (m_eHeader)
        node.appendChild(m_eHeader->write(doc, QString::fromLatin1("header").toLower()));

    if (m_eSizeHint)
        node.appendChild(m_eSizeHint->write(doc, QString::fromLatin1("sizeHint").toLower()));

    child = doc.createElement(QString::fromLatin1("container").toLower());
    t = doc.createTextNode(QString::number(m_eContainer));
    child.appendChild(t);
    node.appendChild(child);

    if (m_eSizePolicy)
        node.appendChild(m_eSizePolicy->write(doc, QString::fromLatin1("sizePolicy").toLower()));

    if (m_ePixmap.size()) {
        child = doc.createElement(QString::fromLatin1("pixmap").toLower());
        t = doc.createTextNode(m_ePixmap);
        child.appendChild(t);
        node.appendChild(child);
    }

    if (m_eProperties)
        node.appendChild(m_eProperties->write(doc, QString::fromLatin1("properties").toLower()));

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomProperties::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Properties").toLower());

    QDomElement child;
    QDomText t;

    for (int i=0; i<m_eProperty.size(); ++i) {
        node.appendChild(m_eProperty.at(i)->write(doc, QString::fromLatin1("property").toLower()));
    }

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomPropertyData::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("PropertyData").toLower());
    if (m_hasType)
        node.setAttribute(QString::fromLatin1("type").toLower(), m_aType);


    QDomElement child;
    QDomText t;

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomSizePolicyData::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("SizePolicyData").toLower());

    QDomElement child;
    QDomText t;

    child = doc.createElement(QString::fromLatin1("horData").toLower());
    t = doc.createTextNode(QString::number(m_eHorData));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("verData").toLower());
    t = doc.createTextNode(QString::number(m_eVerData));
    child.appendChild(t);
    node.appendChild(child);

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomLayoutDefault::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("LayoutDefault").toLower());
    if (m_hasSpacing)
        node.setAttribute(QString::fromLatin1("spacing").toLower(), m_aSpacing);

    if (m_hasMargin)
        node.setAttribute(QString::fromLatin1("margin").toLower(), m_aMargin);


    QDomElement child;
    QDomText t;

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomLayoutFunction::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("LayoutFunction").toLower());
    if (m_hasSpacing)
        node.setAttribute(QString::fromLatin1("spacing").toLower(), m_aSpacing);

    if (m_hasMargin)
        node.setAttribute(QString::fromLatin1("margin").toLower(), m_aMargin);


    QDomElement child;
    QDomText t;

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomTabStops::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("TabStops").toLower());

    QDomElement child;
    QDomText t;

    for (int i=0; i<m_eTabStop.size(); ++i) {
        child = doc.createElement(QString::fromLatin1("tabstop").toLower());
        t = doc.createTextNode(m_eTabStop[i]);
        child.appendChild(t);
        node.appendChild(child);
    }

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomLayout::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Layout").toLower());
    if (m_hasClass)
        node.setAttribute(QString::fromLatin1("class").toLower(), m_aClass);


    QDomElement child;
    QDomText t;

    for (int i=0; i<m_eProperty.size(); ++i) {
        node.appendChild(m_eProperty.at(i)->write(doc, QString::fromLatin1("property").toLower()));
    }

    for (int i=0; i<m_eAttribute.size(); ++i) {
        node.appendChild(m_eAttribute.at(i)->write(doc, QString::fromLatin1("attribute").toLower()));
    }

    for (int i=0; i<m_eItem.size(); ++i) {
        node.appendChild(m_eItem.at(i)->write(doc, QString::fromLatin1("item").toLower()));
    }

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomLayoutItem::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Item").toLower());
    if (m_hasRow)
        node.setAttribute(QString::fromLatin1("row").toLower(), m_aRow);

    if (m_hasColumn)
        node.setAttribute(QString::fromLatin1("column").toLower(), m_aColumn);

    if (m_hasRowSpan)
        node.setAttribute(QString::fromLatin1("rowSpan").toLower(), m_aRowSpan);

    if (m_hasColSpan)
        node.setAttribute(QString::fromLatin1("colSpan").toLower(), m_aColSpan);


    QDomElement child;
    QDomText t;

    switch (m_kind) {
    case DomLayoutItem::Widget: {
    if (m_eWidget)
        node.appendChild(m_eWidget->write(doc, QString::fromLatin1("widget").toLower()));
    }
    break;

    case DomLayoutItem::Layout: {
    if (m_eLayout)
        node.appendChild(m_eLayout->write(doc, QString::fromLatin1("layout").toLower()));
    }
    break;

    case DomLayoutItem::Spacer: {
    if (m_eSpacer)
        node.appendChild(m_eSpacer->write(doc, QString::fromLatin1("spacer").toLower()));
    }
    break;

    default:
        break;
    } // end switch
    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomRow::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Row").toLower());

    QDomElement child;
    QDomText t;

    for (int i=0; i<m_eProperty.size(); ++i) {
        node.appendChild(m_eProperty.at(i)->write(doc, QString::fromLatin1("property").toLower()));
    }

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomColumn::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Column").toLower());

    QDomElement child;
    QDomText t;

    for (int i=0; i<m_eProperty.size(); ++i) {
        node.appendChild(m_eProperty.at(i)->write(doc, QString::fromLatin1("property").toLower()));
    }

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomItem::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Item").toLower());

    QDomElement child;
    QDomText t;

    for (int i=0; i<m_eProperty.size(); ++i) {
        node.appendChild(m_eProperty.at(i)->write(doc, QString::fromLatin1("property").toLower()));
    }

    for (int i=0; i<m_eItem.size(); ++i) {
        node.appendChild(m_eItem.at(i)->write(doc, QString::fromLatin1("item").toLower()));
    }

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomWidget::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Widget").toLower());
    if (m_hasClass)
        node.setAttribute(QString::fromLatin1("class").toLower(), m_aClass);

    if (m_hasName)
        node.setAttribute(QString::fromLatin1("name").toLower(), m_aName);


    QDomElement child;
    QDomText t;

    for (int i=0; i<m_eClass.size(); ++i) {
        child = doc.createElement(QString::fromLatin1("class").toLower());
        t = doc.createTextNode(m_eClass[i]);
        child.appendChild(t);
        node.appendChild(child);
    }

    for (int i=0; i<m_eProperty.size(); ++i) {
        node.appendChild(m_eProperty.at(i)->write(doc, QString::fromLatin1("property").toLower()));
    }

    for (int i=0; i<m_eAttribute.size(); ++i) {
        node.appendChild(m_eAttribute.at(i)->write(doc, QString::fromLatin1("attribute").toLower()));
    }

    for (int i=0; i<m_eRow.size(); ++i) {
        node.appendChild(m_eRow.at(i)->write(doc, QString::fromLatin1("row").toLower()));
    }

    for (int i=0; i<m_eColumn.size(); ++i) {
        node.appendChild(m_eColumn.at(i)->write(doc, QString::fromLatin1("column").toLower()));
    }

    for (int i=0; i<m_eItem.size(); ++i) {
        node.appendChild(m_eItem.at(i)->write(doc, QString::fromLatin1("item").toLower()));
    }

    for (int i=0; i<m_eLayout.size(); ++i) {
        node.appendChild(m_eLayout.at(i)->write(doc, QString::fromLatin1("layout").toLower()));
    }

    for (int i=0; i<m_eWidget.size(); ++i) {
        node.appendChild(m_eWidget.at(i)->write(doc, QString::fromLatin1("widget").toLower()));
    }

    for (int i=0; i<m_eAction.size(); ++i) {
        node.appendChild(m_eAction.at(i)->write(doc, QString::fromLatin1("action").toLower()));
    }

    for (int i=0; i<m_eActionGroup.size(); ++i) {
        node.appendChild(m_eActionGroup.at(i)->write(doc, QString::fromLatin1("actionGroup").toLower()));
    }

    for (int i=0; i<m_eAddAction.size(); ++i) {
        node.appendChild(m_eAddAction.at(i)->write(doc, QString::fromLatin1("addAction").toLower()));
    }

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomSpacer::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Spacer").toLower());
    if (m_hasName)
        node.setAttribute(QString::fromLatin1("name").toLower(), m_aName);


    QDomElement child;
    QDomText t;

    for (int i=0; i<m_eProperty.size(); ++i) {
        node.appendChild(m_eProperty.at(i)->write(doc, QString::fromLatin1("property").toLower()));
    }

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomColor::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Color").toLower());

    QDomElement child;
    QDomText t;

    child = doc.createElement(QString::fromLatin1("red").toLower());
    t = doc.createTextNode(QString::number(m_eRed));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("green").toLower());
    t = doc.createTextNode(QString::number(m_eGreen));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("blue").toLower());
    t = doc.createTextNode(QString::number(m_eBlue));
    child.appendChild(t);
    node.appendChild(child);

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomColorGroup::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("ColorGroup").toLower());

    QDomElement child;
    QDomText t;

    for (int i=0; i<m_eColor.size(); ++i) {
        node.appendChild(m_eColor.at(i)->write(doc, QString::fromLatin1("color").toLower()));
    }

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomPalette::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Palette").toLower());

    QDomElement child;
    QDomText t;

    if (m_eActive)
        node.appendChild(m_eActive->write(doc, QString::fromLatin1("active").toLower()));

    if (m_eInactive)
        node.appendChild(m_eInactive->write(doc, QString::fromLatin1("inactive").toLower()));

    if (m_eDisabled)
        node.appendChild(m_eDisabled->write(doc, QString::fromLatin1("disabled").toLower()));

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomFont::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Font").toLower());

    QDomElement child;
    QDomText t;

    if (m_eFamily.size()) {
        child = doc.createElement(QString::fromLatin1("family").toLower());
        t = doc.createTextNode(m_eFamily);
        child.appendChild(t);
        node.appendChild(child);
    }

    child = doc.createElement(QString::fromLatin1("pointSize").toLower());
    t = doc.createTextNode(QString::number(m_ePointSize));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("weight").toLower());
    t = doc.createTextNode(QString::number(m_eWeight));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("italic").toLower());
    t = doc.createTextNode(m_eItalic ? QLatin1String("true") : QLatin1String("false"));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("bold").toLower());
    t = doc.createTextNode(m_eBold ? QLatin1String("true") : QLatin1String("false"));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("underline").toLower());
    t = doc.createTextNode(m_eUnderline ? QLatin1String("true") : QLatin1String("false"));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("strikeOut").toLower());
    t = doc.createTextNode(m_eStrikeOut ? QLatin1String("true") : QLatin1String("false"));
    child.appendChild(t);
    node.appendChild(child);

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomPoint::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Point").toLower());

    QDomElement child;
    QDomText t;

    child = doc.createElement(QString::fromLatin1("x").toLower());
    t = doc.createTextNode(QString::number(m_eX));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("y").toLower());
    t = doc.createTextNode(QString::number(m_eY));
    child.appendChild(t);
    node.appendChild(child);

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomRect::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Rect").toLower());

    QDomElement child;
    QDomText t;

    child = doc.createElement(QString::fromLatin1("x").toLower());
    t = doc.createTextNode(QString::number(m_eX));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("y").toLower());
    t = doc.createTextNode(QString::number(m_eY));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("width").toLower());
    t = doc.createTextNode(QString::number(m_eWidth));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("height").toLower());
    t = doc.createTextNode(QString::number(m_eHeight));
    child.appendChild(t);
    node.appendChild(child);

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomSizePolicy::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("SizePolicy").toLower());

    QDomElement child;
    QDomText t;

    child = doc.createElement(QString::fromLatin1("hSizeType").toLower());
    t = doc.createTextNode(QString::number(m_eHSizeType));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("vSizeType").toLower());
    t = doc.createTextNode(QString::number(m_eVSizeType));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("horStretch").toLower());
    t = doc.createTextNode(QString::number(m_eHorStretch));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("verStretch").toLower());
    t = doc.createTextNode(QString::number(m_eVerStretch));
    child.appendChild(t);
    node.appendChild(child);

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomSize::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Size").toLower());

    QDomElement child;
    QDomText t;

    child = doc.createElement(QString::fromLatin1("width").toLower());
    t = doc.createTextNode(QString::number(m_eWidth));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("height").toLower());
    t = doc.createTextNode(QString::number(m_eHeight));
    child.appendChild(t);
    node.appendChild(child);

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomDate::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Date").toLower());

    QDomElement child;
    QDomText t;

    child = doc.createElement(QString::fromLatin1("year").toLower());
    t = doc.createTextNode(QString::number(m_eYear));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("month").toLower());
    t = doc.createTextNode(QString::number(m_eMonth));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("day").toLower());
    t = doc.createTextNode(QString::number(m_eDay));
    child.appendChild(t);
    node.appendChild(child);

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomTime::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Time").toLower());

    QDomElement child;
    QDomText t;

    child = doc.createElement(QString::fromLatin1("hour").toLower());
    t = doc.createTextNode(QString::number(m_eHour));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("minute").toLower());
    t = doc.createTextNode(QString::number(m_eMinute));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("second").toLower());
    t = doc.createTextNode(QString::number(m_eSecond));
    child.appendChild(t);
    node.appendChild(child);

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomDateTime::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("DateTime").toLower());

    QDomElement child;
    QDomText t;

    child = doc.createElement(QString::fromLatin1("hour").toLower());
    t = doc.createTextNode(QString::number(m_eHour));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("minute").toLower());
    t = doc.createTextNode(QString::number(m_eMinute));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("second").toLower());
    t = doc.createTextNode(QString::number(m_eSecond));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("year").toLower());
    t = doc.createTextNode(QString::number(m_eYear));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("month").toLower());
    t = doc.createTextNode(QString::number(m_eMonth));
    child.appendChild(t);
    node.appendChild(child);

    child = doc.createElement(QString::fromLatin1("day").toLower());
    t = doc.createTextNode(QString::number(m_eDay));
    child.appendChild(t);
    node.appendChild(child);

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomStringList::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("StringList").toLower());

    QDomElement child;
    QDomText t;

    for (int i=0; i<m_eString.size(); ++i) {
        child = doc.createElement(QString::fromLatin1("string").toLower());
        t = doc.createTextNode(m_eString[i]);
        child.appendChild(t);
        node.appendChild(child);
    }

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomString::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("String").toLower());
    if (m_hasNotr)
        node.setAttribute(QString::fromLatin1("notr").toLower(), m_aNotr);


    QDomElement child;
    QDomText t;

    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}

inline QDomElement DomProperty::write(QDomDocument &doc, const QString &tagName)
{
    QDomElement node = doc.createElement(tagName.size() ? tagName.toLower() : QString::fromLatin1("Property").toLower());
    if (m_hasName)
        node.setAttribute(QString::fromLatin1("name").toLower(), m_aName);

    if (m_hasStdset)
        node.setAttribute(QString::fromLatin1("stdset").toLower(), m_aStdset);


    QDomElement child;
    QDomText t;

    switch (m_kind) {
    case DomProperty::Bool: {
    if (m_eBool.size()) {
        child = doc.createElement(QString::fromLatin1("bool").toLower());
        t = doc.createTextNode(m_eBool);
        child.appendChild(t);
        node.appendChild(child);
    }
    }
    break;

    case DomProperty::Color: {
    if (m_eColor)
        node.appendChild(m_eColor->write(doc, QString::fromLatin1("color").toLower()));
    }
    break;

    case DomProperty::Cstring: {
    if (m_eCstring.size()) {
        child = doc.createElement(QString::fromLatin1("cstring").toLower());
        t = doc.createTextNode(m_eCstring);
        child.appendChild(t);
        node.appendChild(child);
    }
    }
    break;

    case DomProperty::Cursor: {
    child = doc.createElement(QString::fromLatin1("cursor").toLower());
    t = doc.createTextNode(QString::number(m_eCursor));
    child.appendChild(t);
    node.appendChild(child);
    }
    break;

    case DomProperty::Enum: {
    if (m_eEnum.size()) {
        child = doc.createElement(QString::fromLatin1("enum").toLower());
        t = doc.createTextNode(m_eEnum);
        child.appendChild(t);
        node.appendChild(child);
    }
    }
    break;

    case DomProperty::Font: {
    if (m_eFont)
        node.appendChild(m_eFont->write(doc, QString::fromLatin1("font").toLower()));
    }
    break;

    case DomProperty::IconSet: {
    if (m_eIconSet.size()) {
        child = doc.createElement(QString::fromLatin1("iconSet").toLower());
        t = doc.createTextNode(m_eIconSet);
        child.appendChild(t);
        node.appendChild(child);
    }
    }
    break;

    case DomProperty::Pixmap: {
    if (m_ePixmap.size()) {
        child = doc.createElement(QString::fromLatin1("pixmap").toLower());
        t = doc.createTextNode(m_ePixmap);
        child.appendChild(t);
        node.appendChild(child);
    }
    }
    break;

    case DomProperty::Palette: {
    if (m_ePalette)
        node.appendChild(m_ePalette->write(doc, QString::fromLatin1("palette").toLower()));
    }
    break;

    case DomProperty::Point: {
    if (m_ePoint)
        node.appendChild(m_ePoint->write(doc, QString::fromLatin1("point").toLower()));
    }
    break;

    case DomProperty::Rect: {
    if (m_eRect)
        node.appendChild(m_eRect->write(doc, QString::fromLatin1("rect").toLower()));
    }
    break;

    case DomProperty::Set: {
    if (m_eSet.size()) {
        child = doc.createElement(QString::fromLatin1("set").toLower());
        t = doc.createTextNode(m_eSet);
        child.appendChild(t);
        node.appendChild(child);
    }
    }
    break;

    case DomProperty::SizePolicy: {
    if (m_eSizePolicy)
        node.appendChild(m_eSizePolicy->write(doc, QString::fromLatin1("sizePolicy").toLower()));
    }
    break;

    case DomProperty::Size: {
    if (m_eSize)
        node.appendChild(m_eSize->write(doc, QString::fromLatin1("size").toLower()));
    }
    break;

    case DomProperty::String: {
    if (m_eString)
        node.appendChild(m_eString->write(doc, QString::fromLatin1("string").toLower()));
    }
    break;

    case DomProperty::StringList: {
    if (m_eStringList)
        node.appendChild(m_eStringList->write(doc, QString::fromLatin1("stringList").toLower()));
    }
    break;

    case DomProperty::Number: {
    child = doc.createElement(QString::fromLatin1("number").toLower());
    t = doc.createTextNode(QString::number(m_eNumber));
    child.appendChild(t);
    node.appendChild(child);
    }
    break;

    case DomProperty::Date: {
    if (m_eDate)
        node.appendChild(m_eDate->write(doc, QString::fromLatin1("date").toLower()));
    }
    break;

    case DomProperty::Time: {
    if (m_eTime)
        node.appendChild(m_eTime->write(doc, QString::fromLatin1("time").toLower()));
    }
    break;

    case DomProperty::DateTime: {
    if (m_eDateTime)
        node.appendChild(m_eDateTime->write(doc, QString::fromLatin1("dateTime").toLower()));
    }
    break;

    default:
        break;
    } // end switch
    if (m_text.size()) node.appendChild(doc.createTextNode(m_text));
    return node;
}



inline void DomUI::reset(bool full)
{
    if (full) {
    m_hasVersion = false;
    m_hasStdSetDef = false;
    }

    delete m_eWidget; m_eWidget = 0;
    delete m_eLayoutDefault; m_eLayoutDefault = 0;
    delete m_eLayoutFunction; m_eLayoutFunction = 0;
    delete m_eCustomWidgets; m_eCustomWidgets = 0;
    delete m_eTabStops; m_eTabStops = 0;
    delete m_eImages; m_eImages = 0;
    delete m_eIncludes; m_eIncludes = 0;
    m_text = QString::null;
}

inline void DomIncludes::reset(bool full)
{
    if (full) {
    }

    qDeleteAll(m_eInclude);
    m_text = QString::null;
}

inline void DomInclude::reset(bool full)
{
    if (full) {
    m_hasLocation = false;
    m_hasImpldecl = false;
    }

    m_text = QString::null;
}

inline void DomActionGroup::reset(bool full)
{
    if (full) {
    m_hasName = false;
    }

    qDeleteAll(m_eAction);
    qDeleteAll(m_eActionGroup);
    qDeleteAll(m_eProperty);
    qDeleteAll(m_eAttribute);
    m_text = QString::null;
}

inline void DomAction::reset(bool full)
{
    if (full) {
    m_hasName = false;
    m_hasMenu = false;
    }

    qDeleteAll(m_eProperty);
    qDeleteAll(m_eAttribute);
    m_text = QString::null;
}

inline void DomActionRef::reset(bool full)
{
    if (full) {
    m_hasName = false;
    }

    m_text = QString::null;
}

inline void DomImages::reset(bool full)
{
    if (full) {
    }

    qDeleteAll(m_eImage);
    m_text = QString::null;
}

inline void DomImage::reset(bool full)
{
    if (full) {
    m_hasName = false;
    }

    delete m_eData; m_eData = 0;
    m_text = QString::null;
}

inline void DomImageData::reset(bool full)
{
    if (full) {
    m_hasFormat = false;
    m_hasLength = false;
    }

    m_text = QString::null;
}

inline void DomCustomWidgets::reset(bool full)
{
    if (full) {
    }

    qDeleteAll(m_eCustomWidget);
    m_text = QString::null;
}

inline void DomHeader::reset(bool full)
{
    if (full) {
    m_hasLocation = false;
    }

    m_text = QString::null;
}

inline void DomCustomWidget::reset(bool full)
{
    if (full) {
    }

    delete m_eHeader; m_eHeader = 0;
    delete m_eSizeHint; m_eSizeHint = 0;
    delete m_eSizePolicy; m_eSizePolicy = 0;
    delete m_eProperties; m_eProperties = 0;
    m_text = QString::null;
}

inline void DomProperties::reset(bool full)
{
    if (full) {
    }

    qDeleteAll(m_eProperty);
    m_text = QString::null;
}

inline void DomPropertyData::reset(bool full)
{
    if (full) {
    m_hasType = false;
    }

    m_text = QString::null;
}

inline void DomSizePolicyData::reset(bool full)
{
    if (full) {
    }

    m_text = QString::null;
}

inline void DomLayoutDefault::reset(bool full)
{
    if (full) {
    m_hasSpacing = false;
    m_hasMargin = false;
    }

    m_text = QString::null;
}

inline void DomLayoutFunction::reset(bool full)
{
    if (full) {
    m_hasSpacing = false;
    m_hasMargin = false;
    }

    m_text = QString::null;
}

inline void DomTabStops::reset(bool full)
{
    if (full) {
    }

    m_text = QString::null;
}

inline void DomLayout::reset(bool full)
{
    if (full) {
    m_hasClass = false;
    }

    qDeleteAll(m_eProperty);
    qDeleteAll(m_eAttribute);
    qDeleteAll(m_eItem);
    m_text = QString::null;
}

inline void DomLayoutItem::reset(bool full)
{
    m_kind = Unknown;

    if (full) {
    m_hasRow = false;
    m_hasColumn = false;
    m_hasRowSpan = false;
    m_hasColSpan = false;
    }

    delete m_eWidget; m_eWidget = 0;
    delete m_eLayout; m_eLayout = 0;
    delete m_eSpacer; m_eSpacer = 0;
    m_text = QString::null;
}

inline void DomRow::reset(bool full)
{
    if (full) {
    }

    qDeleteAll(m_eProperty);
    m_text = QString::null;
}

inline void DomColumn::reset(bool full)
{
    if (full) {
    }

    qDeleteAll(m_eProperty);
    m_text = QString::null;
}

inline void DomItem::reset(bool full)
{
    if (full) {
    }

    qDeleteAll(m_eProperty);
    qDeleteAll(m_eItem);
    m_text = QString::null;
}

inline void DomWidget::reset(bool full)
{
    if (full) {
    m_hasClass = false;
    m_hasName = false;
    }

    qDeleteAll(m_eProperty);
    qDeleteAll(m_eAttribute);
    qDeleteAll(m_eRow);
    qDeleteAll(m_eColumn);
    qDeleteAll(m_eItem);
    qDeleteAll(m_eLayout);
    qDeleteAll(m_eWidget);
    qDeleteAll(m_eAction);
    qDeleteAll(m_eActionGroup);
    qDeleteAll(m_eAddAction);
    m_text = QString::null;
}

inline void DomSpacer::reset(bool full)
{
    if (full) {
    m_hasName = false;
    }

    qDeleteAll(m_eProperty);
    m_text = QString::null;
}

inline void DomColor::reset(bool full)
{
    if (full) {
    }

    m_text = QString::null;
}

inline void DomColorGroup::reset(bool full)
{
    if (full) {
    }

    qDeleteAll(m_eColor);
    m_text = QString::null;
}

inline void DomPalette::reset(bool full)
{
    if (full) {
    }

    delete m_eActive; m_eActive = 0;
    delete m_eInactive; m_eInactive = 0;
    delete m_eDisabled; m_eDisabled = 0;
    m_text = QString::null;
}

inline void DomFont::reset(bool full)
{
    if (full) {
    }

    m_text = QString::null;
}

inline void DomPoint::reset(bool full)
{
    if (full) {
    }

    m_text = QString::null;
}

inline void DomRect::reset(bool full)
{
    if (full) {
    }

    m_text = QString::null;
}

inline void DomSizePolicy::reset(bool full)
{
    if (full) {
    }

    m_text = QString::null;
}

inline void DomSize::reset(bool full)
{
    if (full) {
    }

    m_text = QString::null;
}

inline void DomDate::reset(bool full)
{
    if (full) {
    }

    m_text = QString::null;
}

inline void DomTime::reset(bool full)
{
    if (full) {
    }

    m_text = QString::null;
}

inline void DomDateTime::reset(bool full)
{
    if (full) {
    }

    m_text = QString::null;
}

inline void DomStringList::reset(bool full)
{
    if (full) {
    }

    m_text = QString::null;
}

inline void DomString::reset(bool full)
{
    if (full) {
    m_hasNotr = false;
    }

    m_text = QString::null;
}

inline void DomProperty::reset(bool full)
{
    m_kind = Unknown;

    if (full) {
    m_hasName = false;
    m_hasStdset = false;
    }

    delete m_eColor; m_eColor = 0;
    delete m_eFont; m_eFont = 0;
    delete m_ePalette; m_ePalette = 0;
    delete m_ePoint; m_ePoint = 0;
    delete m_eRect; m_eRect = 0;
    delete m_eSizePolicy; m_eSizePolicy = 0;
    delete m_eSize; m_eSize = 0;
    delete m_eString; m_eString = 0;
    delete m_eStringList; m_eStringList = 0;
    delete m_eDate; m_eDate = 0;
    delete m_eTime; m_eTime = 0;
    delete m_eDateTime; m_eDateTime = 0;
    m_text = QString::null;
}



#endif // UI4_H
