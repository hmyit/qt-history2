/****************************************************************************
**
** Definition of ProjectBuilderMakefileGenerator class.
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of qmake.
** EDITIONS: FREE, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef __PBUILDER_PBX_H__
#define __PBUILDER_PBX_H__

#include "unixmake.h"

class ProjectBuilderMakefileGenerator : public UnixMakefileGenerator
{
    QString pbx_dir;
    int pbuilderVersion() const;
    bool writeMakeParts(QTextStream &);
    bool writeMakefile(QTextStream &);

    QString pbxbuild();
    QMap<QString, QString> keys;
    QString keyFor(const QString &file);
    QString fixEnvs(const QString &file);
    QString fixEnvsList(const QString &where);
    QString reftypeForFile(const QString &where);

    enum IDE_TYPE { MAC_XCODE, MAC_PBUILDER };
    IDE_TYPE ideType() const;

public:
    ProjectBuilderMakefileGenerator(QMakeProject *p);
    ~ProjectBuilderMakefileGenerator();

    virtual bool openOutput(QFile &) const;
protected:
    virtual bool doDepends() const { return FALSE; } //never necesary
};

inline ProjectBuilderMakefileGenerator::~ProjectBuilderMakefileGenerator()
{ }


#endif /* __PBUILDER_PBX_H__ */
