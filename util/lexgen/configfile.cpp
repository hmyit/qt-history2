/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_GPL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/
#include "configfile.h"

#include <QFile>

ConfigFile::SectionMap ConfigFile::parse(const QString &fileName)
{
    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly))
        return ConfigFile::SectionMap();
    return parse(&f);
}

ConfigFile::SectionMap ConfigFile::parse(QIODevice *dev)
{
    SectionMap sections;
    SectionMap::Iterator currentSection = sections.end();

    ConfigFile::SectionMap result;
    int currentLineNumber = 0;
    while (!dev->atEnd()) {
        QString line = QString::fromUtf8(dev->readLine()).trimmed();
        ++currentLineNumber;

        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;

        if (line.startsWith(QLatin1Char('['))) {
            if (!line.endsWith(']')) {
                qWarning("Syntax error at line %d: Missing ']' at start of new section.", currentLineNumber);
                return SectionMap();
            }
            line.remove(0, 1);
            line.chop(1);
            const QString sectionName = line;
            currentSection = sections.insert(sectionName, Section());
            continue;
        }

        if (currentSection == sections.end()) {
            qWarning("Syntax error at line %d: Entry found outside of any section.", currentLineNumber);
            return SectionMap();
        }

        Entry e;
        e.lineNumber = currentLineNumber;

        int equalPos = line.indexOf(QLatin1Char('='));
        if (equalPos == -1) {
            e.key = line;
        } else {
            e.key = line;
            e.key.truncate(equalPos);
            e.key = e.key.trimmed();
            e.value = line.mid(equalPos + 1).trimmed();
        }
        currentSection->append(e);
    }
    return sections;
}
