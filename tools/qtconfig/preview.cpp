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

#include "preview.h"
#include <qpen.h>
#include <qpainter.h>

PreviewWidget::PreviewWidget(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
}

PreviewWidget::~PreviewWidget()
{
}

PreviewFrame::PreviewFrame(QWidget *parent)
    : QFrame(parent)
{
    setMinimumSize(200, 200);
    setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    setLineWidth(1);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);
    PreviewWorkspace * w = new PreviewWorkspace( this );
    vbox->addWidget(w);

    previewWidget = new PreviewWidget(w);
    QWidget *frame = w->addWindow(previewWidget,
                Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint);
    frame->move(10,10);
    frame->show();
}

void PreviewFrame::setPreviewPalette(const QPalette &pal)
{
    previewWidget->setPalette(pal);
}

PreviewWorkspace::PreviewWorkspace(QWidget *parent)
    : QWorkspace(parent)
{
    setBackgroundRole(QPalette::Dark);
    setAutoFillBackground(true);
}


void PreviewWorkspace::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setPen(QPen(Qt::white));
    p.drawText(0, height() / 2,  width(), height(), Qt::AlignHCenter,
               tr("The moose in the noose\nate the goose who was loose."));
}
