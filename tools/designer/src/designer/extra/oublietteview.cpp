/****************************************************************************
**
** Copyright (C) 2005-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QtGui/QScrollBar>
#include "oubliette.h"
#include "oublietteview.h"


OublietteView::OublietteView()
{
    m_oubliette = new Oubliette;
    setWidget(m_oubliette);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    connect(m_oubliette, SIGNAL(characterMoved(QPoint)),
            this, SLOT(scrollToCharacter(QPoint)));
    setFocusPolicy(Qt::NoFocus);
    m_oubliette->setFocus();
    scrollToCharacter(m_oubliette->visualCursorPos());
}

OublietteView::~OublietteView()
{
}

void OublietteView::scrollToCharacter(const QPoint &pt)
{
    bool needUpdate = false;
    if (qAbs(pt.x() - horizontalScrollBar()->value()) >= 10 * 32) {
        horizontalScrollBar()->setValue(pt.x() - width() / 2);
        needUpdate = true;
    }
    if (qAbs(pt.y() - (verticalScrollBar()->value())) >= 6 * 32) {
        verticalScrollBar()->setValue(pt.y() - height() / 2);
        needUpdate = true;
    }
    if (needUpdate)
        scrollContentsBy(0, 0);
}
