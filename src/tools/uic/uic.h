/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of Qt Designer.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef UIC_H
#define UIC_H

#include "databaseinfo.h"

#include <qstring.h>
#include <qstringlist.h>
#include <qhash.h>
#include <qstack.h>

class QTextStream;
class QIODevice;

class Driver;
class DomUI;
class DomWidget;
class DomSpacer;
class DomLayout;
class DomLayoutItem;
class DomItem;

struct Option;

class Uic
{
public:
    Uic(Driver *driver);
    ~Uic();

    inline Driver *driver() const
    { return drv; }

    inline QTextStream &output()
    { return out; }

    inline const Option &option() const
    { return opt; }

    inline const DatabaseInfo *databaseInfo() const
    { return &info; }

    bool write(QIODevice *in);
    bool write(DomUI *ui);

private:
    // copyright header
    void writeCopyrightHeader(DomUI *ui);

    // header protection
    void writeHeaderProtectionStart();
    void writeHeaderProtectionEnd();

private:
    Driver *drv;
    QTextStream &out;
    const Option &opt;
    DatabaseInfo info;
};

#endif // UIC_H
