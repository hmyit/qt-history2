/**********************************************************************
** Copyright (C) 2000-2002 Trolltech AS.  All rights reserved.
**
** This file is part of Qt Designer.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/gpl/ for GPL licensing information.
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef PREVIEWFRAME_H
#define PREVIEWFRAME_H

#include <QtGui/QWorkspace>
#include <QtGui/QFrame>

namespace qdesigner { namespace components { namespace propertyeditor {

class PreviewWidget;

class PreviewWorkspace: public QWorkspace
{
    Q_OBJECT
public:
    PreviewWorkspace(QWidget *parent)
        : QWorkspace(parent) {}

    virtual ~PreviewWorkspace() {}

protected:
    void paintEvent(QPaintEvent *);
};

class PreviewFrame: public QFrame
{
    Q_OBJECT
public:
    PreviewFrame(QWidget *parent);

    void setPreviewPalette(const QPalette &palette);

private:
    PreviewWidget *previewWidget;
};

} } } // namespace qdesigner::components::propertyeditor

#endif
