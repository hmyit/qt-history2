/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef SCRIPTENGINE_P_H
#define SCRIPTENGINE_P_H

#ifndef QT_H
#include "qtextengine_p.h"
#endif // QT_H

class QString;
struct QGlyphLayout;
struct QCharAttributes;

struct QShaperItem {
    int script;
    const QString *string;
    int from;
    int length;
    QFontEngine *font;
    QGlyphLayout *glyphs;
    int num_glyphs; // in: available glyphs out: glyphs used/needed
    unsigned short *log_clusters;
    int flags;
};

// return true if ok.
typedef bool (*ShapeFunction)(QShaperItem *item);
//typedef void (*ShapeFunction)(int script, const QString &, int, int, QTextEngine *, QScriptItem *);
typedef void (*AttributeFunction)(int script, const QString &, int, int, QCharAttributes *);

struct q_scriptEngine {
    ShapeFunction shape;
    AttributeFunction charAttributes;
};

extern const q_scriptEngine scriptEngines[];

#endif
